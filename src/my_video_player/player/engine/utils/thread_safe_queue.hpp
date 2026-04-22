#ifndef MY_VIDEO_PLAYER_UTILS_THREAD_SAFE_QUEUE_HPP_
#define MY_VIDEO_PLAYER_UTILS_THREAD_SAFE_QUEUE_HPP_

#include <cassert>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace my_video_player {

/*
     running: 正常
     eof: 停止生产 + 消费完再退出
     abort: 立即停止一切
 */

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue(size_t max_size = 256) : max_size_(max_size), aborted_(false) {}

    ~ThreadSafeQueue() { abort(); }

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void abort() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            aborted_ = true;
        }
        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);

        aborted_ = false;
        eof_ = false;

        std::queue<T> empty_queue;
        std::swap(queue_, empty_queue);

        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    /**
     * @brief 阻塞式插入
     */
    bool push(T&& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_cv_.wait(lock, [this]() { return queue_.size() < max_size_ || aborted_ || eof_; });

        if (aborted_ || eof_)
            return false;

        bool was_empty = queue_.empty();
        queue_.push(std::move(value));

        lock.unlock(); // 提前释放锁

        if (was_empty) {
            not_empty_cv_.notify_one();
        }
        return true;
    }

    bool push(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_cv_.wait(lock, [this]() { return queue_.size() < max_size_ || aborted_ || eof_; });

        if (aborted_ || eof_)
            return false;

        bool was_empty = queue_.empty();
        queue_.push(value);

        lock.unlock();

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
    bool try_push(T&& value, bool drop_old_if_full = false) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (aborted_ || eof_)
            return false;

        if (queue_.size() >= max_size_) {
            if (!drop_old_if_full)
                return false;
            queue_.pop(); // 丢弃最旧
        }
        bool was_empty = queue_.empty();
        queue_.push(std::move(value));

        lock.unlock();

        if (was_empty) {
            not_empty_cv_.notify_one();
        }
        return true;
    }

    /**
     * @brief 阻塞式弹出
     * @return true 成功获取数据; false 队列被强制终止，应该立刻退出线程。
     */
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_cv_.wait(lock, [this]() { return !queue_.empty() || aborted_ || eof_; });

        if (aborted_) {
            return false;
        }

        // Draining State 消费完剩下的再退出
        if (eof_ && queue_.empty()) {
            return false;
        }

        value = std::move(queue_.front());
        queue_.pop();

        lock.unlock();

        not_full_cv_.notify_one();
        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
            return false;

        value = std::move(queue_.front());
        queue_.pop();
        not_full_cv_.notify_one();

        return true;
    }

    void flush() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::queue<T> empty_queue;
        std::swap(queue_, empty_queue);

        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void set_eof() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            eof_ = true;
        }
        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_cv_; // 消费者
    std::condition_variable not_full_cv_;  // 生产者
    size_t max_size_;

    bool aborted_ = false;
    bool eof_ = false;
};

} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_UTILS_THREAD_SAFE_QUEUE_HPP_