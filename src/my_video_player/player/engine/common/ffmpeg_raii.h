#ifndef MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_FFMPEG_RAII_H_
#define MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_FFMPEG_RAII_H_

#include <iostream>
#include <memory>

#include "player/engine/common/error_util.h"
#include "log/my_spdlog.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace my_video_player {

struct AVFrameDeleter {
    void operator()(AVFrame* frame) const {
        if (frame) {
            av_frame_free(&frame);
        }
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* pkt) const {
        if (pkt) {
            av_packet_free(&pkt);
        }
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx) {
            avcodec_free_context(&ctx);
        }
    }
};

struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (ctx) {
            avformat_close_input(&ctx);
        }
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ctx) const {
        if (ctx) {
            sws_freeContext(ctx);
        }
    }
};

// 智能指针
using AvFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;

using AvPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;

using AvCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;

using AvFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;

using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;

inline void ResetPacket(AvPacketPtr& pkt) {
    if (!pkt)
        return;
    av_packet_unref(pkt.get());
}

inline void ResetFrame(AvFramePtr& frame) {
    if (!frame)
        return;
    av_frame_unref(frame.get());
}

inline AvFramePtr MakeAvFramePtr() {
    return AvFramePtr(av_frame_alloc());
}

inline AvPacketPtr MakeAvPacketPtr() {
    return AvPacketPtr(av_packet_alloc());
}

inline AvFormatContextPtr OpenInput(const char* url) {
    AVFormatContext* ctx = nullptr;

    int ret = avformat_open_input(&ctx, url, nullptr, nullptr);

    if (ret < 0) {
        
        LOG_ERROR(LM::kFFmpeg, "Could not open input: {} , reason is: {}", url, av_error2string(ret));
        return AvFormatContextPtr(nullptr);
    }

    return AvFormatContextPtr(ctx);
}

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_FFMPEG_RAII_H_