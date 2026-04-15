#ifndef MY_VIDEO_PLAYER_ENGINE_BUFFER_VIDEO_PACKET_QUEUE_H_
#define MY_VIDEO_PLAYER_ENGINE_BUFFER_VIDEO_PACKET_QUEUE_H_

#include "player/engine/utils/thread_safe_queue.hpp"
#include "player/engine/common/engine_types.h" // 包含 PacketItem 定义

namespace my_video_player {

// 视频包队列：限制最多缓存 120 个包（约 5~8 秒视频），防止内存飙升
class VideoPacketQueue : public ThreadSafeQueue<PacketItem> {
public:
    explicit VideoPacketQueue(size_t max_size = 120) : ThreadSafeQueue<PacketItem>(max_size) {}
};

} // namespace my_video_player
#endif
