#include "player/player.h"

#include <iostream>
#include <thread>
#include <chrono>

namespace my_video_player {

Player::Player() {}

int Player::Open(const std::string& url) {
    std::cout << "[Player] Attempting to open: " << url << std::endl;

    // 打开文件
    int ret = demuxer_.Open(url);
    if (ret < 0) {
        std::cerr << "[Player] Demuxer open failed: " << ret << std::endl;
        return -1;
    }

    // 初始化视频解码器
    AVCodecParameters* v_params = demuxer_.GetVideoCodecParameters();
    if (!v_params) {
        std::cerr << "[Player] Could not find video codec parameters." << std::endl;
        return -2; // 报错返回 -2
    }
    std::cout << "[Player] Found video stream, codec_id: " << v_params->codec_id << std::endl;

    ret = video_decoder_.Init(v_params);
    if (ret < 0) {
        std::cerr << "[Player] Video decoder init failed: " << ret << std::endl;
        return -3; // 报错返回 -3
    }
    std::cout << "[Player] Video decoder initialized." << std::endl;

    return 0;
}

double Player::GetDuration() const {
    if (!demuxer_.format_context())
        return 0.0;
    auto duration = demuxer_.format_context()->duration;
    if (duration == AV_NOPTS_VALUE)
        return 0.0;
    return static_cast<double>(duration) / AV_TIME_BASE;
}

const AVCodecParameters* Player::GetVideoCodecParams() const {
    return demuxer_.GetVideoCodecParameters();
}

void Player::StartLoop(std::atomic<bool>& thread_runing) {
    auto* fmt_ctx = demuxer_.format_context();
    if (!fmt_ctx)
        return;

    AVRational tb = fmt_ctx->streams[demuxer_.video_stream_index()]->time_base;

    double total_duration = GetDuration();

    PacketItem pkt;
    FrameItem frame;
    double last_pts = -1.0;
    std::cout << "[Player] PlayLoop Start" << std::endl;

    while (thread_runing) {
        State current_state = media_state_.GetState();

        // 停止和错误处理
        if (current_state == State::kStopped || current_state == State::kError)
            break;

        // 处理暂停
        if (current_state == State::kPaused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        // 处理Seek
        if (media_state_.HasSeekRequest()) {
            double target = media_state_.ConsumeSeekPosition();

            demuxer_.Seek(target);

            // 清理缓冲区
            video_decoder_.Flush();

            last_pts = -1.0;

            media_state_.Dispatch(Action::kReachedSeekTarget);
            continue;
        }

        // 读取与解码逻辑
        if (demuxer_.ReadPacket(&pkt) < 0) {
            media_state_.Dispatch(Action::kReachedEnd);
            if (on_finished_)
                on_finished_();
            break;
        }

        if (pkt.stream_index == demuxer_.video_stream_index()) {
            if (video_decoder_.SendPacket(pkt) == 0) {
                while (video_decoder_.ReceiveFrame(&frame, tb) == 0) {
                    // 触发上层 UI 渲染与进度更新
                    if (on_frame_ready_) {
                        on_frame_ready_(frame);
                    }

                    // 简易帧率同步
                    if (last_pts >= 0) {
                        double diff = frame.pts - last_pts;
                        if (diff > 0 && diff < 1.0) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(diff * 1000)));
                        }
                    }
                    last_pts = frame.pts;
                }
            }
        }

    } // namespace my_video_player