#ifndef MY_VIDEO_PLAYER_ENGINE_DEMUX_DEMUXER_H_
#define MY_VIDEO_PLAYER_ENGINE_DEMUX_DEMUXER_H_

#include <string>
#include <vector>

#include "player/engine/common/ffmpeg_raii.h"

namespace my_video_player {

struct PacketItem {
    AvPacketPtr packet_;
    int stream_index_ = -1;
};

class Demuxer {
public:
    Demuxer() = default;
    ~Demuxer() = default;

    Demuxer(const Demuxer&) = delete;
    Demuxer& operator=(const Demuxer&) = delete;

    /**
     * 打开媒体文件并探测流信息。
     * @return 0 成功，负数为 FFmpeg 错误码。
     */
    int Open(const std::string& url);

    /**
     * 读取一个数据包并填充到传入的 item 中。
     * 采用复用模式：如果 packet_item->packet 已经分配，则仅重置缓冲区。
     */
    int ReadPacket(PacketItem* packet);

    // Getters
    int video_stream_index() const { return video_stream_index_; }
    int audio_stream_index() const { return audio_stream_index_; }
    AVFormatContext* format_context() const { return format_ctx_.get(); }

    AVCodecParameters* GetVideoCodecParameters() const;
    AVCodecParameters* GetAudioCodecParameters() const;

private:
    std::string url_;
    AvFormatContextPtr format_ctx_{nullptr};
    int video_stream_index_ = -1;
    int audio_stream_index_ = -1;
};

}; // namespace my_video_player

#endif // MY_VIDEO_PLAYER_ENGINE_DEMUX_DEMUXER_H_