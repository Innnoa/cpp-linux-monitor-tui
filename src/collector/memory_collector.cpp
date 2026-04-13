#include "collector/memory_collector.h"

#include <sstream>
#include <string>

namespace monitor::collector {

model::MemoryMetrics parse_memory_info(std::string_view text) {
    model::MemoryMetrics metrics;
    std::istringstream input(std::string{text});
    std::string label;
    std::uint64_t value = 0;
    std::string unit;
    std::uint64_t mem_available = 0;
    std::uint64_t swap_free = 0;

    while (input >> label >> value >> unit) {
        if (label == "MemTotal:") {
            metrics.total_bytes = value * 1024ULL;
        } else if (label == "MemAvailable:") {
            mem_available = value * 1024ULL;
        } else if (label == "SwapTotal:") {
            metrics.swap_total_bytes = value * 1024ULL;
        } else if (label == "SwapFree:") {
            swap_free = value * 1024ULL;
        }
    }

    metrics.used_bytes = metrics.total_bytes - mem_available;
    metrics.swap_used_bytes = metrics.swap_total_bytes - swap_free;
    return metrics;
}

}  // namespace monitor::collector
