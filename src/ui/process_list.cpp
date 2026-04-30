#include "ui/process_list.h"

#include <iomanip>
#include <sstream>
#include <algorithm>

namespace monitor::ui {

namespace {

constexpr double kHighCpuThreshold = 50.0;
constexpr double kMediumCpuThreshold = 20.0;
constexpr double kHighMemoryThreshold = 10.0;
constexpr double kMediumMemoryThreshold = 5.0;

}  // namespace

ftxui::Color process_row_color(
    const model::ProcessInfo& process,
    const ProcessColorScheme& color_scheme) {
    if (process.state == 'Z') {
        return color_scheme.zombie;
    }

    if (process.cpu_percent >= kHighCpuThreshold) {
        return color_scheme.high_cpu;
    }

    if (process.memory_percent >= kHighMemoryThreshold) {
        return color_scheme.high_memory;
    }

    if (process.cpu_percent >= kMediumCpuThreshold) {
        return color_scheme.medium_cpu;
    }

    if (process.memory_percent >= kMediumMemoryThreshold) {
        return color_scheme.medium_memory;
    }

    if (process.state == 'R') {
        return color_scheme.running;
    }

    return color_scheme.default_text;
}

std::string format_process_row(
    const model::ProcessInfo& process,
    bool is_selected) {
    std::ostringstream line;

    line << (is_selected ? "▸ " : "  ");

    line << std::setw(6) << process.pid << " ";

    line << process.state << " ";

    line << std::setw(6) << std::fixed << std::setprecision(1) << process.cpu_percent << " ";

    line << std::setw(6) << std::fixed << std::setprecision(1) << process.memory_percent << " ";

    std::string user = process.user;
    if (user.size() > 8) {
        user = user.substr(0, 7) + "…";
    }
    line << std::setw(8) << std::left << user << " ";

    std::string name = process.name;
    if (name.size() > 15) {
        name = name.substr(0, 14) + "…";
    }
    line << std::setw(15) << std::left << name;

    return line.str();
}

ftxui::Element process_list_row(
    const model::ProcessInfo& process,
    bool is_selected,
    const ProcessColorScheme& color_scheme) {
    const auto text = format_process_row(process, is_selected);
    const auto color = process_row_color(process, color_scheme);

    auto element = ftxui::text(text) | ftxui::color(color);

    if (is_selected) {
        element = element | ftxui::bgcolor(ftxui::Color::RGB(49, 50, 68)) | ftxui::bold;
    }

    return element;
}

ftxui::Element process_list_header() {
    return ftxui::text("  PID    S   CPU%   MEM%   USER     NAME           ")
           | ftxui::color(ftxui::Color::RGB(137, 180, 250))
           | ftxui::bold;
}

ftxui::Element process_list(
    std::span<const model::ProcessInfo> processes,
    std::size_t selected_index,
    std::size_t window_start,
    std::size_t window_height,
    const ProcessColorScheme& color_scheme) {
    std::vector<ftxui::Element> rows;

    rows.push_back(process_list_header());

    if (processes.empty()) {
        rows.push_back(
            ftxui::text("  no matching processes")
            | ftxui::color(ftxui::Color::RGB(250, 179, 135)));
    } else {
        const auto clamped_selected = std::min(selected_index, processes.size() - 1);
        const auto clamped_start = std::min(window_start, processes.size() - 1);
        const auto end_index = std::min(
            processes.size(),
            clamped_start + std::max<std::size_t>(1, window_height));

        for (std::size_t i = clamped_start; i < end_index; ++i) {
            const auto is_selected = (i == clamped_selected);
            rows.push_back(process_list_row(processes[i], is_selected, color_scheme));
        }
    }

    return ftxui::vbox(rows);
}

}  // namespace monitor::ui
