#pragma once

#include <string>
#include <string_view>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace monitor::ui {

struct ThemeColors {
    ftxui::Color rosewater;
    ftxui::Color mauve;
    ftxui::Color red;
    ftxui::Color peach;
    ftxui::Color green;
    ftxui::Color sapphire;
    ftxui::Color blue;
    ftxui::Color text;
    ftxui::Color subtext1;
    ftxui::Color overlay1;
    ftxui::Color surface2;
    ftxui::Color surface1;
    ftxui::Color surface0;
    ftxui::Color base;
    ftxui::Color mantle;
    ftxui::Color crust;
};

inline const ThemeColors& catppuccin_mocha() {
    static const ThemeColors theme{
        .rosewater = ftxui::Color::RGB(245, 224, 220),
        .mauve = ftxui::Color::RGB(203, 166, 247),
        .red = ftxui::Color::RGB(243, 139, 168),
        .peach = ftxui::Color::RGB(250, 179, 135),
        .green = ftxui::Color::RGB(166, 227, 161),
        .sapphire = ftxui::Color::RGB(116, 199, 236),
        .blue = ftxui::Color::RGB(137, 180, 250),
        .text = ftxui::Color::RGB(205, 214, 244),
        .subtext1 = ftxui::Color::RGB(186, 194, 222),
        .overlay1 = ftxui::Color::RGB(127, 132, 156),
        .surface2 = ftxui::Color::RGB(88, 91, 112),
        .surface1 = ftxui::Color::RGB(69, 71, 90),
        .surface0 = ftxui::Color::RGB(49, 50, 68),
        .base = ftxui::Color::RGB(30, 30, 46),
        .mantle = ftxui::Color::RGB(24, 24, 37),
        .crust = ftxui::Color::RGB(17, 17, 27),
    };
    return theme;
}

inline std::string focus_title(std::string title, bool focused) {
    if (focused) {
        title += " *";
    }
    return title;
}

inline ftxui::Element themed_window(std::string title, ftxui::Element body, bool focused) {
    const auto& theme = catppuccin_mocha();
    const auto border_color = focused ? theme.blue : theme.surface1;
    const auto title_color = focused ? theme.blue : theme.text;

    return ftxui::window(ftxui::text(focus_title(std::move(title), focused)) | ftxui::color(title_color) | ftxui::bold,
                         std::move(body))
           | ftxui::color(border_color) | ftxui::bgcolor(theme.mantle);
}

inline ftxui::Color status_color(std::string_view status_text) {
    const auto& theme = catppuccin_mocha();

    if (status_text == "ready" || status_text == "ok" || status_text == "cleared"
        || status_text.starts_with("sort:") || status_text.starts_with("filter:")
        || status_text.starts_with("killed ") || status_text.starts_with("reniced ")) {
        return theme.green;
    }
    if (status_text == "quitting") {
        return theme.mauve;
    }
    if (status_text == "Filter mode" || status_text.starts_with("filter cleared")) {
        return theme.sapphire;
    }
    if (status_text.starts_with("Enter new nice value")) {
        return theme.peach;
    }
    if (status_text.starts_with("Kill PID ") || status_text.find("unknown command") != std::string_view::npos
        || status_text.find("invalid") != std::string_view::npos
        || status_text.find("permission denied") != std::string_view::npos
        || status_text.find("process no longer exists") != std::string_view::npos) {
        return theme.red;
    }
    return theme.subtext1;
}

}  // namespace monitor::ui
