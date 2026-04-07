#ifndef MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_
#define MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_

#include <string>

extern "C" {
#include <libavutil/error.h>
}

namespace my_video_player {
inline std::string AvErrorToString(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};

    if (av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE) == 0) {
        return std::string(errbuf);
    }

    return "Unknown FFmpeg error (" + std::to_string(errnum) + ")";
}

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_