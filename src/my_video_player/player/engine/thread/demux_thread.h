#ifndef MY_VIDEO_PLAYER_ENGINE_THREAD_DEMUX_THREAD_H_
#define MY_VIDEO_PLAYER_ENGINE_THREAD_DEMUX_THREAD_H_

#include <thread>
#include <atomic>
#include <functional>

namespace my_video_player {
class Demuxer;
class VideoPacketQueue;
class MediaState;

using FinishCallback = std::function<void()>;

class DemuxThread {
public:
    DemuxThread() = default;
    ~DemuxThread();

    DemuxThread(const DemuxThread&) = delete;
    DemuxThread& operator=(const DemuxThread&) = delete;

    /**
     * @brief 启动解封装线程
     * @param demuxer 解封装器实例（已 Open）
     * @param pkt_queue 视频包队列
     * @param state 全局状态机
     * @param serial 播放器维护的全局序列号指针（用于 Seek 时打戳）
     */
    void Start(Demuxer*, VideoPacketQueue*, MediaState*, std::atomic<int>*, FinishCallback);

    /**
     * @brief 停止线程
     * 注意：调用前，Player 应该先调用 pkt_queue_->Abort()
     * 以确保线程不会死锁在 Push 上。
     */
    void Stop();

private:
    void Run();

    Demuxer* demuxer_ = nullptr;
    VideoPacketQueue* pkt_queue_ = nullptr;
    MediaState* media_state_ = nullptr;
    std::atomic<int>* serial_ = nullptr;

    std::atomic<bool> running_{false};
    std::thread thread_;

    FinishCallback on_finished_;
};
} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_ENGINE_THREAD_DEMUX_THREAD_H_