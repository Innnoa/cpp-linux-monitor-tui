#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "store/snapshot_store.h"

namespace {
monitor::model::SystemSnapshot make_snapshot(double cpu_percent, std::uint64_t memory_used) {
    monitor::model::SystemSnapshot snapshot;
    snapshot.captured_at = std::chrono::system_clock::time_point{std::chrono::seconds{cpu_percent > 20.0 ? 2 : 1}};
    snapshot.cpu.total_percent = cpu_percent;
    snapshot.memory.used_bytes = memory_used;
    return snapshot;
}
}  // namespace

TEST_CASE("snapshot store keeps the latest snapshot and bounded history") {
    monitor::store::SnapshotStore store(2);

    store.publish(make_snapshot(10.0, 1'000));
    store.publish(make_snapshot(20.0, 2'000));
    store.publish(make_snapshot(30.0, 3'000));

    const auto latest = store.latest();
    const auto cpu_history = store.cpu_history();
    const auto memory_history = store.memory_history();

    CHECK(latest.cpu.total_percent == 30.0);
    CHECK(cpu_history.size() == 2);
    CHECK(cpu_history.front() == 20.0);
    CHECK(cpu_history.back() == 30.0);
    CHECK(memory_history.back() == 3'000);
}
