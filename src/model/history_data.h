#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace monitor::model {

template<typename T, std::size_t N>
class RingBuffer {
public:
    static_assert(N > 0, "RingBuffer capacity must be greater than 0");

    void push(T value) {
        buffer_[head_] = value;
        head_ = (head_ + 1) % N;
        if (size_ < N) {
            ++size_;
        }
    }

    std::span<const T> data() const {
        if (size_ < N) {
            return std::span<const T>(buffer_.data(), size_);
        }
        for (std::size_t i = 0; i < N; ++i) {
            ordered_[i] = buffer_[(head_ + i) % N];
        }
        return std::span<const T>(ordered_.data(), N);
    }

    std::size_t size() const {
        return size_;
    }

    bool full() const {
        return size_ == N;
    }

    void clear() {
        head_ = 0;
        size_ = 0;
        ordered_ = {};
    }

    T latest() const {
        if (size_ == 0) {
            return T{};
        }
        std::size_t index = (head_ == 0) ? (N - 1) : (head_ - 1);
        return buffer_[index];
    }

private:
    std::array<T, N> buffer_{};
    mutable std::array<T, N> ordered_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

struct HistoryData {
    RingBuffer<double, 60> cpu_history;
    RingBuffer<double, 60> memory_history;
    RingBuffer<double, 60> disk_read_history;
    RingBuffer<double, 60> disk_write_history;
    RingBuffer<double, 60> network_rx_history;
    RingBuffer<double, 60> network_tx_history;

    void clear() {
        cpu_history.clear();
        memory_history.clear();
        disk_read_history.clear();
        disk_write_history.clear();
        network_rx_history.clear();
        network_tx_history.clear();
    }
};

}  // namespace monitor::model
