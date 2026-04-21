#ifndef MY_VIDEO_PLAYER_ENGINE_COMMON_ENGINE_TYPES_H_
#define MY_VIDEO_PLAYER_ENGINE_COMMON_ENGINE_TYPES_H_

#include "player/engine/common/ffmpeg_raii.h"

namespace my_video_player {
struct PacketItem {
    AvPacketPtr packet;

    int stream_index = -1;

    double  pts = AV_NOPTS_VALUE;
    double  dts = AV_NOPTS_VALUE;
    double  duration = 0;

    int serial = 0;

    bool flush = false; // seek / reset时插入

    static PacketItem CreateFlushPacket(int serial) {
        PacketItem pkt;
        pkt.serial = serial;
        pkt.flush = true;
        return pkt;
    }
};

struct FrameItem {
    AvFramePtr frame;

    int stream_index = -1;

    double  pts = AV_NOPTS_VALUE;
    double  duration = 0;

    bool key_frame = false;

    int serial = 0;
};
} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_ENGINE_COMMON_ENGINE_TYPES_H_
