#pragma once

#include <condition_variable>
#include <limits>
#include <queue>

template<typename T>
class ConcurrentQueue
{
public:
  explicit ConcurrentQueue(size_t max_size = std::numeric_limits<size_t>::max()) : max_size_{max_size} {}

  ConcurrentQueue(ConcurrentQueue const&)            = delete;
  ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;

  T pop()
  {
    std::unique_lock lock(mutex_);
    is_not_empty_.wait(lock, [this]() { return !queue_.empty(); });

    T value = std::move(queue_.front());
    queue_.pop();

    lock.unlock();
    is_not_full_.notify_one();

    return value;
  }

  void push(T value)
  {
    std::unique_lock lock(mutex_);
    is_not_full_.wait(lock, [this]() { return queue_.size() < max_size_; });

    queue_.push(std::move(value));
    lock.unlock();
    is_not_empty_.notify_one();
  }

  size_t size() const
  {
    std::unique_lock lock(mutex_);
    return queue_.size();
  }

  bool empty() const
  {
    std::unique_lock lock(mutex_);
    return queue_.empty();
  }

private:
  size_t max_size_;

  mutable std::mutex      mutex_;
  std::condition_variable is_not_full_;
  std::condition_variable is_not_empty_;

  std::queue<T> queue_;
};
