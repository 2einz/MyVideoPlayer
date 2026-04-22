#ifndef MY_VIDEO_PLAYER_ENGINE_COMMON_ENGINE_TYPES_H_
#define MY_VIDEO_PLAYER_ENGINE_COMMON_ENGINE_TYPES_H_

#include "player/engine/common/ffmpeg_raii.h"

namespace my_video_player {

// 定义一个更适合 double 的无效时间戳常量
const double kInvalidPts = -1.0;

struct PacketItem {
    AvPacketPtr packet;

    int stream_index = -1;

    // 建议默认值设为 kInvalidPts
    double pts = kInvalidPts;
    double dts = kInvalidPts;
    double duration = 0;

    int serial = 0;
    bool flush = false;

    PacketItem() = default;

    // 显式禁止拷贝（因为包含 unique_ptr）
    PacketItem(const PacketItem&) = delete;
    PacketItem& operator=(const PacketItem&) = delete;

    // 显式允许移动
    PacketItem(PacketItem&&) noexcept = default;
    PacketItem& operator=(PacketItem&&) noexcept = default;

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
    double pts = kInvalidPts; // 使用统一的无效常数
    double duration = 0;
    bool key_frame = false;
    int serial = 0;

    FrameItem() = default;

    // 禁止拷贝
    FrameItem(const FrameItem&) = delete;
    FrameItem& operator=(const FrameItem&) = delete;

    // 允许移动
    FrameItem(FrameItem&&) noexcept = default;
    FrameItem& operator=(FrameItem&&) noexcept = default;

    void assign_from(const FrameItem& other) {
        this->pts = other.pts;
        this->duration = other.duration;
        this->key_frame = other.key_frame;
        this->serial = other.serial;
        this->stream_index = other.stream_index;
        if (other.frame) {
            this->frame = MakeAvFramePtr(); // 分配外壳
            av_frame_ref(this->frame.get(), other.frame.get()); // 引用计数浅拷贝像素
        }
    }
};

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_ENGINE_COMMON_ENGINE_TYPES_H_