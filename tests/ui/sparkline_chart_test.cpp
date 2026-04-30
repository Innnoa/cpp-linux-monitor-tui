#include <catch2/catch_test_macros.hpp>
#include "ui/sparkline_chart.h"
#include <cmath>
#include <string>
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
        REQUIRE(result.size() >= 3);  // one spark char is 3 UTF-8 bytes
    }

    SECTION("Constant values returns middle spark") {
        std::vector<double> data = {5.0, 5.0, 5.0, 5.0, 5.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE_FALSE(result.empty());
        // When range is 0, normalized=0.5 → index=4 → "▅"
        REQUIRE(result == "▅▅▅▅▅");
    }

    SECTION("Increasing trend") {
        std::vector<double> data = {0.0, 25.0, 50.0, 75.0, 100.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE_FALSE(result.empty());
        // Each spark char is 3 UTF-8 bytes; characters should be monotonically non-decreasing
        for (std::size_t i = 3; i < result.size(); i += 3) {
            REQUIRE(result[i] >= result[i - 3]);
        }
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

    SECTION("Negative values work correctly") {
        std::vector<double> data = {-100.0, -50.0, 0.0, 50.0, 100.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE_FALSE(result.empty());
        // First char should be lowest, last char should be highest
        REQUIRE(result.substr(0, 3) == "▁");
        REQUIRE(result.substr(12, 3) == "█");
    }

    SECTION("NaN values are filtered out") {
        std::vector<double> data = {1.0, std::nan(""), 3.0, std::nan(""), 5.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("All NaN returns empty") {
        std::vector<double> data = {std::nan(""), std::nan(""), std::nan("")};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE(result.empty());
    }

    SECTION("Inf values are filtered out") {
        std::vector<double> data = {1.0, std::numeric_limits<double>::infinity(), 3.0};
        auto result = monitor::ui::format_sparkline(data, 3);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("Negative Inf values are filtered out") {
        std::vector<double> data = {1.0, -std::numeric_limits<double>::infinity(), 3.0};
        auto result = monitor::ui::format_sparkline(data, 3);
        REQUIRE_FALSE(result.empty());
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
