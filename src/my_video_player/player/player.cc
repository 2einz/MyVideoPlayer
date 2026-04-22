#include "player/player.h"

#include <iostream>

#include <log/my_spdlog.h>

extern "C" {
#include <libavutil/rational.h>
}

namespace my_video_player {

int Player::Open(const std::string& url) {
    LOG_INFO(LM::kPlayer, "Attempting to open: {}", url);

    last_url_ = url;

    media_state_.Dispatch(Action::kOpen);

    OpenFile(url);

    InitDecoder();

    LOG_INFO(LM::kPlayer, "Open success, current state is prepared.");

    return 0;
}

int Player::InitDecoder() {
    // 初始化视频解码器
    AVCodecParameters* v_params = demuxer_.GetVideoCodecParameters();
    if (!v_params) {
        LOG_ERROR(LM::kPlayer, "Could not find video codec parameters.");
        media_state_.Dispatch(Action::kError);
        return -1;
    }

    int ret = video_decoder_.Init(v_params);
    if (ret < 0) {
        LOG_ERROR(LM::kPlayer, "Video decoder init failed.");
        media_state_.Dispatch(Action::kError);
        return ret;
    }

    return ret;
}
int Player::OpenFile(const std::string& url) {
    // 打开文件
    int ret = demuxer_.Open(url);
    if (ret < 0) {
        LOG_ERROR(LM::kPlayer, "Demuxer open failed: ");
        media_state_.Dispatch(Action::kError);
        return -1;
    }

    media_state_.Dispatch(Action::kPrepared);

    return ret;
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
        LOG_INFO(LM::kPlayer, "Trying to restart from EOF...");

        PrepareForRestart(0.0);

        StartInternalThreads();

        LOG_INFO(LM::kPlayer, "Restarting from EOF successfully.");
        return;
    }

    // 正常流程
    media_state_.Dispatch(Action::kUserPlay);

    // 如果底层线程还没启动，拉起线程
    if (!threads_running_) {
        StartInternalThreads();
        LOG_INFO(LM::kPlayer, "Function StartInternalThreads is Finish");
    } else {
        video_decode_thread_.wake_up();
    }
}

void Player::Pause() {
    media_state_.Dispatch(Action::kUserPause);
}

void Player::Stop() {
    media_state_.Dispatch(Action::kUserStop);

    if (threads_running_) {
        // 停读包，再停解码
        demux_thread_.stop();
        video_decode_thread_.stop();

        video_pkt_queue_.abort();
        video_pkt_queue_.reset();

        threads_running_ = false;
        LOG_INFO(LM::kPlayer, "Stopped completely.");
    }
}

void Player::Seek(double target_sec) {
    bool engine_dead = !demux_thread_.is_running() || !video_decode_thread_.is_running();

    // Case A: 视频结束，或者线程没有跑
    if (media_state_.IsEnded() || engine_dead) {
        LOG_INFO(LM::kPlayer, "Engine is inactive. Restarting for Seek at {}s...", target_sec);

        PrepareForRestart(target_sec);
        media_state_.Dispatch(Action::kUserPlay);
        media_state_.Seek(target_sec);

        StartInternalThreads();
        return;
    }

    // Case B: 正常运行
    int new_serial = ++video_serial_;

    video_pkt_queue_.flush();

    demuxer_.Seek(target_sec);

    video_pkt_queue_.try_push(PacketItem::CreateFlushPacket(new_serial), true);

    media_state_.Seek(target_sec);
    video_decode_thread_.wake_up(); // 无论是否暂停，都需要唤醒解码线程去处理Flush包和数据

    LOG_INFO(LM::kPlayer, "Seek dispatched to {} s, serial= {}", target_sec, new_serial);
}

void Player::StartInternalThreads() {
    auto* fmt_ctx = demuxer_.format_context();
    if (!fmt_ctx)
        return;

    int v_stream_idx = demuxer_.video_stream_index();

    AVRational tb = fmt_ctx->streams[v_stream_idx]->time_base;

    // 绑定解码线程的渲染回调
    video_decode_thread_.set_frame_callback([this](const FrameItem& frame) {
        if (on_frame_ready_)
            on_frame_ready_(frame);
    });

    // 启动解封装线程
    demux_thread_.Start(&demuxer_, &video_pkt_queue_, &media_state_, &video_serial_, on_finished_);

    // 启动解码线程
    video_decode_thread_.start(&video_decoder_, &video_pkt_queue_, &media_state_, tb, &video_serial_, v_stream_idx);

    threads_running_ = true;
    LOG_INFO(LM::kPlayer, "Internal threads started.");
}
void Player::PrepareForRestart(double seek_target) {
    Stop();

    video_pkt_queue_.reset();

    OpenFile(last_url_);
    InitDecoder();

    if (demuxer_.Seek(seek_target) < 0) {
        LOG_ERROR(LM::kPlayer, "Seek failed during restart.");
    }

    int new_serial = ++video_serial_;

    media_state_.Seek(seek_target);

    video_pkt_queue_.try_push(PacketItem::CreateFlushPacket(new_serial), true);
}

} // namespace my_video_player