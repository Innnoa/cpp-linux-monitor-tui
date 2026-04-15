#include "ui/dashboard_view.h"

#include "collector/process_collector.h"

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

ftxui::Element resource_box(const std::string& title, const std::string& summary) {
    return ftxui::window(ftxui::text(title), ftxui::vbox({
        ftxui::text(summary),
        ftxui::separator(),
        ftxui::text("▁▂▃▄▅▆▅▄"),
    }));
}
}  // namespace

ftxui::Element render_dashboard_body_document(const model::SystemSnapshot& snapshot, const AppController& controller) {
    const auto header = ftxui::hbox({
        ftxui::text("host: local"),
        ftxui::separator(),
        ftxui::text("refresh " + std::to_string(controller.refresh_interval().count()) + "ms"),
        ftxui::separator(),
        ftxui::text("tab: proc"),
    });

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
        {resource_box("CPU", cpu_summary), resource_box("Memory", memory_summary)},
        {resource_box("Disk", disk_summary), resource_box("Network", network_summary)},
    });

    auto visible_processes =
        collector::filter_processes(collector::sort_processes(snapshot.processes, controller.sort_key()),
                                    controller.filter_query());
    std::vector<ftxui::Element> process_rows;
    process_rows.push_back(ftxui::text("PID   S   CPU   MEM   USER    NAME"));
    if (visible_processes.empty()) {
        process_rows.push_back(ftxui::text("no matching processes"));
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
            process_rows.push_back(ftxui::text(line.str()));
        }
    }
    process_rows.push_back(ftxui::text("h/l focus · j/k select · / filter · : command · K kill · R renice · q quit"));

    const auto process_list = ftxui::window(ftxui::text("Processes"), ftxui::vbox(process_rows));

    ftxui::Element detail_body;
    if (visible_processes.empty()) {
        detail_body = ftxui::vbox({
            ftxui::text("No process selected"),
        });
    } else {
        const auto selected_index = std::min(controller.selected_process_index(), visible_processes.size() - 1);
        const auto& selected = visible_processes[selected_index];
        std::ostringstream memory_line;
        memory_line << std::fixed << std::setprecision(1) << selected.memory_percent;
        detail_body = ftxui::vbox({
            ftxui::text("PID: " + std::to_string(selected.pid)),
            ftxui::text("Name: " + selected.name),
            ftxui::text("User: " + selected.user),
            ftxui::text(std::string{"State: "} + selected.state),
            ftxui::text("Memory %: " + memory_line.str()),
            ftxui::text("Nice: " + std::to_string(selected.nice_value)),
            ftxui::separator(),
            ftxui::text("K kill"),
            ftxui::text("R renice"),
        });
    }
    const auto detail = ftxui::window(ftxui::text("Selected Process"), detail_body);
    const auto lower = ftxui::hbox({
        process_list | ftxui::flex,
        detail | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32),
    });

    return ftxui::vbox({header, ftxui::separator(), resources, ftxui::separator(), lower});
}

ftxui::Element render_dashboard_bottom_bar_document(const AppController& controller) {
    if (controller.shared_input_active()) {
        return ftxui::hbox({
            ftxui::window(ftxui::text("Input"), ftxui::text(controller.command_text())) | ftxui::flex,
            ftxui::window(ftxui::text("Status"), ftxui::text(controller.status_text()))
                | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 28),
        });
    }
    return ftxui::window(ftxui::text("Status"), ftxui::text(controller.status_text()));
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
