#include "ui/progress_bar.h"

#include <iomanip>
#include <sstream>
#include <algorithm>

namespace monitor::ui {

namespace {

constexpr double kLowThreshold = 50.0;
constexpr double kMediumThreshold = 80.0;

}  // namespace

ftxui::Color threshold_color(
    double percentage,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color) {
    if (percentage >= kMediumThreshold) {
        return high_color;
    }
    if (percentage >= kLowThreshold) {
        return medium_color;
    }
    return low_color;
}

std::string format_progress_bar(
    double percentage,
    int width,
    const std::string& filled_char,
    const std::string& empty_char) {
    const auto clamped = std::clamp(percentage, 0.0, 100.0);
    const auto filled_count = static_cast<int>((clamped / 100.0) * width);
    const auto empty_count = width - filled_count;

    std::string result;
    for (int i = 0; i < filled_count; ++i) {
        result += filled_char;
    }
    for (int i = 0; i < empty_count; ++i) {
        result += empty_char;
    }
    return result;
}

ftxui::Element progress_bar(
    double value,
    double max_value,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color) {
    if (max_value <= 0.0) {
        std::string empty_bar;
        for (int i = 0; i < config.width; ++i) {
            empty_bar += config.empty_char;
        }
        return ftxui::text(empty_bar);
    }

    const auto percentage = (value / max_value) * 100.0;
    return progress_bar(percentage, config, low_color, medium_color, high_color);
}

ftxui::Element progress_bar(
    double percentage,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color) {
    const auto bar = format_progress_bar(
        percentage, config.width, config.filled_char, config.empty_char);

    const auto color = threshold_color(percentage, low_color, medium_color, high_color);

    const auto clamped_pct = std::clamp(percentage, 0.0, 100.0);
    std::ostringstream display;
    display << bar;
    if (config.show_percentage) {
        display << " " << std::fixed << std::setprecision(0) << clamped_pct << "%";
    }

    return ftxui::text(display.str()) | ftxui::color(color);
}

}  // namespace monitor::ui
