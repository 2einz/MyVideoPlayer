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

    while (running_) {
        if (media_state_->IsStopped() || media_state_->HasError())
            break;

        if (media_state_->IsPaused()) {
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this]() { return !media_state_->IsPaused() || !running_; });
            continue;
        }

        // 拿包
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
            // 旧包
            continue;
        }

        // 正常解包
        if (pkt.stream_index != video_stream_index_) {
            continue;
        }

        decoder_->SendPacket(pkt); // 忽略 EAGAIN，只要不是底层崩溃，就尝试掏帧
        // 一个包可能有多帧
        while (decoder_->ReceiveFrame(&frame, time_base_) == 0) {
            // 精确 Seek
            if (is_seeking_exact) {
                if (frame.pts < accurate_seek_target) { // 丢帧
                    continue;
                }

                is_seeking_exact = false;

                media_state_->Dispatch(Action::kReachedSeekTarget);
            }

            // 渲染
            if (on_frame_ready_) {
                on_frame_ready_(frame);
            }

            // 简易帧率控制
            if (last_pts >= 0) {
                double diff = frame.pts - last_pts;
                if (diff > 0 && diff < 1.0) { // 防御异常时间戳
                    auto target_time = last_render_time + std::chrono::duration<double>(diff);

                    std::this_thread::sleep_until(target_time);
                }
            }
            last_render_time = std::chrono::steady_clock::now();
            last_pts = frame.pts;
        }
    }
    std::cout << "[VideoDecodeThread] Thread exiting..." << std::endl;
}
} // namespace my_video_player