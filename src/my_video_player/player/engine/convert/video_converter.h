#ifndef MY_VIDEO_PLAYER_ENGINE_CONVERT_VIDEO_CONVERTER_H_
#define MY_VIDEO_PLAYER_ENGINE_CONVERT_VIDEO_CONVERTER_H_

#include "player/engine/common/ffmpeg_raii.h"

namespace my_video_player {

class VideoConverter {
public:
    VideoConverter() = default;
    ~VideoConverter() = default;

    // 初始化转换上下文
    int init(int src_w, int src_h, AVPixelFormat src_fmt, int dst_w, int dst_h, AVPixelFormat dst_fmt);

    // 执行转化
    int convert2frame(const AVFrame* src, AVFrame* dst);

    int convert2buffer(const AVFrame* src, uint8_t* dst, int linesize);

private:
    SwsContextPtr sws_ctx_{nullptr};

    int src_h_ = 0;
    int dst_w_ = 0;
    int dst_h_ = 0;
    AVPixelFormat dst_fmt_ = AV_PIX_FMT_NONE;
};

} // namespace my_video_player
#endif // MY_VIDEO_PLAYER_ENGINE_CONVERT_VIDEO_CONVERTER_H_