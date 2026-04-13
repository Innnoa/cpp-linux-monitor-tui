#include "collector/network_collector.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

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
    std::unordered_map<std::string, NetworkCounters> previous_by_interface;
    previous_by_interface.reserve(previous.size());
    for (const auto& counter : previous) {
        previous_by_interface.emplace(counter.interface_name, counter);
    }

    for (const auto& counter : current) {
        const auto previous_it = previous_by_interface.find(counter.interface_name);
        if (previous_it == previous_by_interface.end()) {
            continue;
        }

        const auto& previous_counter = previous_it->second;
        model::NetworkMetrics metric;
        metric.interface_name = counter.interface_name;
        const auto rx_delta = (counter.rx_bytes > previous_counter.rx_bytes)
                                  ? (counter.rx_bytes - previous_counter.rx_bytes)
                                  : 0ULL;
        const auto tx_delta = (counter.tx_bytes > previous_counter.tx_bytes)
                                  ? (counter.tx_bytes - previous_counter.tx_bytes)
                                  : 0ULL;
        metric.rx_bytes_per_sec = rx_delta;
        metric.tx_bytes_per_sec = tx_delta;
        metrics.push_back(metric);
    }
    return metrics;
}

}  // namespace monitor::collector
