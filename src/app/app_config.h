#pragma once

#include <chrono>
#include <cstddef>

namespace monitor::app {

enum class FocusZone {
    Cpu,
    Memory,
    Disk,
    Network,
    Processes,
    CommandBar,
};

struct AppConfig {
    std::chrono::milliseconds refresh_interval{1000};
    std::size_t history_size{60};
    FocusZone default_focus{FocusZone::Processes};

    static AppConfig defaults() { return {}; }
};

}  // namespace monitor::app
