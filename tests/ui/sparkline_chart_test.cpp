#include <catch2/catch_test_macros.hpp>
#include "ui/sparkline_chart.h"
#include <vector>

TEST_CASE("format_sparkline", "[sparkline]") {
    SECTION("Empty data returns empty string") {
        std::vector<double> data;
        auto result = monitor::ui::format_sparkline(data, 10);
        REQUIRE(result.empty());
    }

    SECTION("Single value returns single char") {
        std::vector<double> data = {50.0};
        auto result = monitor::ui::format_sparkline(data, 10);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("Constant values returns middle spark") {
        std::vector<double> data = {5.0, 5.0, 5.0, 5.0, 5.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("Increasing trend") {
        std::vector<double> data = {0.0, 25.0, 50.0, 75.0, 100.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("Width larger than data") {
        std::vector<double> data = {1.0, 2.0, 3.0};
        auto result = monitor::ui::format_sparkline(data, 10);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("Width smaller than data") {
        std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("Zero width returns empty") {
        std::vector<double> data = {1.0, 2.0, 3.0};
        auto result = monitor::ui::format_sparkline(data, 0);
        REQUIRE(result.empty());
    }

    SECTION("Negative width returns empty") {
        std::vector<double> data = {1.0, 2.0, 3.0};
        auto result = monitor::ui::format_sparkline(data, -1);
        REQUIRE(result.empty());
    }
}

TEST_CASE("sparkline_chart element", "[sparkline]") {
    monitor::ui::SparklineConfig config;

    SECTION("Empty data renders blank space") {
        std::vector<double> data;
        auto el = monitor::ui::sparkline_chart(data, ftxui::Color::Green, config);
        REQUIRE(el != nullptr);
    }

    SECTION("Non-empty data renders sparkline") {
        std::vector<double> data = {10.0, 20.0, 30.0, 40.0, 50.0};
        auto el = monitor::ui::sparkline_chart(data, ftxui::Color::Green, config);
        REQUIRE(el != nullptr);
    }

    SECTION("Width overload works") {
        std::vector<double> data = {10.0, 20.0, 30.0};
        auto el = monitor::ui::sparkline_chart(data, ftxui::Color::Blue, 20);
        REQUIRE(el != nullptr);
    }

    SECTION("Show latest disabled") {
        config.show_latest = false;
        std::vector<double> data = {10.0, 20.0, 30.0};
        auto el = monitor::ui::sparkline_chart(data, ftxui::Color::Green, config);
        REQUIRE(el != nullptr);
    }
}
