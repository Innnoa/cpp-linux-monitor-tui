#include "ui/sparkline_chart.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

namespace monitor::ui {

namespace {

constexpr std::string_view kSparkChars[] = {
    "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"
};
constexpr int kSparkLevels = 8;

std::string_view spark_char(double normalized) {
    const auto index = std::clamp(
        static_cast<int>(normalized * kSparkLevels), 0, kSparkLevels - 1);
    return kSparkChars[index];
}

}  // namespace

std::string format_sparkline(
    std::span<const double> data,
    int width) {
    if (data.empty() || width <= 0) {
        return {};
    }

    std::vector<double> filtered;
    filtered.reserve(data.size());
    for (const auto v : data) {
        if (std::isfinite(v)) {
            filtered.push_back(v);
        }
    }

    if (filtered.empty()) {
        return {};
    }

    const auto [min_it, max_it] = std::minmax_element(filtered.begin(), filtered.end());
    const auto min_val = *min_it;
    const auto max_val = *max_it;
    const auto range = max_val - min_val;

    std::string result;
    result.reserve(width * 3);

    const auto step = static_cast<double>(filtered.size()) / width;
    for (int i = 0; i < width; ++i) {
        const auto index = static_cast<std::size_t>(i * step);
        if (index >= filtered.size()) {
            break;
        }

        const auto value = filtered[index];
        const auto normalized = (range > 0.0) ? ((value - min_val) / range) : 0.5;
        result.append(spark_char(normalized));
    }

    return result;
}

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    const SparklineConfig& config) {
    if (data.empty()) {
        return ftxui::text(std::string(config.width, ' '));
    }

    const auto sparkline = format_sparkline(data, config.width);

    std::ostringstream display;
    display << sparkline;

    if (config.show_latest && !data.empty()) {
        display << " " << std::fixed << std::setprecision(1) << data.back();
    }

    return ftxui::text(display.str()) | ftxui::color(color);
}

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    int width) {
    SparklineConfig config;
    config.width = width;
    return sparkline_chart(data, color, config);
}

}  // namespace monitor::ui
