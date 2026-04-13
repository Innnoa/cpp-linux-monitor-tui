#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/system_snapshot.h"

namespace monitor::collector {

struct NetworkCounters {
    std::string interface_name;
    std::uint64_t rx_bytes{0};
    std::uint64_t tx_bytes{0};
};

std::vector<NetworkCounters> parse_network_stats(std::string_view text);
std::vector<model::NetworkMetrics> compute_network_metrics(
    const std::vector<NetworkCounters>& previous,
    const std::vector<NetworkCounters>& current);

}  // namespace monitor::collector
