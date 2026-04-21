#include "player/engine/convert/video_converter.h"

namespace my_video_player {

int VideoConverter::init(int src_w, int src_h, AVPixelFormat src_fmt, int dst_w, int dst_h, AVPixelFormat dst_fmt) {
    src_h_ = src_h;
    dst_w_ = dst_w;
    dst_h_ = dst_h;
    dst_fmt_ = dst_fmt;

    // 释放智能指针的管理权，给FFmpeg缓存管理
    struct SwsContext* raw_ctx = sws_ctx_.release();

    // 使用 sws_getCachedContext 完美处理中途分辨率或格式变化
    raw_ctx = sws_getCachedContext(
        raw_ctx,
        src_w, src_h, src_fmt,
        dst_w, dst_h, dst_fmt,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
    );

    // 分配SwsContext
    sws_ctx_.reset(raw_ctx);

    return sws_ctx_ ? 0 : -1;
}

int VideoConverter::convert2frame(const AVFrame* src, AVFrame* dst) {
    if (!sws_ctx_ || !src || !dst)
        return -1;

    if (!dst->data[0]) {
        dst->format = dst_fmt_;
        dst->width = dst_w_;
        dst->height = dst_h_;
        int ret = av_frame_get_buffer(dst, 32); // 32字节对齐，迎合 SIMD
        if (ret < 0) return ret;
    }
    else {
        // 如果外部已经分配过，仅仅检查是否可写
        int ret = av_frame_make_writable(dst);
        if (ret < 0) return ret;
    }

    return sws_scale(sws_ctx_.get(), src->data, src->linesize, 0, src_h_, dst->data, dst->linesize);
}

int VideoConverter::convert2buffer(const AVFrame* src, uint8_t* dst, int linesize) {
    if (!sws_ctx_ || !src || !dst)
        return -1;

    // sws_scale 需要指针数组，我们构造一个临时的
    uint8_t* dst_data[4] = {dst, nullptr, nullptr, nullptr};
    int dst_linesize[4] = {linesize, 0, 0, 0};

    return sws_scale(sws_ctx_.get(), src->data, src->linesize, 0, src_h_, dst_data, dst_linesize);
}
} // namespace my_video_player