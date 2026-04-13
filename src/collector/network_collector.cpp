#include "collector/network_collector.h"

#include <algorithm>
#include <sstream>

namespace monitor::collector {

std::vector<NetworkCounters> parse_network_stats(std::string_view text) {
    std::vector<NetworkCounters> counters;
    std::istringstream input(std::string{text});
    std::string line;
    while (std::getline(input, line)) {
        if (line.find(':') == std::string::npos) {
            continue;
        }
        NetworkCounters counter;
        std::replace(line.begin(), line.end(), ':', ' ');
        std::istringstream row(line);
        row >> counter.interface_name >> counter.rx_bytes;
        for (int skip = 0; skip < 7; ++skip) {
            std::uint64_t ignored = 0;
            row >> ignored;
        }
        row >> counter.tx_bytes;
        counters.push_back(counter);
    }
    return counters;
}

std::vector<model::NetworkMetrics> compute_network_metrics(
    const std::vector<NetworkCounters>& previous,
    const std::vector<NetworkCounters>& current) {
    std::vector<model::NetworkMetrics> metrics;
    for (std::size_t index = 0; index < current.size(); ++index) {
        model::NetworkMetrics metric;
        metric.interface_name = current.at(index).interface_name;
        metric.rx_bytes_per_sec = current.at(index).rx_bytes - previous.at(index).rx_bytes;
        metric.tx_bytes_per_sec = current.at(index).tx_bytes - previous.at(index).tx_bytes;
        metrics.push_back(metric);
    }
    return metrics;
}

}  // namespace monitor::collector
