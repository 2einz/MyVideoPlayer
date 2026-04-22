#include "player/engine/demux/demuxer.h"

#include <iostream>

#include <log/my_spdlog.h>

namespace my_video_player {

int Demuxer::Open(const std::string& url) {
    url_ = url;

    format_ctx_ = OpenInput(url_.c_str());
    if (!format_ctx_)
        return AVERROR(EIO);

    // 读取数据流
    int ret = avformat_find_stream_info(format_ctx_.get(), nullptr);
    if (ret < 0) {
        LOG_ERROR(LM::kDemux, "Find Stream info failed: ", av_error2string(ret));
        return ret;
    }

    video_stream_index_ = av_find_best_stream(format_ctx_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    audio_stream_index_ = av_find_best_stream(format_ctx_.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (video_stream_index_ < 0 && audio_stream_index_ < 0) {
        LOG_ERROR(LM::kDemux, "No usable video/audio stream in: ", url_);
        return AVERROR_STREAM_NOT_FOUND;
    }

    // 打印格式
    av_dump_format(format_ctx_.get(), 0, url_.c_str(), 0);

    return 0;
}

int Demuxer::ReadPacket(PacketItem* packet_item) {
    if (!packet_item)
        return AVERROR(EINVAL);

    // 复用AVPacket
    if (!packet_item->packet) {
        packet_item->packet = MakeAvPacketPtr();
    } else {
        ResetPacket(packet_item->packet);
    }

    int ret = av_read_frame(format_ctx_.get(), packet_item->packet.get());
    if (ret >= 0) {
        packet_item->stream_index = packet_item->packet->stream_index;
    }

    return ret;
}

int Demuxer::Seek(double seconds) {
    if (!format_ctx_)
        return AVERROR(EINVAL);

    auto* stream = format_ctx_->streams[video_stream_index_];

    // 将秒转换为流的内部时间戳
    // AV_TIME_BASE_Q = av_make_q(1, AV_TIME_BASE)
    int64_t target_pts = av_rescale_q(static_cast<int64_t>(seconds * AV_TIME_BASE), AV_TIME_BASE_Q, stream->time_base);

    // 执行seek
    int ret =
        av_seek_frame(format_ctx_.get(), video_stream_index_, target_pts, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    if (ret < 0) {
        LOG_ERROR(LM::kDemux, "Seek failed: {}", av_error2string(ret));
        return ret;
    }

    // 如果没有这个在读到EOF内部状态还是EOF
    avformat_flush(format_ctx_.get());
    LOG_INFO(LM::kDemux, "Seek success to {} sec (pts={})", seconds, target_pts);

    return ret;
}

void Demuxer::Close() {
    format_ctx_.reset();
}

AVCodecParameters* Demuxer::GetVideoCodecParameters() const {
    if (video_stream_index_ < 0)
        return nullptr;
    return format_ctx_->streams[video_stream_index_]->codecpar;
}

AVCodecParameters* Demuxer::GetAudioCodecParameters() const {
    if (audio_stream_index_ < 0)
        return nullptr;
    return format_ctx_->streams[audio_stream_index_]->codecpar;
}
}; // namespace my_video_player