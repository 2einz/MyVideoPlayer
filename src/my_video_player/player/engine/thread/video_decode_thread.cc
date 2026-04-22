#include "player/engine/thread/video_decode_thread.h"

#include <iostream>
#include <chrono>

#include "player/engine/decode/video_decoder.h"
#include "player/engine/buffer/video_packet_queue.h"
#include "player/media_state.hpp"
#include "player/engine/common/engine_types.h"

namespace my_video_player {

void VideoDecodeThread::start(VideoDecoder* decoder,
                              VideoPacketQueue* pkt_queue,
                              MediaState* state,
                              AVRational time_base,
                              std::atomic<int>* serial,
                              int stream_index) {
    this->stop();

    decoder_ = decoder;
    pkt_queue_ = pkt_queue;
    media_state_ = state;
    current_serial_ = serial;
    time_base_ = time_base;
    video_stream_index_ = stream_index;

    MyThread::start();
}

void VideoDecodeThread::stop() {
    pause_cv_.notify_all();

    if (pkt_queue_) {
        pkt_queue_->Abort();
    }

    MyThread::stop();
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

    FrameItem last_frame; // 保存底帧
    bool has_last_frame = false;


    LOG_INFO(LM::kThread, "VideoDecodeThread has been Started.");

    // Main Loop
    while (running_) {
        if (media_state_->IsStopped() || media_state_->HasError())
            break;

        // Pause processing
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
            has_last_frame = false;
            continue;
        }

        if (pkt.serial != current_serial_->load()) {
            continue;
        }

        if (pkt.stream_index != video_stream_index_) {
            continue;
        }

        decoder_->SendPacket(pkt);

        // 掏出当前包能解出来的所有帧
        while (running_) {
            FrameItem frame;
            if (decoder_->ReceiveFrame(&frame, time_base_) != 0) break;

            last_frame.assign_from(frame); // 实时更新最后一帧
            has_last_frame = true;


            // 精确 Seek
            if (is_seeking_exact) {
                if (frame.pts < accurate_seek_target)
                    continue;
                is_seeking_exact = false;
                media_state_->Dispatch(Action::kReachedSeekTarget);
            }

            render_and_sync(frame, last_pts, last_render_time);
        }
    }

    // Drain phase
    bool can_drain = running_ &&
        !media_state_->IsStopped() &&
        !media_state_->IsPaused() &&
        !media_state_->IsSeeking() &&
        !media_state_->HasError();

    if (can_drain) {
        LOG_INFO(LM::kThread, "Main loop end, entering final drain phase.");

        decoder_->SendPacket(PacketItem()); // 传空包

        while (running_) {
            FrameItem frame;

            //int ret = decoder_->ReceiveFrame(&frame, time_base_);

            //if (ret == AVERROR_EOF)
            //    break;
            //if (ret < 0)
            //    break;

            if (decoder_->ReceiveFrame(&frame, time_base_) != 0) break;

            last_frame.assign_from(frame); // 即使在 Drain 阶段也更新最后一帧
            has_last_frame = true;

            if (is_seeking_exact) {
                //if (frame.pts < accurate_seek_target)
                //    continue;
                is_seeking_exact = false;
                media_state_->Dispatch(Action::kReachedSeekTarget);
            }
            render_and_sync(frame, last_pts, last_render_time);
        }

        // EOF 后的保底逻辑
        if (is_seeking_exact) {
            if (has_last_frame && on_frame_ready_) {
                LOG_WARN(LM::kThread, "Seek never reached target! Target: {}, Last PTS: {}",
                    accurate_seek_target, last_pts);

                on_frame_ready_(last_frame); // 强行画最后一帧
                LOG_INFO(LM::kThread, "Seek target exceeded EOF, rendering last frame.");
            }
            is_seeking_exact = false;
            media_state_->Dispatch(Action::kReachedSeekTarget);
        }

        LOG_INFO(LM::kThread, "Playback finished naturally");
        media_state_->Dispatch(Action::kReachedEnd);
    }

    if (media_state_->IsSeeking()) {
        is_seeking_exact = false;
        // 只有没停止时才 Dispatch，防止 Stopped -> ReachedSeekTarget 警告
        if (!media_state_->IsStopped()) {
            media_state_->Dispatch(Action::kReachedSeekTarget);
        }
    }

    LOG_INFO(LM::kThread, "VideoDecodeThread is exiting...");
}

void VideoDecodeThread::render_and_sync(FrameItem& frame,
                                        double& last_pts,
                                        std::chrono::steady_clock::time_point& last_render_time) {
    if (on_frame_ready_) {
        on_frame_ready_(frame);
    }

    if (last_pts >= 0) {
        double diff = frame.pts - last_pts;
        if (diff >= 0 && diff < 1.0) {
            auto delay = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(diff));
            auto target_time = last_render_time + delay;
            auto now = std::chrono::steady_clock::now();

            if (now > target_time + std::chrono::milliseconds(100)) {
                last_render_time = now;
            } else {
                std::this_thread::sleep_until(target_time);
                last_render_time = target_time;
            }
        } else {
            last_render_time = std::chrono::steady_clock::now();
        }
    } else {
        last_render_time = std::chrono::steady_clock::now();
    }
    last_pts = frame.pts;
}

} // namespace my_video_player