#ifndef MY_VIDEO_PLAYER_ENGINE_DECODE_VIDEO_DECODER_H_
#define MY_VIDEO_PLAYER_ENGINE_DECODE_VIDEO_DECODER_H_

#include "player/engine/demux/demuxer.h"
#include "player/engine/common/ffmpeg_raii.h"
#include "player/engine/common/engine_types.h"

namespace my_video_player {

class VideoDecoder {
public:
    VideoDecoder() = default;
    ~VideoDecoder() = default;

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    int Init(AVCodecParameters* params);

    int SendPacket(const PacketItem& packet);

    // 接收解码帧，需要传入 stream 的 time_base 来精确计算时间
    int ReceiveFrame(FrameItem* frame_item, AVRational time_base);

    void Flush();

private:
    AvCodecContextPtr codec_ctx_{nullptr};
};
} // namespace my_video_player
#endif // MY_VIDEO_PLAYER_ENGINE_DECODE_VIDEO_DECODER_H_