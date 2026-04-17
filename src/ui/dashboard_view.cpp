#include "ui/dashboard_view.h"

#include "collector/process_collector.h"
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

ftxui::Element resource_box(
    const std::string& title,
    const std::string& summary,
    ftxui::Color accent_color,
    bool focused) {
    const auto& theme = catppuccin_mocha();
    return themed_window(title,
                         ftxui::vbox({
                             ftxui::text(summary) | ftxui::color(theme.text),
                             ftxui::separator() | ftxui::color(theme.surface2),
                             ftxui::text("▁▂▃▄▅▆▅▄") | ftxui::color(accent_color) | ftxui::bold,
                         }),
                         focused);
}
}  // namespace

ftxui::Element render_dashboard_body_document(const model::SystemSnapshot& snapshot, const AppController& controller) {
    const auto& theme = catppuccin_mocha();
    const auto header = ftxui::hbox({
        ftxui::text("host: local") | ftxui::color(theme.subtext1),
        ftxui::separator() | ftxui::color(theme.surface2),
        ftxui::text("refresh " + std::to_string(controller.refresh_interval().count()) + "ms")
            | ftxui::color(theme.subtext1),
        ftxui::separator() | ftxui::color(theme.surface2),
        ftxui::text("tab: proc") | ftxui::color(theme.rosewater) | ftxui::bold,
    }) | ftxui::bgcolor(theme.base);

    const auto cpu_summary = format_percent(snapshot.cpu.total_percent);
    const auto memory_summary =
        format_mb(snapshot.memory.used_bytes) + " / " + format_mb(snapshot.memory.total_bytes);
    std::string disk_summary = "n/a";
    if (!snapshot.disks.empty()) {
        const auto& disk = snapshot.disks.front();
        disk_summary = disk.label + " " + format_percent(disk.used_percent);
    }
    std::string network_summary = "n/a";
    if (!snapshot.interfaces.empty()) {
        const auto& network = snapshot.interfaces.front();
        network_summary = network.interface_name + " " + format_mb(network.rx_bytes_per_sec) + "/s";
    }

    const auto resources = ftxui::gridbox({
        {resource_box("CPU", cpu_summary, theme.red, controller.focus() == app::FocusZone::Cpu),
         resource_box("Memory", memory_summary, theme.green, controller.focus() == app::FocusZone::Memory)},
        {resource_box("Disk", disk_summary, theme.peach, controller.focus() == app::FocusZone::Disk),
         resource_box("Network", network_summary, theme.sapphire, controller.focus() == app::FocusZone::Network)},
    });

    auto visible_processes =
        collector::filter_processes(collector::sort_processes(snapshot.processes, controller.sort_key()),
                                    controller.filter_query());
    std::vector<ftxui::Element> process_rows;
    process_rows.push_back(ftxui::text("PID   S   CPU   MEM   USER    NAME") | ftxui::color(theme.blue) | ftxui::bold);
    if (visible_processes.empty()) {
        process_rows.push_back(ftxui::text("no matching processes") | ftxui::color(theme.peach));
    } else {
        const auto selected_index = std::min(controller.selected_process_index(), visible_processes.size() - 1);
        const auto start_index = std::min(controller.process_window_start(), visible_processes.size() - 1);
        const auto end_index =
            std::min(visible_processes.size(), start_index + std::max<std::size_t>(1, controller.process_window_height()));
        for (std::size_t index = start_index; index < end_index; ++index) {
            const auto& process = visible_processes[index];
            std::ostringstream line;
            line << ((index == selected_index) ? "> " : "  ");
            line << process.pid << "   " << process.state << "   " << std::fixed << std::setprecision(0)
                 << process.cpu_percent << "    " << std::setprecision(1) << process.memory_percent << "   "
                 << process.user << "    " << process.name;
            auto row = ftxui::text(line.str());
            if (index == selected_index) {
                row = row | ftxui::color(theme.text) | ftxui::bgcolor(theme.surface0) | ftxui::bold;
            } else {
                row = row | ftxui::color(theme.subtext1);
            }
            process_rows.push_back(std::move(row));
        }
    }
    process_rows.push_back(ftxui::text("h/l focus · j/k select · / filter · : command · K kill · R renice · q quit")
                               | ftxui::color(theme.overlay1) | ftxui::dim);

    const auto process_list = themed_window(
        process_sort_title(controller.sort_key()), ftxui::vbox(process_rows),
        controller.focus() == app::FocusZone::Processes);

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
    const auto lower = ftxui::hbox({
        process_list | ftxui::flex,
        detail | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32),
    });

    return ftxui::vbox({
               header,
               ftxui::separator() | ftxui::color(theme.surface2),
               resources,
               ftxui::separator() | ftxui::color(theme.surface2),
               lower,
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
    int width,
    int height) {
    auto document =
        ftxui::vbox({render_dashboard_body_document(snapshot, controller), render_dashboard_bottom_bar_document(controller)});
    return document | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width)
           | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);
}

std::string render_dashboard_to_string(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    int width,
    int height) {
    auto document = render_dashboard_document(snapshot, controller, width, height);
    auto screen = ftxui::Screen(width, height);
    ftxui::Render(screen, document);
    return screen.ToString();
}

}  // namespace monitor::ui
