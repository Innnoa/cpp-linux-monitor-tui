#pragma once

#include <span>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace monitor::ui {

struct SparklineConfig {
    int width = 40;
    bool show_latest = true;
};

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    const SparklineConfig& config = {});

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    int width);

std::string format_sparkline(
    std::span<const double> data,
    int width);

}  // namespace monitor::ui
