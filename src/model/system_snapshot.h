#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace monitor::model {

struct CpuMetrics {
    double total_percent{0.0};
    std::vector<double> core_percents;
};

struct MemoryMetrics {
    std::uint64_t total_bytes{0};
    std::uint64_t used_bytes{0};
    std::uint64_t swap_total_bytes{0};
    std::uint64_t swap_used_bytes{0};
};

struct DiskMetrics {
    std::string label;
    double used_percent{0.0};
    std::uint64_t read_bytes_per_sec{0};
    std::uint64_t write_bytes_per_sec{0};
};

struct NetworkMetrics {
    std::string interface_name;
    std::uint64_t rx_bytes_per_sec{0};
    std::uint64_t tx_bytes_per_sec{0};
};

struct ProcessInfo {
    int pid{0};
    char state{'S'};
    double cpu_percent{0.0};
    double memory_percent{0.0};
    int nice_value{0};
    std::string user;
    std::string name;
};

struct SystemSnapshot {
    std::chrono::system_clock::time_point captured_at{};
    CpuMetrics cpu;
    MemoryMetrics memory;
    std::vector<DiskMetrics> disks;
    std::vector<NetworkMetrics> interfaces;
    std::vector<ProcessInfo> processes;
};

}  // namespace monitor::model
