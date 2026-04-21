#include "player/engine/thread/video_decode_thread.h"

#include <iostream>
#include <chrono>

#include "player/engine/decode/video_decoder.h"
#include "player/engine/buffer/video_packet_queue.h"
#include "player/media_state.hpp"
#include "player/engine/common/engine_types.h"

namespace my_video_player {

VideoDecodeThread::~VideoDecodeThread() {
    Stop();
}

void VideoDecodeThread::Start(VideoDecoder* decoder,
                              VideoPacketQueue* pkt_queue,
                              MediaState* state,
                              AVRational time_base,
                              std::atomic<int>* serial,
                              int stream_index) {
    decoder_ = decoder;
    pkt_queue_ = pkt_queue;
    media_state_ = state;
    current_serial_ = serial;
    time_base_ = time_base;
    video_stream_index_ = stream_index;

    running_ = true;
    thread_ = std::thread(&VideoDecodeThread::Run, this);
}

void VideoDecodeThread::Stop() {
    if (!running_)
        return;
    running_ = false;

    pause_cv_.notify_all();
    // 外部 (Player::Stop) 必须先调用 pkt_queue_->Abort() 唤醒可能阻塞的 Pop()
    if (thread_.joinable()) {
        thread_.join();
    }
}

void VideoDecodeThread::WakeUp() {
    // 唤醒等待在条件变量上的线程
    pause_cv_.notify_all();
}

void VideoDecodeThread::Run() {
    PacketItem pkt;
    FrameItem frame;

    double last_pts = -1.0;
    auto last_render_time = std::chrono::steady_clock::now();

    double accurate_seek_target = -1.0;
    bool is_seeking_exact = false;

    std::cout << "[VideoDecodeThread] Started. Waiting for packets..." << std::endl;

    // ==========================================
    // 1. 正常的主循环：不断从队列拿包并解码
    // ==========================================
    while (running_) {
        if (media_state_->IsStopped() || media_state_->HasError())
            break;

        if (media_state_->IsPaused()) {
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this]() { return !media_state_->IsPaused() || !running_; });
            continue;
        }

        // 拿包：如果解复用器读到结尾关闭了队列，这里会返回 false，跳出 while 循环
        if (!pkt_queue_->Pop(pkt)) {
            break;
        }

        if (pkt.flush) {
            accurate_seek_target = media_state_->ConsumeSeekPosition();
            is_seeking_exact = true;
            last_pts = -1.0;
            decoder_->Flush();
            continue;
        }

        if (pkt.serial != current_serial_->load()) {
            continue;
        }

        if (pkt.stream_index != video_stream_index_) {
            continue;
        }

        decoder_->SendPacket(pkt);

        while (decoder_->ReceiveFrame(&frame, time_base_) == 0) {
            if (is_seeking_exact) {
                if (frame.pts < accurate_seek_target)
                    continue;
                is_seeking_exact = false;
                media_state_->Dispatch(Action::kReachedSeekTarget);
            }

            if (on_frame_ready_) {
                on_frame_ready_(frame);
            }

            // 帧率控制
            if (last_pts >= 0) {
                double diff = frame.pts - last_pts;
                if (diff > 0 && diff < 1.0) {
                    auto delay =
                        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(diff));
                    auto target_time = last_render_time + delay;
                    std::this_thread::sleep_until(target_time);
                    last_render_time = target_time; // 严格递增
                } else {
                    last_render_time = std::chrono::steady_clock::now();
                }
            } else {
                last_render_time = std::chrono::steady_clock::now();
            }
            last_pts = frame.pts;
        }
    } // <--- 注意：正常的 while 循环在这里结束！！！

    if (!media_state_->IsStopped() && !media_state_->HasError()) {
        std::cout << "[VideoDecodeThread] Draining decoder..." << std::endl;

        // 发送空包告诉解码器：没有数据了，把肚子里的存货全吐出来
        PacketItem null_pkt;
        null_pkt.packet = nullptr;
        decoder_->SendPacket(null_pkt);

        while (decoder_->ReceiveFrame(&frame, time_base_) == 0) {
            if (on_frame_ready_) {
                on_frame_ready_(frame);
            }

            // 注意：排空出来的最后几帧也需要控制播放速度，否则会瞬间闪过去
            if (last_pts >= 0) {
                double diff = frame.pts - last_pts;
                if (diff > 0 && diff < 1.0) {
                    auto delay =
                        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(diff));
                    auto target_time = last_render_time + delay;
                    std::this_thread::sleep_until(target_time);
                    last_render_time = target_time;
                }
            }
            last_pts = frame.pts;
            
        }

        std::cout << "[VideoDecodeThread] Playback finished naturally." << std::endl;
        media_state_->Dispatch(Action::kReachedEnd);
    }

    std::cout << "[VideoDecodeThread] Thread exiting..." << std::endl;
}

} // namespace my_video_player