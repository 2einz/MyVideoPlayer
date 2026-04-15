#include "player/player.h"

#include <iostream>

extern "C" {
#include <libavutil/rational.h>
}

namespace my_video_player {

int Player::Open(const std::string& url) {
    std::cout << "[Player] Attempting to open: " << url << std::endl;

    media_state_.Dispatch(Action::kOpen);

    // 打开文件
    int ret = demuxer_.Open(url);
    if (ret < 0) {
        std::cerr << "[Player] Demuxer open failed: " << ret << std::endl;
        media_state_.Dispatch(Action::kError);
        return -1;
    }

    // 初始化视频解码器
    AVCodecParameters* v_params = demuxer_.GetVideoCodecParameters();
    if (!v_params) {
        std::cerr << "[Player] Could not find video codec parameters." << std::endl;
        media_state_.Dispatch(Action::kError);
        return -1; // 报错返回 -2
    }

    ret = video_decoder_.Init(v_params);
    if (ret < 0) {
        std::cerr << "[Player] Video decoder init failed: " << ret << std::endl;
        media_state_.Dispatch(Action::kError);
        return -ret; // 报错返回 -3
    }

    media_state_.Dispatch(Action::kPrepared);

    std::cout << "[Player] Open success, Prepared." << std::endl;

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

AVCodecParameters* Player::GetVideoCodecParams() {
    return demuxer_.GetVideoCodecParameters();
}
const AVCodecParameters* Player::GetVideoCodecParams() const {
    return demuxer_.GetVideoCodecParameters();
}

void Player::Play() {
    if (media_state_.IsEnded()) {
        media_state_.Seek(0.0); // 状态先切到 Seeking (变灰)

        // 重新拉起底层线程

        // 执行到 0 秒的 Seek 操作
        int new_serial = ++video_serial_;
        video_pkt_queue_.Flush();
        demuxer_.Seek(0.0);
        video_pkt_queue_.TryPush(PacketItem::CreateFlushPacket(new_serial), true);

        StartInternalThreads();

        std::cout << "[Player] Restarting from EOF." << std::endl;
        return;
    }

    media_state_.Dispatch(Action::kUserPlay);

    // 如果底层线程还没启动，拉起线程
    if (!threads_running_) {
        StartInternalThreads();
    } else {
        video_decode_thread_.WakeUp();
    }
}

void Player::Pause() {
    media_state_.Dispatch(Action::kUserPause);
}

void Player::Stop() {
    media_state_.Dispatch(Action::kUserStop);

    if (threads_running_) {
        video_pkt_queue_.Abort();

        demux_thread_.Stop();
        video_decode_thread_.Stop();

        video_pkt_queue_.Restart();

        threads_running_ = false;
        std::cout << "[Player] Stopped completely." << std::endl;
    }
}

void Player::Seek(double target_sec) {
    media_state_.Seek(target_sec);

    if (!threads_running_)
        return;

    // 增加序列号
    int new_serial = ++video_serial_;

    video_pkt_queue_.Flush();

    demuxer_.Seek(target_sec);

    video_pkt_queue_.TryPush(PacketItem::CreateFlushPacket(new_serial), true);

    std::cout << "[Player] Seek dispatched to " << target_sec << "s, serial=" << new_serial << std::endl;
}

void Player::StartInternalThreads() {
    auto* fmt_ctx = demuxer_.format_context();
    if (!fmt_ctx)
        return;

    int v_stream_idx = demuxer_.video_stream_index();

    AVRational tb = fmt_ctx->streams[v_stream_idx]->time_base;

    // 绑定解码线程的渲染回调
    video_decode_thread_.SetFrameCallback([this](const FrameItem& frame) {
        if (on_frame_ready_)
            on_frame_ready_(frame);
    });

    // 启动解封装线程
    demux_thread_.Start(&demuxer_, &video_pkt_queue_, &media_state_, &video_serial_, on_finished_);

    // 启动解码线程
    video_decode_thread_.Start(&video_decoder_, &video_pkt_queue_, &media_state_, tb, &video_serial_, v_stream_idx);

    threads_running_ = true;
    std::cout << "[Player] Internal threads started." << std::endl;
}

} // namespace my_video_player