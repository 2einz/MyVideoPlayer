#ifndef MY_VIDEO_PLAYER_UTILS_THREAD_SAFE_QUEUE_HPP_
#define MY_VIDEO_PLAYER_UTILS_THREAD_SAFE_QUEUE_HPP_

#include <cassert>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace my_video_player {
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue(size_t max_size = 256) : max_size_(max_size), state_(State::kRunning) {}
    ~ThreadSafeQueue() { Abort(); }

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void Abort() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_ = State::kAborted;
        }
        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    void Restart() {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(state_ == State::kAborted && "Restart must be called after Abort and Thread Join!");
        state_ = State::kRunning;
        std::queue<T> empty_queue;
        std::swap(queue_, empty_queue);

        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    void Close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_ = State::kDraining;
        }
        not_full_cv_.notify_all();
    }

    /**
     * @brief 阻塞式插入
     */
    bool Push(T&& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_cv_.wait(lock, [this]() { return queue_.size() < max_size_ || state_ != State::kRunning; });

        if (state_ != State::kRunning)
            return false;

        bool was_empty = queue_.empty();
        queue_.push(std::move(value));

        if (was_empty) {
            not_empty_cv_.notify_one();
        }
        return true;
    }

    bool Push(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_cv_.wait(lock, [this]() { return queue_.size() < max_size_ || state_ != State::kRunning; });

        if (state_ != State::kRunning)
            return false;

        bool was_empty = queue_.empty();
        queue_.push(value);

        if (was_empty) {
            not_empty_cv_.notify_one();
        }
        return true;
    }

    /**
     * @brief 非阻塞尝试插入 (带丢帧策略)
     * @param drop_old_if_full 当队列满时，是否丢弃最老的数据以腾出空间。
     *                         true: 用于视频帧队列（防止延迟累积，必须追上最新画面）
     *                         false: 用于音频包队列（音频不能丢，丢了会爆音，直接返回 false）
     */
    bool TryPush(T&& value, bool drop_old_if_full = false) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != State::kRunning)
            return false;

        if (queue_.size() >= max_size_) {
            if (!drop_old_if_full)
                return false;
            queue_.pop();
        }

        bool was_empty = queue_.empty();
        queue_.push(std::move(value));

        if (was_empty) {
            not_empty_cv_.notify_one();
        }
        return true;
    }

    /**
     * @brief 阻塞式弹出
     * @return true 成功获取数据; false 队列被强制终止，应该立刻退出线程。
     */
    bool Pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_cv_.wait(lock, [this]() { return !queue_.empty() || state_ != State::kRunning; });

        // Abort State
        if (state_ == State::kAborted) {
            return false;
        }

        // Draining State 消费完剩下的再推出
        if (state_ == State::kDraining && queue_.empty()) {
            return false;
        }

        bool was_full = (queue_.size() == max_size_);
        value = std::move(queue_.front());
        queue_.pop();

        if (was_full) {
            not_full_cv_.notify_one();
        }

        return true;
    }

    bool TryPop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
            return false;

        bool was_full = (queue_.size() == max_size_);
        value = std::move(queue_.front());
        queue_.pop();

        if (was_full) {
            not_full_cv_.notify_one();
        }
        return true;
    }

    void Flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        bool was_full = (queue_.size() == max_size_);

        std::queue<T> empty_queue;
        std::swap(queue_, empty_queue);

        not_empty_cv_.notify_all();
        if (was_full) {
            not_full_cv_.notify_all();
        }
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    enum class State {
        kRunning,  // 正常运行
        kDraining, // 排空中，不再接受新包
        kAborted   // 强制终止 Seek或Stop
    };
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_cv_;
    std::condition_variable not_full_cv_;
    size_t max_size_;

    State state_;
};

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_UTILS_THREAD_SAFE_QUEUE_HPP_