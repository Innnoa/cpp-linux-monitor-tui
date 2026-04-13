#pragma once

#include <string_view>
#include <vector>

#include "model/system_snapshot.h"

namespace monitor::collector {

enum class ProcessSortKey {
    Cpu,
    Memory,
    Pid,
    Name,
};

model::ProcessInfo parse_process_info(
    int pid,
    std::string_view stat_text,
    std::string_view status_text,
    std::uint64_t total_memory_bytes,
    long clock_ticks_per_second);

std::vector<model::ProcessInfo> sort_processes(
    std::vector<model::ProcessInfo> processes,
    ProcessSortKey sort_key);

std::vector<model::ProcessInfo> filter_processes(
    const std::vector<model::ProcessInfo>& processes,
    std::string_view query);

}  // namespace monitor::collector
