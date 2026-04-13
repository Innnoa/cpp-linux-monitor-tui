#include <catch2/catch_test_macros.hpp>

#include "app/sampling_worker.h"
#include "model/system_snapshot.h"
#include "store/snapshot_store.h"

namespace {
class FakeSampler final : public monitor::app::Sampler {
  public:
    monitor::model::SystemSnapshot collect() override {
        monitor::model::SystemSnapshot snapshot;
        snapshot.cpu.total_percent = 37.0;
        snapshot.memory.used_bytes = 31ULL * 1024ULL * 1024ULL;
        return snapshot;
    }
};
}  // namespace

TEST_CASE("sampling worker publishes snapshots into the store") {
    FakeSampler sampler;
    monitor::store::SnapshotStore store(4);
    monitor::app::SamplingWorker worker(sampler, store);

    worker.tick_once();

    const auto latest = store.latest();
    CHECK(latest.cpu.total_percent == 37.0);
    CHECK(latest.memory.used_bytes == 31ULL * 1024ULL * 1024ULL);
}
