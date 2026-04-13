#pragma once

#include <cstddef>
#include <deque>

namespace monitor::store {

template <typename T>
class HistoryBuffer {
  public:
    explicit HistoryBuffer(std::size_t capacity) : capacity_(capacity) {}

    void push(T value) {
        if (capacity_ == 0) {
            return;
        }
        if (values_.size() == capacity_) {
            values_.pop_front();
        }
        values_.push_back(value);
    }

    [[nodiscard]] const std::deque<T>& values() const { return values_; }

  private:
    std::size_t capacity_{0};
    std::deque<T> values_;
};

}  // namespace monitor::store
