#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "model/system_snapshot.h"

namespace monitor::collector {

struct DiskCounters {
    std::string device;
    std::uint64_t read_sectors{0};
    std::uint64_t write_sectors{0};
};

std::vector<DiskCounters> parse_disk_stats(std::string_view text);
std::vector<model::DiskMetrics> compute_disk_metrics(
    const std::vector<DiskCounters>& previous,
    const std::vector<DiskCounters>& current,
    std::string mount_label);

}  // namespace monitor::collector
