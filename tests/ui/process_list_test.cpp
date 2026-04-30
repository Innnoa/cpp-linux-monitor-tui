#include <catch2/catch_test_macros.hpp>
#include "ui/process_list.h"

TEST_CASE("format_process_row", "[process_list]") {
    monitor::model::ProcessInfo process;
    process.pid = 1234;
    process.state = 'R';
    process.cpu_percent = 45.5;
    process.memory_percent = 12.3;
    process.user = "root";
    process.name = "chrome";

    SECTION("Selected row has arrow") {
        auto result = monitor::ui::format_process_row(process, true);
        REQUIRE(result.starts_with("▸ "));
    }

    SECTION("Unselected row has spaces") {
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.starts_with("  "));
    }

    SECTION("Contains PID") {
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("1234") != std::string::npos);
    }

    SECTION("Contains process name") {
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("chrome") != std::string::npos);
    }

    SECTION("Long name is truncated") {
        process.name = "very_long_process_name_here";
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("…") != std::string::npos);
    }

    SECTION("Long user is truncated") {
        process.user = "very_long_username";
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("…") != std::string::npos);
    }
}

TEST_CASE("process_row_color", "[process_list]") {
    monitor::ui::ProcessColorScheme scheme;
    monitor::model::ProcessInfo process;

    SECTION("Zombie process gets zombie color") {
        process.state = 'Z';
        process.cpu_percent = 0.0;
        process.memory_percent = 0.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.zombie);
    }

    SECTION("High CPU gets high cpu color") {
        process.state = 'S';
        process.cpu_percent = 60.0;
        process.memory_percent = 1.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.high_cpu);
    }

    SECTION("Running process gets running color") {
        process.state = 'R';
        process.cpu_percent = 5.0;
        process.memory_percent = 1.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.running);
    }

    SECTION("Default process gets default color") {
        process.state = 'S';
        process.cpu_percent = 1.0;
        process.memory_percent = 1.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.default_text);
    }
}
