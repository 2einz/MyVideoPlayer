#ifndef MY_VIDEO_PLAYER_PLAYER_PLAYER_H_
#define MY_VIDEO_PLAYER_PLAYER_PLAYER_H_

#include <string>
#include <memory>

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

    // Getter
    MediaState* GetMediaState() { return &media_state_; }
    const MediaState* GetMediaState() const { return &media_state_; }

    Demuxer& GetDemuxer() { return demuxer_; }
    const Demuxer& GetDemuxer() const { return demuxer_; }

    VideoDecoder& GetVideoDecoder() { return video_decoder_; }
    const VideoDecoder& GetVideoDecoder() const { return video_decoder_; }

private:
    // 核心组件
    MediaState media_state_;
    Demuxer demuxer_;
    VideoDecoder video_decoder_;
};
} // namespace my_video_player
#endif // MY_VIDEO_PLAYER_PLAYER_PLAYER_H_