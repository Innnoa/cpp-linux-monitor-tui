#include <catch2/catch_test_macros.hpp>
#include "ui/progress_bar.h"

TEST_CASE("format_progress_bar", "[progress_bar]") {
    SECTION("Empty bar at 0%") {
        auto result = monitor::ui::format_progress_bar(0.0, 10, "■", "□");
        REQUIRE(result == "□□□□□□□□□□");
    }

    SECTION("Full bar at 100%") {
        auto result = monitor::ui::format_progress_bar(100.0, 10, "■", "□");
        REQUIRE(result == "■■■■■■■■■■");
    }

    SECTION("Half bar at 50%") {
        auto result = monitor::ui::format_progress_bar(50.0, 10, "■", "□");
        REQUIRE(result == "■■■■■□□□□□");
    }

    SECTION("Clamps values above 100%") {
        auto result = monitor::ui::format_progress_bar(150.0, 10, "■", "□");
        REQUIRE(result == "■■■■■■■■■■");
    }

    SECTION("Clamps values below 0%") {
        auto result = monitor::ui::format_progress_bar(-10.0, 10, "■", "□");
        REQUIRE(result == "□□□□□□□□□□");
    }
}

TEST_CASE("threshold_color", "[progress_bar]") {
    auto low = ftxui::Color::Green;
    auto medium = ftxui::Color::Yellow;
    auto high = ftxui::Color::Red;

    SECTION("Low threshold") {
        auto color = monitor::ui::threshold_color(30.0, low, medium, high);
        REQUIRE(color == low);
    }

    SECTION("Medium threshold") {
        auto color = monitor::ui::threshold_color(60.0, low, medium, high);
        REQUIRE(color == medium);
    }

    SECTION("High threshold") {
        auto color = monitor::ui::threshold_color(90.0, low, medium, high);
        REQUIRE(color == high);
    }

    SECTION("Boundary at 50%") {
        auto color = monitor::ui::threshold_color(50.0, low, medium, high);
        REQUIRE(color == medium);
    }

    SECTION("Boundary at 80%") {
        auto color = monitor::ui::threshold_color(80.0, low, medium, high);
        REQUIRE(color == high);
    }
}
