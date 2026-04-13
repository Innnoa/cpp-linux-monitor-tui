#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "model/system_snapshot.h"

namespace monitor::collector {

struct CpuTimes {
    std::uint64_t user{0};
    std::uint64_t nice{0};
    std::uint64_t system{0};
    std::uint64_t idle{0};
    std::uint64_t iowait{0};
};

struct CpuSample {
    CpuTimes total;
    std::vector<CpuTimes> cores;
};

CpuSample parse_cpu_sample(std::string_view text);
model::CpuMetrics compute_cpu_metrics(const CpuSample& previous, const CpuSample& current);

}  // namespace monitor::collector
