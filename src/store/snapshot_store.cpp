#include "store/snapshot_store.h"

namespace monitor::store {

SnapshotStore::SnapshotStore(std::size_t history_size)
    : cpu_history_(history_size), memory_history_(history_size) {}

void SnapshotStore::publish(model::SystemSnapshot snapshot) {
    std::scoped_lock lock(mutex_);
    latest_ = std::move(snapshot);
    cpu_history_.push(latest_.cpu.total_percent);
    memory_history_.push(latest_.memory.used_bytes);
}

model::SystemSnapshot SnapshotStore::latest() const {
    std::scoped_lock lock(mutex_);
    return latest_;
}

std::vector<double> SnapshotStore::cpu_history() const {
    std::scoped_lock lock(mutex_);
    return {cpu_history_.values().begin(), cpu_history_.values().end()};
}

std::vector<std::uint64_t> SnapshotStore::memory_history() const {
    std::scoped_lock lock(mutex_);
    return {memory_history_.values().begin(), memory_history_.values().end()};
}

}  // namespace monitor::store
