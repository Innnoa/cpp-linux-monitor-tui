#pragma once

#include "model/system_snapshot.h"
#include "store/snapshot_store.h"

namespace monitor::app {

class Sampler {
  public:
    virtual ~Sampler() = default;
    virtual model::SystemSnapshot collect() = 0;
};

class SamplingWorker {
  public:
    SamplingWorker(Sampler& sampler, store::SnapshotStore& store)
        : sampler_(sampler), store_(store) {}

    void tick_once();

  private:
    Sampler& sampler_;
    store::SnapshotStore& store_;
};

}  // namespace monitor::app
