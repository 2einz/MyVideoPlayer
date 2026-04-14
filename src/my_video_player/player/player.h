#ifndef MY_VIDEO_PLAYER_PLAYER_PLAYER_H_
#define MY_VIDEO_PLAYER_PLAYER_PLAYER_H_

#include <atomic>
#include <string>
#include <memory>
#include <functional>

#include "player/engine/media_state.hpp"
#include "player/engine/decode/video_decoder.h"
#include "player/engine/demux/demuxer.h"

namespace my_video_player {

class Player {
public:
    Player();
    ~Player() = default;

    // 禁止拷贝
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // 打开并测试视频解码
    int Open(const std::string& url);

    void StartLoop(std::atomic<bool>& thread_running);

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
    // 核心组件
    MediaState media_state_;
    Demuxer demuxer_;
    VideoDecoder video_decoder_;

    FrameCallback on_frame_ready_;
    FinishCallback on_finished_;
};
} // namespace my_video_player
#endif // MY_VIDEO_PLAYER_PLAYER_PLAYER_H_