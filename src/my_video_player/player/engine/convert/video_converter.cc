#include "player/engine/convert/video_converter.h"

namespace my_video_player {

int VideoConverter::Init(int src_w, int src_h, AVPixelFormat src_fmt, int dst_w, int dst_h, AVPixelFormat dst_fmt) {
    src_h_ = src_h;

    // 分配SwsContext
    sws_ctx_ = SwsContextPtr(
        sws_getContext(src_w, src_h, src_fmt, dst_w, dst_h, dst_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr));

    return sws_ctx_ ? 0 : -1;
}

int VideoConverter::ConvertToFrame(const AVFrame* src, AVFrame* dst) {
    if (!sws_ctx_ || !src || !dst)
        return -1;

    int ret = av_frame_make_writable(dst);
    if (ret < 0)
        return ret;

    return sws_scale(sws_ctx_.get(), src->data, src->linesize, 0, src_h_, dst->data, dst->linesize);
}

int VideoConverter::ConvertToBuffer(const AVFrame* src, uint8_t* dst, int linesize) {
    if (!sws_ctx_ || !src || !dst)
        return -1;

    // sws_scale 需要指针数组，我们构造一个临时的
    uint8_t* dst_data[4] = {dst, nullptr, nullptr, nullptr};
    int dst_linesize[4] = {linesize, 0, 0, 0};

    return sws_scale(sws_ctx_.get(), src->data, src->linesize, 0, src_h_, dst_data, dst_linesize);
}
} // namespace my_video_player