#include "ui/dashboard_view.h"

#include "collector/process_collector.h"
#include "model/history_data.h"
#include "ui/process_list.h"
#include "ui/progress_bar.h"
#include "ui/sparkline_chart.h"
#include "ui/theme.h"

#include <iomanip>
#include <sstream>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

namespace monitor::ui {

namespace {
std::string format_percent(double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(0) << value << "%";
    return output.str();
}

std::string format_mb(std::uint64_t bytes) {
    const auto mb = bytes / (1024ULL * 1024ULL);
    return std::to_string(mb) + " MB";
}

std::string format_gb(std::uint64_t bytes) {
    const auto gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    std::ostringstream output;
    output << std::fixed << std::setprecision(1) << gb << " GB";
    return output.str();
}

std::string format_speed(std::uint64_t bytes_per_sec) {
    if (bytes_per_sec >= 1024ULL * 1024ULL) {
        const auto mb = static_cast<double>(bytes_per_sec) / (1024.0 * 1024.0);
        std::ostringstream output;
        output << std::fixed << std::setprecision(1) << mb << " MB/s";
        return output.str();
    }
    const auto kb = bytes_per_sec / 1024ULL;
    return std::to_string(kb) + " KB/s";
}

std::string process_sort_title(collector::ProcessSortKey sort_key) {
    switch (sort_key) {
        case collector::ProcessSortKey::Cpu:
            return "Processes [sort: cpu]";
        case collector::ProcessSortKey::Memory:
            return "Processes [sort: memory]";
        case collector::ProcessSortKey::Pid:
            return "Processes [sort: pid]";
        case collector::ProcessSortKey::Name:
            return "Processes [sort: name]";
    }
    return "Processes";
}

ftxui::Element resource_panel(
    const std::string& title,
    double percentage,
    std::span<const double> history,
    const std::string& summary,
    ftxui::Color accent_color,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color,
    bool focused,
    int bar_width) {
    const auto& theme = catppuccin_mocha();
    ProgressBarConfig bar_config;
    bar_config.width = bar_width;
    bar_config.show_percentage = true;

    SparklineConfig spark_config;
    spark_config.width = bar_width + 6;
    spark_config.show_latest = false;

    return themed_window(
        title,
        ftxui::vbox({
            ftxui::text(summary) | ftxui::color(theme.text),
            progress_bar(percentage, bar_config, low_color, medium_color, high_color),
            sparkline_chart(history, accent_color, spark_config),
        }),
        focused);
}

ftxui::Element render_left_panel(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history) {
    const auto& theme = catppuccin_mocha();

    const auto cpu_summary = "Total: " + format_percent(snapshot.cpu.total_percent);
    const auto mem_pct = (snapshot.memory.total_bytes > 0)
                             ? (static_cast<double>(snapshot.memory.used_bytes) / snapshot.memory.total_bytes * 100.0)
                             : 0.0;
    const auto mem_summary = format_gb(snapshot.memory.used_bytes) + " / " + format_gb(snapshot.memory.total_bytes);

    constexpr int kBarWidth = 24;

    auto cpu_panel = resource_panel(
        "CPU", snapshot.cpu.total_percent, history.cpu_history.data(), cpu_summary,
        theme.red, theme.green, theme.peach, theme.red,
        controller.focus() == app::FocusZone::Cpu, kBarWidth);

    auto mem_panel = resource_panel(
        "Memory", mem_pct, history.memory_history.data(), mem_summary,
        theme.green, theme.green, theme.peach, theme.red,
        controller.focus() == app::FocusZone::Memory, kBarWidth);

    return ftxui::vbox({
        std::move(cpu_panel),
        std::move(mem_panel),
    });
}

ftxui::Element render_right_panel(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history) {
    const auto& theme = catppuccin_mocha();

    std::string disk_summary = "n/a";
    double disk_pct = 0.0;
    if (!snapshot.disks.empty()) {
        const auto& disk = snapshot.disks.front();
        disk_pct = disk.used_percent;
        disk_summary = disk.label + " " + format_percent(disk.used_percent);
    }

    std::string network_summary = "n/a";
    double network_pct = 0.0;
    if (!snapshot.interfaces.empty()) {
        const auto& net = snapshot.interfaces.front();
        const auto total_speed = net.rx_bytes_per_sec + net.tx_bytes_per_sec;
        network_summary = net.interface_name + " " + format_speed(total_speed);
        constexpr std::uint64_t kMaxSpeed = 125000000ULL;  // 1 Gbps
        network_pct = std::min(100.0, static_cast<double>(total_speed) / kMaxSpeed * 100.0);
    }

    constexpr int kBarWidth = 20;

    auto disk_panel = resource_panel(
        "Disk", disk_pct, history.disk_read_history.data(), disk_summary,
        theme.peach, theme.green, theme.peach, theme.red,
        controller.focus() == app::FocusZone::Disk, kBarWidth);

    auto net_panel = resource_panel(
        "Network", network_pct, history.network_rx_history.data(), network_summary,
        theme.sapphire, theme.green, theme.peach, theme.red,
        controller.focus() == app::FocusZone::Network, kBarWidth);

    return ftxui::vbox({
        std::move(disk_panel),
        std::move(net_panel),
    });
}

}  // namespace

ftxui::Element render_dashboard_body_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history) {
    const auto& theme = catppuccin_mocha();

    const auto header = ftxui::hbox({
        ftxui::text("host: local") | ftxui::color(theme.subtext1),
        ftxui::separator() | ftxui::color(theme.surface2),
        ftxui::text("refresh " + std::to_string(controller.refresh_interval().count()) + "ms")
            | ftxui::color(theme.subtext1),
        ftxui::separator() | ftxui::color(theme.surface2),
        ftxui::text("tab: proc") | ftxui::color(theme.rosewater) | ftxui::bold,
    }) | ftxui::bgcolor(theme.base);

    auto left = render_left_panel(snapshot, controller, history);
    auto right = render_right_panel(snapshot, controller, history);

    const auto resource_row = ftxui::hbox({
        left | ftxui::flex,
        ftxui::separator() | ftxui::color(theme.surface2),
        right | ftxui::flex,
    });

    auto visible_processes =
        collector::filter_processes(collector::sort_processes(snapshot.processes, controller.sort_key()),
                                    controller.filter_query());

    ProcessColorScheme color_scheme;
    const auto process_element = process_list(
        visible_processes,
        controller.selected_process_index(),
        controller.process_window_start(),
        controller.process_window_height(),
        color_scheme);

    ftxui::Element detail_body;
    if (visible_processes.empty()) {
        detail_body = ftxui::vbox({
            ftxui::text("No process selected") | ftxui::color(theme.overlay1),
        });
    } else {
        const auto selected_index = std::min(controller.selected_process_index(), visible_processes.size() - 1);
        const auto& selected = visible_processes[selected_index];
        std::ostringstream memory_line;
        memory_line << std::fixed << std::setprecision(1) << selected.memory_percent;
        detail_body = ftxui::vbox({
            ftxui::text("PID: " + std::to_string(selected.pid)) | ftxui::color(theme.text),
            ftxui::text("Name: " + selected.name) | ftxui::color(theme.text),
            ftxui::text("User: " + selected.user) | ftxui::color(theme.text),
            ftxui::text(std::string{"State: "} + selected.state) | ftxui::color(theme.text),
            ftxui::text("Memory %: " + memory_line.str()) | ftxui::color(theme.text),
            ftxui::text("Nice: " + std::to_string(selected.nice_value)) | ftxui::color(theme.text),
            ftxui::separator() | ftxui::color(theme.surface2),
            ftxui::text("K kill") | ftxui::color(theme.red) | ftxui::bold,
            ftxui::text("R renice") | ftxui::color(theme.peach) | ftxui::bold,
        });
    }

    const auto detail = themed_window("Selected Process", detail_body, false);
    const auto process_area = ftxui::hbox({
        themed_window(process_sort_title(controller.sort_key()),
                      ftxui::vbox({
                          process_element,
                          ftxui::separator() | ftxui::color(theme.surface2),
                          ftxui::text("h/l focus · j/k select · / filter · : command · K kill · R renice · q quit")
                              | ftxui::color(theme.overlay1) | ftxui::dim,
                      }),
                      controller.focus() == app::FocusZone::Processes)
            | ftxui::flex,
        detail | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32),
    });

    return ftxui::vbox({
               header,
               ftxui::separator() | ftxui::color(theme.surface2),
               resource_row,
               ftxui::separator() | ftxui::color(theme.surface2),
               process_area,
           })
           | ftxui::bgcolor(theme.base);
}

ftxui::Element render_dashboard_bottom_bar_document(const AppController& controller) {
    const auto& theme = catppuccin_mocha();
    if (controller.shared_input_active()) {
        return ftxui::hbox({
            themed_window(
                "Input",
                ftxui::text(controller.command_text()) | ftxui::color(theme.text),
                controller.focus() == app::FocusZone::CommandBar)
                | ftxui::flex,
            themed_window("Status", ftxui::text(controller.status_text()) | ftxui::color(status_color(controller.status_text())), false)
                | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 28),
        }) | ftxui::bgcolor(theme.base);
    }
    return themed_window(
               "Status",
               ftxui::text(controller.status_text()) | ftxui::color(status_color(controller.status_text())),
               controller.focus() == app::FocusZone::CommandBar)
           | ftxui::bgcolor(theme.base);
}

ftxui::Element render_dashboard_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history,
    int width,
    int height) {
    auto document =
        ftxui::vbox({render_dashboard_body_document(snapshot, controller, history), render_dashboard_bottom_bar_document(controller)});
    return document | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width)
           | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);
}

std::string render_dashboard_to_string(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history,
    int width,
    int height) {
    auto document = render_dashboard_document(snapshot, controller, history, width, height);
    auto screen = ftxui::Screen(width, height);
    ftxui::Render(screen, document);
    return screen.ToString();
}

}  // namespace monitor::ui
