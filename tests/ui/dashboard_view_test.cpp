#include <string>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>

#include "app/app_config.h"
#include "model/system_snapshot.h"
#include "ui/app_controller.h"
#include "ui/dashboard_view.h"

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
    CHECK(output.find("Command Bar") != std::string::npos);
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
    CHECK(output.find("Filter mode") != std::string::npos);
    CHECK(output.find("postgres") != std::string::npos);
    CHECK(output.find("nginx") == std::string::npos);
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
