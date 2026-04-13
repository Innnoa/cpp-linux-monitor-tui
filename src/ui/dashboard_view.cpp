#include "ui/dashboard_view.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

namespace monitor::ui {

namespace {
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
    const AppController& /*controller*/,
    int width,
    int height) {
    const auto header = ftxui::hbox({
        ftxui::text("host: local"),
        ftxui::separator(),
        ftxui::text("refresh 1000ms"),
        ftxui::separator(),
        ftxui::text("tab: proc"),
    });

    const auto resources = ftxui::gridbox({
        {resource_box("CPU", "37%"), resource_box("Memory", "31 / 64 GB")},
        {resource_box("Disk", "/ 71%"), resource_box("Network", "eth0 12 MB/s")},
    });

    const auto processes = ftxui::window(ftxui::text("Processes"), ftxui::vbox({
        ftxui::text("PID   S   CPU   MEM   USER    NAME"),
        ftxui::text("812   R   98    2.1   root    postgres"),
        ftxui::text("j/k move · gg/G jump · / filter · s sort · K kill · R renice"),
    }));

    const auto bottom = ftxui::hbox({
        ftxui::window(ftxui::text("Command Bar"), ftxui::text(":sort cpu")),
        ftxui::window(ftxui::text("Status"), ftxui::text("ready")),
    });

    auto document = ftxui::vbox({header, ftxui::separator(), resources, ftxui::separator(), processes, bottom});
    document = document | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width)
               | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);

    auto screen = ftxui::Screen(width, height);
    ftxui::Render(screen, document);
    return screen.ToString();
}

}  // namespace monitor::ui
