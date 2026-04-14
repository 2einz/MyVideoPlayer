#ifndef MY_VIDEO_PLAYER_ENGINE_CONVERT_VIDEO_CONVERTER_H_
#define MY_VIDEO_PLAYER_ENGINE_CONVERT_VIDEO_CONVERTER_H_

#include "player/engine/common/ffmpeg_raii.h"

namespace my_video_player {

class VideoConverter {
public:
    VideoConverter() = default;
    ~VideoConverter() = default;

    // 初始化转换上下文
    int Init(int src_w, int src_h, AVPixelFormat src_fmt, int dst_w, int dst_h, AVPixelFormat dst_fmt);

    // 执行转化

    int ConvertToFrame(const AVFrame* src, AVFrame* dst);

    int ConvertToBuffer(const AVFrame* src, uint8_t* dst, int linesize);

private:
    SwsContextPtr sws_ctx_{nullptr};
    int src_h_ = 0;
};

} // namespace my_video_player
#endif // MY_VIDEO_PLAYER_ENGINE_CONVERT_VIDEO_CONVERTER_H_