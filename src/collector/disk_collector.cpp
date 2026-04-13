#include "collector/disk_collector.h"

#include <limits>
#include <sstream>
#include <unordered_map>

namespace monitor::collector {

std::vector<DiskCounters> parse_disk_stats(std::string_view text) {
    std::vector<DiskCounters> counters;
    std::istringstream input(std::string{text});
    DiskCounters counter;
    std::uint64_t major = 0;
    std::uint64_t minor = 0;
    std::uint64_t reads_completed = 0;
    std::uint64_t reads_merged = 0;
    std::uint64_t writes_completed = 0;
    std::uint64_t writes_merged = 0;
    std::uint64_t ignored = 0;

    while (input >> major >> minor >> counter.device >> reads_completed >> reads_merged >>
        counter.read_sectors >> ignored >> writes_completed >> writes_merged >>
        counter.write_sectors) {
        counters.push_back(counter);
        // Consume the rest of the current disk stats line.
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return counters;
}

std::vector<model::DiskMetrics> compute_disk_metrics(
    const std::vector<DiskCounters>& previous,
    const std::vector<DiskCounters>& current,
    std::string mount_label) {
    std::vector<model::DiskMetrics> metrics;
    std::unordered_map<std::string, DiskCounters> previous_by_device;
    previous_by_device.reserve(previous.size());
    for (const auto& counter : previous) {
        previous_by_device.emplace(counter.device, counter);
    }

    for (const auto& counter : current) {
        const auto previous_it = previous_by_device.find(counter.device);
        if (previous_it == previous_by_device.end()) {
            continue;
        }

        const auto& previous_counter = previous_it->second;
        model::DiskMetrics metric;
        metric.label = mount_label;
        metric.read_bytes_per_sec =
            (counter.read_sectors - previous_counter.read_sectors) * 512ULL;
        metric.write_bytes_per_sec =
            (counter.write_sectors - previous_counter.write_sectors) * 512ULL;
        metrics.push_back(metric);
    }
    return metrics;
}

}  // namespace monitor::collector
