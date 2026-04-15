#ifndef MY_VIDEO_PLAYER_PLAYER_PLAYER_H_
#define MY_VIDEO_PLAYER_PLAYER_PLAYER_H_

#include <atomic>
#include <string>
#include <memory>
#include <functional>

#include "player/media_state.hpp"
#include "player/engine/common/engine_types.h"
#include "player/engine/decode/video_decoder.h"
#include "player/engine/demux/demuxer.h"
#include "player/engine/buffer/video_packet_queue.h"
#include "player/engine/thread/demux_thread.h"
#include "player/engine/thread/video_decode_thread.h"

namespace my_video_player {

class Player {
public:
    Player() = default;
    ~Player() = default;

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // 打开 (kLoading -> kPrepared)
    int Open(const std::string& url);

    void Play();
    void Pause();
    void Stop();
    void Seek(double target_sec);

    // Getter
    MediaState* GetMediaState() { return &media_state_; }
    const MediaState* GetMediaState() const { return &media_state_; }

    AVCodecParameters* GetVideoCodecParams();
    const AVCodecParameters* GetVideoCodecParams() const;

    double GetDuration() const;

    // 回调设置
    using FrameCallback = std::function<void(const FrameItem&)>;
    using FinishCallback = std::function<void()>;

    void SetFrameCallback(FrameCallback cb) { on_frame_ready_ = cb; }
    void SetFinishCallback(FinishCallback cb) { on_finished_ = std::move(cb); }

private:
    void StartInternalThreads(); // 仅在第一次调用

private:
    // 核心组件
    MediaState media_state_;
    Demuxer demuxer_;
    VideoDecoder video_decoder_;

    VideoPacketQueue video_pkt_queue_;
    DemuxThread demux_thread_;
    VideoDecodeThread video_decode_thread_;

    std::atomic<int> video_serial_{1};

    bool threads_running_{false};

    FrameCallback on_frame_ready_;
    FinishCallback on_finished_;
};
} // namespace my_video_player
#endif // MY_VIDEO_PLAYER_PLAYER_PLAYER_H_