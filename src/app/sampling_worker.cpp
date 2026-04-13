#include "app/sampling_worker.h"

namespace monitor::app {

void SamplingWorker::tick_once() {
    store_.publish(sampler_.collect());
}

}  // namespace monitor::app
