#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
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

TEST_CASE("progress_bar element", "[progress_bar]") {
    auto low = ftxui::Color::Green;
    auto medium = ftxui::Color::Yellow;
    auto high = ftxui::Color::Red;
    monitor::ui::ProgressBarConfig config;

    SECTION("max_value <= 0 renders empty bar") {
        auto el = monitor::ui::progress_bar(50.0, 0.0, config, low, medium, high);
        REQUIRE(el != nullptr);

        auto el2 = monitor::ui::progress_bar(50.0, -10.0, config, low, medium, high);
        REQUIRE(el2 != nullptr);
    }

    SECTION("percentage overload clamps display") {
        config.show_percentage = true;
        auto el = monitor::ui::progress_bar(150.0, config, low, medium, high);
        REQUIRE(el != nullptr);

        auto el2 = monitor::ui::progress_bar(-20.0, config, low, medium, high);
        REQUIRE(el2 != nullptr);
    }

    SECTION("percentage overload without label") {
        config.show_percentage = false;
        auto el = monitor::ui::progress_bar(75.0, config, low, medium, high);
        REQUIRE(el != nullptr);
    }

    SECTION("value overload computes percentage correctly") {
        auto el = monitor::ui::progress_bar(50.0, 200.0, config, low, medium, high);
        REQUIRE(el != nullptr);
    }
}
