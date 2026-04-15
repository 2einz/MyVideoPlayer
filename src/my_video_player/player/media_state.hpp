#ifndef MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_HPP_
#define MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_HPP_

#include <atomic>
#include <iostream>
#include <string>

namespace my_video_player {

enum class State {
    kIdle,     // 闲置 Idle 是“对象是否存在”的状态
    kLoading,  // 打开文件（解封装中）
    kPrepared, // 已准备好（获取到流信息）
    kPlaying,  // 播放中
    kPaused,   // 暂停
    kSeeking,  // 正在跳转
    kEnded,    // 播放结束
    kStopped,  // 已停止
    kError     // 发生错误 IO错误
};

enum class Action {
    kOpen,
    kPrepared,

    kUserPlay,
    kUserPause,
    kUserStop,
    kUserSeek,

    kReachedSeekTarget,

    kReachedEnd,
    kError
};

// 播放器状态中心
class MediaState {
    // 播放器主体状态
public:
    MediaState() : state_(State::kIdle), seek_requested_(false), seek_position_(0.0) {}

    MediaState(const MediaState&) = delete;
    MediaState& operator=(const MediaState&) = delete;

    // Query
    bool IsPlaying() const { return state_.load() == State::kPlaying; }
    bool IsPaused() const { return state_.load() == State::kPaused; }
    bool IsStopped() const { return state_.load() == State::kStopped; }
    bool HasError() const { return state_.load() == State::kError; }
    bool IsIdle() const { return state_.load() == State::kIdle; }
    bool IsSeeking() const { return state_.load() == State::kSeeking; }
    bool IsEnded() const { return state_.load() == State::kEnded; }
    bool IsPrepared() const { return state_.load() == State::kPrepared; }

    State GetState() const { return state_.load(); }

    // --- Action ---
    void Dispatch(Action action) {
        State old = state_.load();
        State next = Redux(old, action);

        if (next != old) {
            state_.store(next);
        } else {
            std::cerr << "[MediaState] Invalid transition:" << toString() << " action=" << static_cast<int>(action);
        }
    }

    void Seek(double pos) {
        // 保存目标位置
        seek_position_.store(pos);
        seek_requested_.store(true);

        // 用户发起seek
        Dispatch(Action::kUserSeek);
    }

    // Internal Logic
    bool HasSeekRequest() const { return seek_requested_.load(); }

    // 获取位置并重置请求状态
    double ConsumeSeekPosition() {
        seek_requested_.store(false);
        return seek_position_.load();
    }

    // DEBUG
    std::string toString() const {
        switch (state_.load()) {
        case State::kIdle:
            return "Idle";
        case State::kLoading:
            return "Loading";
        case State::kPlaying:
            return "Playing";
        case State::kPaused:
            return "Paused";
        case State::kStopped:
            return "Stopped";
        case State::kSeeking:
            return "Seeking";
        case State::kEnded:
            return "Ended";
        case State::kError:
            return "Error";
        default:
            return "Unknown";
        }
    }

private:
    // 状态机
    State Redux(State from, Action action) {
        switch (from) {
        case State::kIdle:
            if (action == Action::kOpen)
                return State::kLoading;
            break;

        case State::kLoading:
            if (action == Action::kPrepared)
                return State::kPrepared;
            if (action == Action::kError)
                return State::kError;
            break;

        case State::kPrepared:
            if (action == Action::kUserPlay)
                return State::kPlaying;
            if (action == Action::kUserStop)
                return State::kStopped;
            if (action == Action::kError)
                return State::kError;
            break;

        case State::kPlaying:
            if (action == Action::kUserPause)
                return State::kPaused;

            if (action == Action::kUserSeek)
                return State::kSeeking;

            if (action == Action::kReachedEnd)
                return State::kEnded;

            if (action == Action::kUserStop)
                return State::kStopped;

            if (action == Action::kError)
                return State::kError;

            break;

        case State::kPaused:
            if (action == Action::kUserPlay)
                return State::kPlaying;

            if (action == Action::kUserSeek)
                return State::kSeeking;

            if (action == Action::kUserStop)
                return State::kStopped;

            break;

        case State::kSeeking:

            if (action == Action::kReachedSeekTarget)
                return State::kPlaying;

            if (action == Action::kUserPause)
                return State::kPaused;

            if (action == Action::kUserStop)
                return State::kStopped;

            if (action == Action::kError)
                return State::kError;

            break;

        case State::kEnded:
            if (action == Action::kUserPlay)
                return State::kPlaying;

            if (action == Action::kUserStop)
                return State::kStopped;

            if (action == Action::kUserSeek)
                return State::kSeeking;

            break;

        case State::kStopped:
            if (action == Action::kOpen)
                return State::kLoading;

            break;

        case State::kError:
            if (action == Action::kUserStop)
                return State::kStopped;

            break;
        }

        return from; // 不允许非法跳转
    }
    // --- 控制请求 (Flags) ---
    bool seek_requested() const { return seek_requested_.load(); }

private:
    std::atomic<State> state_;

    std::atomic_bool seek_requested_;
    std::atomic<double> seek_position_;
};

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_HPP_