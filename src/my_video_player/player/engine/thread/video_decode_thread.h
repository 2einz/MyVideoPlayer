#ifndef MY_VIDEO_PLAYER_ENGINE_THREAD_VIDEO_DECODE_THREAD_H_
#define MY_VIDEO_PLAYER_ENGINE_THREAD_VIDEO_DECODE_THREAD_H_

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

extern "C" {
#include <libavutil/rational.h>
}

namespace my_video_player {

class VideoDecoder;
class VideoPacketQueue;
class MediaState;
struct FrameItem;
class VideoDecodeThread {
public:
    using FrameCallback = std::function<void(const FrameItem&)>;

    VideoDecodeThread() = default;
    ~VideoDecodeThread();

    VideoDecodeThread(const VideoDecodeThread&) = delete;
    VideoDecodeThread& operator=(const VideoDecodeThread&) = delete;

    /**
     * @brief 启动视频解码线程
     * @param decoder 视频解码器实例
     * @param pkt_queue 视频包队列
     * @param state 全局状态机
     * @param time_base 视频流的时间基
     * @param serial 全局序列号指针
     * @param stream_index 当前处理的视频流索引
     */
    void start(VideoDecoder* decoder,
               VideoPacketQueue* pkt_queue,
               MediaState* state,
               AVRational time_base,
               std::atomic<int>* serial,
               int stream_index);

    /**
     * @brief 唤醒因暂停而休眠的解码线程
     * 必须在 Player::Play() 时显式调用
     */
    void wake_up();

    void stop();

    void set_frame_callback(FrameCallback cb) { on_frame_ready_ = std::move(cb); }

private:
    void run();

private:
    VideoDecoder* decoder_ = nullptr;
    VideoPacketQueue* pkt_queue_ = nullptr;
    MediaState* media_state_ = nullptr;
    std::atomic<int>* current_serial_ = nullptr;

    AVRational time_base_;
    int video_stream_index_ = -1;

    FrameCallback on_frame_ready_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    std::mutex pause_mutex_;
    std::condition_variable pause_cv_;
};
} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_ENGINE_THREAD_VIDEO_DECODE_THREAD_H_