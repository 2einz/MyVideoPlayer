#ifndef MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_
#define MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_

#include <string>

#include "log/my_spdlog.h"
extern "C" {
#include <libavutil/error.h>
#include <libavutil/log.h>
}

namespace my_video_player {
inline std::string av_error2string(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};

    if (av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE) == 0) {
        return std::string(errbuf);
    }

    return "Unknown FFmpeg error (" + std::to_string(errnum) + ")";
}

inline void ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vl) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, vl);

    std::string msg(buffer);
    if (!msg.empty() && msg.back() == '\n') {
        msg.pop_back();
    }

    // 映射到你的日志系统
    LOG_INFO(LM::kFFmpeg, "{}", msg);
}
} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_