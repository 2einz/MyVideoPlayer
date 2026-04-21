#include "player/engine/thread/video_decode_thread.h"

#include <iostream>
#include <chrono>

#include "player/engine/decode/video_decoder.h"
#include "player/engine/buffer/video_packet_queue.h"
#include "player/media_state.hpp"
#include "player/engine/common/engine_types.h"

namespace my_video_player {

VideoDecodeThread::~VideoDecodeThread() {
    stop();
}

void VideoDecodeThread::start(VideoDecoder* decoder,
                              VideoPacketQueue* pkt_queue,
                              MediaState* state,
                              AVRational time_base,
                              std::atomic<int>* serial,
                              int stream_index) {
    stop();
    decoder_ = decoder;
    pkt_queue_ = pkt_queue;
    media_state_ = state;
    current_serial_ = serial;
    time_base_ = time_base;
    video_stream_index_ = stream_index;

    running_ = true;
    thread_ = std::thread(&VideoDecodeThread::run, this);
}

void VideoDecodeThread::stop() {
    if (!running_)
        return;
    running_ = false;

    pause_cv_.notify_all();

    if (pkt_queue_) {
        pkt_queue_->Abort();
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

void VideoDecodeThread::wake_up() {
    // 唤醒等待在条件变量上的线程
    pause_cv_.notify_all();
}

void VideoDecodeThread::run() {
    double last_pts = -1.0;
    auto last_render_time = std::chrono::steady_clock::now();

    double accurate_seek_target = -1.0;
    bool is_seeking_exact = false;

    LOG_INFO(LM::kThread, "VideoDecodeThread has been Started, Waiting for packets...");

    while (running_) {
        if (media_state_->IsStopped() || media_state_->HasError())
            break;

        if (media_state_->IsPaused()) {
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this]() { return !media_state_->IsPaused() || !running_; });
            continue;
        }

        // 拿包：如果解复用器读到结尾关闭了队列，这里会返回 false，跳出 while 循环
        PacketItem pkt;
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

        while (running_) {
            FrameItem frame;
            if (decoder_->ReceiveFrame(&frame, time_base_) != 0) {
                break; // 没有更多帧了，拿下一个包
            }

            if (is_seeking_exact) {
                if (frame.pts < accurate_seek_target)
                    continue;
                is_seeking_exact = false;
                media_state_->Dispatch(Action::kReachedSeekTarget);
            }

            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count() % 1000000;


            LOG_INFO(LM::kThread, "Render PTS: {}, Now_ms: {}", frame.pts, now_ms);

            if (on_frame_ready_) {
                on_frame_ready_(frame);
            }

            now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count() % 1000000;


            LOG_INFO(LM::kThread, "Render PTS: {}, Now_ms: {}", frame.pts, now_ms);
            // 帧率控制
            if (last_pts >= 0) {
                double diff = frame.pts - last_pts;
                if (diff > 0 && diff < 1.0) { // 合理的帧间距
                    auto delay = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::duration<double>(diff));

                    auto target_time = last_render_time + delay;
                    auto now = std::chrono::steady_clock::now();

                    // 【核心修复】：如果目标时间比现在早了 100ms 以上，说明追不上了
                    if (now > target_time + std::chrono::milliseconds(100)) {
                        // 记录一下，方便调试
                         LOG_DEBUG(LM::kThread, "Too late! Resetting clock. PTS: {}", frame.pts);
                        last_render_time = now; // 强制重置为当前时间
                    }
                    else {
                        // 只有在没迟到太久的情况下，才进行精准休眠
                        std::this_thread::sleep_until(target_time);
                        last_render_time = target_time; // 保持理论时间轴的严格递增
                    }
                }
                else {
                    // 如果 diff 异常（比如 PTS 跳变），立刻同步到当前时间
                    last_render_time = std::chrono::steady_clock::now();
                }
            }
            else {
                // 第一帧，直接记录起始时间
                last_render_time = std::chrono::steady_clock::now();
            }
            last_pts = frame.pts;
        }
    } //

    if (!media_state_->IsStopped() && !media_state_->HasError()) {
        LOG_INFO(LM::kThread, "VideoDecodeThread is draining decoder...");

        // 发送空包告诉解码器：没有数据了，把肚子里的存货全吐出来
        PacketItem null_pkt;
        null_pkt.packet = nullptr;
        decoder_->SendPacket(null_pkt);

        while (running_) {
            FrameItem frame;
            if (decoder_->ReceiveFrame(&frame, time_base_) != 0) break;

            if (on_frame_ready_) {
                on_frame_ready_(frame);
            }

            // 注意：排空出来的最后几帧也需要控制播放速度，否则会瞬间闪过去
            if (last_pts >= 0) {
                double diff = frame.pts - last_pts;
                if (diff > 0 && diff < 1.0) { // 合理的帧间距
                    auto delay = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::duration<double>(diff));

                    auto target_time = last_render_time + delay;
                    auto now = std::chrono::steady_clock::now();

                    // 【核心修复】：如果目标时间比现在早了 100ms 以上，说明追不上了
                    if (now > target_time + std::chrono::milliseconds(100)) {
                        // 记录一下，方便调试
                         LOG_DEBUG(LM::kThread, "Too late! Resetting clock. PTS: {}", frame.pts);
                        last_render_time = now; // 强制重置为当前时间
                    }
                    else {
                        // 只有在没迟到太久的情况下，才进行精准休眠
                        std::this_thread::sleep_until(target_time);
                        last_render_time = target_time; // 保持理论时间轴的严格递增
                    }
                }
                else {
                    // 如果 diff 异常（比如 PTS 跳变），立刻同步到当前时间
                    last_render_time = std::chrono::steady_clock::now();
                }
            }
            else {
                // 第一帧，直接记录起始时间
                last_render_time = std::chrono::steady_clock::now();
            }
            last_pts = frame.pts;
        }

        LOG_INFO(LM::kThread, "Playback finished naturally.");
        media_state_->Dispatch(Action::kReachedEnd);
    }

    LOG_INFO(LM::kThread, "VideoDecodeThread is exiting...");
}

} // namespace my_video_player