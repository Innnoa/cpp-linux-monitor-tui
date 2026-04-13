#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "model/system_snapshot.h"
#include "store/history_buffer.h"

namespace monitor::store {

class SnapshotStore {
  public:
    explicit SnapshotStore(std::size_t history_size);

    void publish(model::SystemSnapshot snapshot);
    [[nodiscard]] model::SystemSnapshot latest() const;
    [[nodiscard]] std::vector<double> cpu_history() const;
    [[nodiscard]] std::vector<std::uint64_t> memory_history() const;

  private:
    mutable std::mutex mutex_;
    model::SystemSnapshot latest_;
    HistoryBuffer<double> cpu_history_;
    HistoryBuffer<std::uint64_t> memory_history_;
};

}  // namespace monitor::store
