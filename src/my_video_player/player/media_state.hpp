#ifndef MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_HPP_
#define MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_HPP_

#include <atomic>
#include <iostream>
#include <string>

#include "log/my_spdlog.h"

namespace my_video_player {

#define STATE_LIST(X)                                                                                                  \
    X(kIdle)                                                                                                           \
    X(kLoading)                                                                                                        \
    X(kPrepared)                                                                                                       \
    X(kPlaying)                                                                                                        \
    X(kPaused)                                                                                                         \
    X(kSeeking)                                                                                                        \
    X(kEnded)                                                                                                          \
    X(kStopped)                                                                                                        \
    X(kError)

enum class State {
#define X(name) name,
    STATE_LIST(X)
#undef X
};

#define ACTION_LIST(X)                                                                                                 \
    X(kOpen)                                                                                                           \
    X(kPrepared)                                                                                                       \
    X(kUserPlay)                                                                                                       \
    X(kUserPause)                                                                                                      \
    X(kUserStop)                                                                                                       \
    X(kUserSeek)                                                                                                       \
    X(kReachedSeekTarget)                                                                                              \
    X(kReachedEnd)                                                                                                     \
    X(kError)

enum class Action {
#define X(name) name,
    ACTION_LIST(X)
#undef X
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

        // 更新用户意图
        CaptureIntent(old, action);

        State next = Redux(old, action);

        if (next != old) {
            state_.store(next);
        } else {
            LOG_WARN(LM::kMediaState, "Invalid transition: {} action to {} action", state2string(),
                     action2string(action));
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
    std::string state2string() const {
        switch (state_.load()) {
#define X(name)                                                                                                        \
    case State::name:                                                                                                  \
        return #name + 1;
            STATE_LIST(X)
#undef X
        default:
            return "UnknownState";
        }
    }

    std::string action2string(Action action) {
        switch (action) {
#define X(name)                                                                                                        \
    case Action::name:                                                                                                 \
        return #name + 1;
            ACTION_LIST(X)
#undef X
        default:
            return "UnknownAction";
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
            if (action == Action::kUserSeek)
                return State::kSeeking;

            if (action == Action::kReachedSeekTarget)
                return play_intent_.load() ? State::kPlaying : State::kPaused;

            if (action == Action::kReachedEnd)
                return State::kEnded;

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

            if (action == Action::kUserSeek)
                return State::kSeeking;
            
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

    void CaptureIntent(State current, Action action) {
        switch (action) {
        case Action::kUserPlay:
        case Action::kOpen:
            // 打开文件和点播放，都视为“希望播放”
            play_intent_.store(true);
            break;

        case Action::kUserPause:
        case Action::kUserStop:
        case Action::kError:
            // 暂停、停止、错误，都视为“不再播放”
            play_intent_.store(false);
            break;

        case Action::kUserSeek:
            // 核心逻辑：Seek时的意图取决于发起Seek的那一刻
            // 如果是从播放状态点的，Seek完继续播；如果是从暂停点的，Seek完保持停。
            if (current == State::kPlaying) {
                play_intent_.store(true);
            }
            else if (current == State::kPaused) {
                play_intent_.store(false);
            }
            // 如果当前已经是 kSeeking 状态（连续滑动进度条），则不更新意图，保持上一次的判断
            break;

        default:
            // kPrepared, kReachedSeekTarget, kReachedEnd 等内部动作不改变用户意图
            break;
        }
    }
private:
    std::atomic<State> state_;

    std::atomic_bool seek_requested_;
    std::atomic<double> seek_position_;

    std::atomic<bool> play_intent_{false}; // 用户的播放意图 true表示用户希望播放
};

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_PLAYER_MEDIA_STATE_HPP_