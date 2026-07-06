#pragma once
#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t capacity) : buffer_(capacity), capacity_(capacity) {}

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_not_full_.wait(lock, [this] { return count_ < capacity_ || closed_; });
        if (closed_) return false;
        buffer_[tail_] = std::move(item);
        tail_ = (tail_ + 1) % capacity_;
        ++count_;
        lock.unlock();
        cv_not_empty_.notify_one();
        return true;
    }

    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_not_empty_.wait(lock, [this] { return count_ > 0 || closed_; });
        if (count_ == 0) return false;
        out = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;
        lock.unlock();
        cv_not_full_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        cv_not_full_.notify_all();
        cv_not_empty_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
    std::vector<T> buffer_; 
    size_t capacity_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t count_ = 0;
    bool closed_ = false;
};
