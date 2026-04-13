#include "collector/process_collector.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace monitor::collector {

namespace {
std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}
}  // namespace

model::ProcessInfo parse_process_info(
    int pid,
    std::string_view stat_text,
    std::string_view status_text,
    std::uint64_t total_memory_bytes,
    long /*clock_ticks_per_second*/) {
    model::ProcessInfo info;
    info.pid = pid;

    const auto open = stat_text.find('(');
    const auto close = stat_text.rfind(')');
    info.name = std::string{stat_text.substr(open + 1, close - open - 1)};
    info.state = close + 2 < stat_text.size() ? stat_text[close + 2] : 'S';

    if (open != std::string_view::npos && close != std::string_view::npos && close + 4 < stat_text.size()) {
        std::istringstream stat_stream(std::string{stat_text.substr(close + 4)});
        long value = 0;
        for (int field = 4; stat_stream >> value; ++field) {
            if (field == 19) {
                info.nice_value = static_cast<int>(value);
                break;
            }
        }
    }

    std::istringstream status_stream(std::string{status_text});
    std::string label;
    while (status_stream >> label) {
        if (label == "VmRSS:") {
            std::uint64_t rss_kb = 0;
            std::string unit;
            status_stream >> rss_kb >> unit;
            info.memory_percent =
                (static_cast<double>(rss_kb) * 1024.0 / static_cast<double>(total_memory_bytes)) * 100.0;
        } else if (label == "Uid:") {
            int uid = 0;
            status_stream >> uid;
            info.user = uid == 0 ? "root" : "user";
        }
    }

    return info;
}

std::vector<model::ProcessInfo> sort_processes(
    std::vector<model::ProcessInfo> processes,
    ProcessSortKey sort_key) {
    std::sort(processes.begin(), processes.end(), [sort_key](const auto& left, const auto& right) {
        switch (sort_key) {
            case ProcessSortKey::Cpu:
                return left.cpu_percent > right.cpu_percent;
            case ProcessSortKey::Memory:
                return left.memory_percent > right.memory_percent;
            case ProcessSortKey::Pid:
                return left.pid < right.pid;
            case ProcessSortKey::Name:
                return left.name < right.name;
        }
        return false;
    });
    return processes;
}

std::vector<model::ProcessInfo> filter_processes(
    const std::vector<model::ProcessInfo>& processes,
    std::string_view query) {
    const auto needle = lowercase(std::string{query});
    if (needle.empty()) {
        return processes;
    }

    std::vector<model::ProcessInfo> filtered;
    for (const auto& process : processes) {
        if (lowercase(process.name).find(needle) != std::string::npos) {
            filtered.push_back(process);
        }
    }
    return filtered;
}

}  // namespace monitor::collector
