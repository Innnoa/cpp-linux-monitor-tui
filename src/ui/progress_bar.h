#pragma once

#include <string>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace monitor::ui {

struct ProgressBarConfig {
    int width = 20;
    std::string filled_char = "■";
    std::string empty_char = "□";
    bool show_percentage = true;
    bool show_value = false;
};

ftxui::Element progress_bar(
    double value,
    double max_value,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color);

ftxui::Element progress_bar(
    double percentage,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color);

std::string format_progress_bar(
    double percentage,
    int width,
    const std::string& filled_char,
    const std::string& empty_char);

ftxui::Color threshold_color(
    double percentage,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color);

}  // namespace monitor::ui
