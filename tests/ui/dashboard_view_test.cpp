#include <string>
#include <optional>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "app/app_config.h"
#include "model/system_snapshot.h"
#include "ui/app_controller.h"
#include "ui/dashboard_view.h"

namespace {
struct ScreenMatch {
    int x;
    int y;
};

std::optional<ScreenMatch> find_ascii_text(const ftxui::Screen& screen, std::string_view needle) {
    if (needle.empty() || needle.size() > static_cast<std::size_t>(screen.dimx())) {
        return std::nullopt;
    }

    for (int y = 0; y < screen.dimy(); ++y) {
        for (int x = 0; x <= screen.dimx() - static_cast<int>(needle.size()); ++x) {
            bool matches = true;
            for (std::size_t index = 0; index < needle.size(); ++index) {
                if (screen.PixelAt(x + static_cast<int>(index), y).character != std::string(1, needle[index])) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return ScreenMatch{x, y};
            }
        }
    }
    return std::nullopt;
}
}  // namespace

TEST_CASE("dashboard renders the approved panel layout") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.cpu.total_percent = 37.0;
    snapshot.memory.total_bytes = 64ULL * 1024ULL * 1024ULL;
    snapshot.memory.used_bytes = 31ULL * 1024ULL * 1024ULL;
    snapshot.disks.push_back({"/", 71.0, 21 * 1024ULL * 1024ULL, 13 * 1024ULL * 1024ULL});
    snapshot.interfaces.push_back({"eth0", 12 * 1024ULL * 1024ULL, 2 * 1024ULL * 1024ULL});
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 0, "root", "postgres"});

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.handle_key(':');
    controller.handle_text("sort cpu");
    const auto output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);

    CHECK(output.find("CPU") != std::string::npos);
    CHECK(output.find("Memory") != std::string::npos);
    CHECK(output.find("Disk") != std::string::npos);
    CHECK(output.find("Network") != std::string::npos);
    CHECK(output.find("Processes") != std::string::npos);
    CHECK(output.find("q quit") != std::string::npos);
    CHECK(output.find("Input") != std::string::npos);
    CHECK(output.find("Status") != std::string::npos);
    CHECK(output.find("ready") != std::string::npos);
    CHECK(output.find("refresh 1000ms") != std::string::npos);
    CHECK(output.find(":sort cpu") != std::string::npos);
    CHECK(output.find("postgres") != std::string::npos);
}

TEST_CASE("dashboard shows filter text while in filter mode") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 0, "root", "postgres"});
    snapshot.processes.push_back({301, 'S', 12.0, 1.0, 0, "user", "nginx"});
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.handle_key('/');
    controller.handle_text("postgres");
    controller.set_visible_process_count(1);

    const auto output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);

    CHECK(output.find("/postgres") != std::string::npos);
    CHECK(output.find("Input") != std::string::npos);
    CHECK(output.find("Filter mode") != std::string::npos);
    CHECK(output.find("postgres") != std::string::npos);
    CHECK(output.find("nginx") != std::string::npos);
}

TEST_CASE("dashboard highlights selected filtered process") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 0, "root", "postgres"});
    snapshot.processes.push_back({301, 'S', 12.0, 1.0, 0, "user", "nginx"});
    snapshot.processes.push_back({411, 'S', 9.0, 0.5, 0, "user", "redis"});

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.set_visible_process_count(3);
    controller.handle_key('j');
    const auto output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);

    CHECK(output.find("> 301") != std::string::npos);
    CHECK(output.find("j/k select") != std::string::npos);
}

TEST_CASE("dashboard marks the focused panel title for h l navigation") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 0, "root", "postgres"});

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    const auto default_output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);
    CHECK(default_output.find("Processes [sort: cpu] *") != std::string::npos);

    controller.handle_key('h');
    const auto moved_output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);
    CHECK(moved_output.find("Network *") != std::string::npos);
    CHECK(moved_output.find("Processes [sort: cpu] *") == std::string::npos);
}

TEST_CASE("dashboard shows the current process sort key in the panel title") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 0, "root", "postgres"});

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    const auto cpu_output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);
    CHECK(cpu_output.find("Processes [sort: cpu] *") != std::string::npos);

    controller.handle_key('s');
    const auto memory_output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);
    CHECK(memory_output.find("Processes [sort: memory] *") != std::string::npos);
}

TEST_CASE("dashboard renders Catppuccin Mocha colors") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 0, "root", "postgres"});

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.set_visible_process_count(1);
    controller.set_process_window_height(4);

    auto document = monitor::ui::render_dashboard_document(snapshot, controller, 120, 40);
    auto screen = ftxui::Screen(120, 40);
    ftxui::Render(screen, document);

    const auto title = find_ascii_text(screen, "Processes");
    REQUIRE(title.has_value());
    CHECK(screen.PixelAt(title->x, title->y).foreground_color == ftxui::Color::RGB(137, 180, 250));

    const auto selected_row = find_ascii_text(screen, "> 812");
    REQUIRE(selected_row.has_value());
    CHECK(screen.PixelAt(selected_row->x, selected_row->y).background_color == ftxui::Color::RGB(49, 50, 68));

    const auto ready = find_ascii_text(screen, "ready");
    REQUIRE(ready.has_value());
    CHECK(screen.PixelAt(ready->x, ready->y).foreground_color == ftxui::Color::RGB(166, 227, 161));
}

TEST_CASE("dashboard shows selected process details in the right pane") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 5, "root", "postgres"});
    snapshot.processes.push_back({301, 'S', 12.0, 1.0, 0, "user", "nginx"});

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.set_visible_process_count(2);
    controller.handle_key('j');

    const auto output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);

    CHECK(output.find("Selected Process") != std::string::npos);
    CHECK(output.find("PID: 301") != std::string::npos);
    CHECK(output.find("Name: nginx") != std::string::npos);
    CHECK(output.find("User: user") != std::string::npos);
    CHECK(output.find("State: S") != std::string::npos);
    CHECK(output.find("Memory %: 1.0") != std::string::npos);
    CHECK(output.find("Nice: 0") != std::string::npos);
    CHECK(output.find("K kill") != std::string::npos);
    CHECK(output.find("R renice") != std::string::npos);
}

TEST_CASE("dashboard keeps the selected row visible near the bottom while scrolling") {
    monitor::model::SystemSnapshot snapshot;
    for (int index = 0; index < 10; ++index) {
        snapshot.processes.push_back(
            {100 + index, 'S', 100.0 - static_cast<double>(index), 1.0, 0, "user", "proc" + std::to_string(index)});
    }

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.set_visible_process_count(snapshot.processes.size());
    controller.set_process_window_height(4);
    for (int step = 0; step < 6; ++step) {
        controller.handle_key('j');
    }

    const auto output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 24);

    CHECK(controller.process_window_start() == 4);
    CHECK(output.find("> 106") != std::string::npos);
    CHECK(output.find("PID: 106") != std::string::npos);
}

TEST_CASE("dashboard shows empty detail pane when no process is selected") {
    monitor::model::SystemSnapshot snapshot;
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.set_visible_process_count(0);

    const auto output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);

    CHECK(output.find("no matching processes") != std::string::npos);
    CHECK(output.find("Selected Process") != std::string::npos);
    CHECK(output.find("No process selected") != std::string::npos);
}
