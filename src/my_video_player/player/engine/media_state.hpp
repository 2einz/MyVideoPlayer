#ifndef MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_H_
#define MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_H_

#include <atomic>
#include <string>

namespace my_video_player {

// 播放器主体状态
enum class PlayerState {
    kIdle,     // 闲置
    kLoading,  // 打开文件（解封装中）
    kPrepared, // 已准备好（获取到流信息）
    kPlaying,  // 播放中
    kPaused,   // 暂停
    kSeeking,  // 正在跳转
    kEnded,    // 播放结束
    kStopped,  // 已停止
    kError     // 发生错误
};

// 缓冲池状态
enum class BufferState {
    kEmpty,     // 完全没数据
    kBuffering, // 正在缓冲
    kReady      // 数据充足，可以播放
};

// 播放器状态中心
class MediaState {
public:
    MediaState()
        : state_(PlayerState::kIdle), buffer_state_(BufferState::kEmpty), stop_requested_(false),
          pause_requested_(false), seek_requested_(false), seek_position_(0.0) {}

    MediaState(const MediaState&) = delete;
    MediaState& operator=(const MediaState&) = delete;

    void set_state(PlayerState state) { state_.store(state); }
    PlayerState state() const { return state_.load(); }

    bool is_playing() const { return state_.load() == PlayerState::kPlaying; }
    bool is_paused() const { return state_.load() == PlayerState::kPaused; }
    bool is_stopped() const { return state_.load() == PlayerState::kStopped; }
    bool has_error() const { return state_.load() == PlayerState::kError; }

    void set_buffer_state(BufferState state) { buffer_state_.store(state); }
    BufferState buffer_state() const { return buffer_state_.load(); }
    bool is_buffering() const { return buffer_state_.load() == BufferState::kBuffering; }

    // --- 控制请求 (Flags) ---
    void set_stop_requested(bool stop) { stop_requested_.store(stop); }
    bool stop_requested() const { return stop_requested_.load(); }

    void set_pause_requested(bool pause) { pause_requested_.store(pause); }
    bool pause_requested() const { return pause_requested_.load(); }

    // --- 动作函数 ---
    void RequestSeek(double position) {
        seek_position_.store(position);
        seek_requested_.store(true);
    }

    bool seek_requested() const { return seek_requested_.load(); }

    // 获取位置并重置请求状态
    double ConsumeSeekPosition() {
        seek_requested_.store(false);
        return seek_position_.load();
    }

    // 测试
    std::string toString() const {
        switch (state_.load()) {
        case PlayerState::kIdle:
            return "Idle";
        case PlayerState::kLoading:
            return "Loading";
        case PlayerState::kPlaying:
            return "Playing";
        case PlayerState::kPaused:
            return "Paused";
        case PlayerState::kStopped:
            return "Stopped";
        case PlayerState::kSeeking:
            return "Seeking";
        case PlayerState::kEnded:
            return "Ended";
        case PlayerState::kError:
            return "Error";
        default:
            return "Unknown";
        }
    }

private:
    std::atomic<PlayerState> state_;
    std::atomic<BufferState> buffer_state_;

    std::atomic_bool stop_requested_;
    std::atomic_bool pause_requested_;

    std::atomic_bool seek_requested_;
    std::atomic<double> seek_position_;
};

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_H_