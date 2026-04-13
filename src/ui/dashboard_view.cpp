#include "ui/dashboard_view.h"

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

std::string render_dashboard_to_string(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    int width,
    int height) {
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

    std::string process_line = "no processes";
    if (!snapshot.processes.empty()) {
        const auto& process = snapshot.processes.front();
        std::ostringstream line;
        line << process.pid << "   " << process.state << "   " << std::fixed << std::setprecision(0)
             << process.cpu_percent << "    " << std::setprecision(1) << process.memory_percent << "   "
             << process.user << "    " << process.name;
        process_line = line.str();
    }

    const auto processes = ftxui::window(ftxui::text("Processes"), ftxui::vbox({
        ftxui::text("PID   S   CPU   MEM   USER    NAME"),
        ftxui::text(process_line),
        ftxui::text("h/l focus · / filter · : command · K kill · R renice · q quit"),
    }));

    auto command_display = controller.command_text();
    if (controller.mode() == InputMode::Filter) {
        command_display = "/" + controller.filter_query();
    }

    const auto bottom = ftxui::hbox({
        ftxui::window(ftxui::text("Command Bar"), ftxui::text(command_display)),
        ftxui::window(ftxui::text("Status"), ftxui::text(controller.status_text())),
    });

    auto document = ftxui::vbox({header, ftxui::separator(), resources, ftxui::separator(), processes, bottom});
    document = document | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width)
               | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);

    auto screen = ftxui::Screen(width, height);
    ftxui::Render(screen, document);
    return screen.ToString();
}

}  // namespace monitor::ui
