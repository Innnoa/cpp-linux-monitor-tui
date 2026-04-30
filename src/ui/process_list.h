#pragma once

#include <cstddef>
#include <ftxui/dom/elements.hpp>

#include "model/system_snapshot.h"
#include "ui/theme.h"

namespace monitor::ui {

struct ProcessColorScheme {
    ftxui::Color high_cpu = ftxui::Color::RGB(243, 139, 168);    // theme.red
    ftxui::Color medium_cpu = ftxui::Color::RGB(250, 179, 135);  // theme.peach
    ftxui::Color high_memory = ftxui::Color::RGB(203, 166, 247); // theme.mauve
    ftxui::Color medium_memory = ftxui::Color::RGB(137, 180, 250); // theme.blue
    ftxui::Color running = ftxui::Color::RGB(166, 227, 161);     // theme.green
    ftxui::Color sleeping = ftxui::Color::RGB(127, 132, 156);    // theme.overlay1
    ftxui::Color zombie = ftxui::Color::RGB(243, 139, 168);      // theme.red
    ftxui::Color default_text = ftxui::Color::RGB(186, 194, 222); // theme.subtext1
};

ftxui::Element process_list_row(
    const model::ProcessInfo& process,
    bool is_selected,
    const ProcessColorScheme& color_scheme);

ftxui::Element process_list_header();

ftxui::Element process_list(
    std::span<const model::ProcessInfo> processes,
    std::size_t selected_index,
    std::size_t window_start,
    std::size_t window_height,
    const ProcessColorScheme& color_scheme);

std::string format_process_row(
    const model::ProcessInfo& process,
    bool is_selected);

ftxui::Color process_row_color(
    const model::ProcessInfo& process,
    const ProcessColorScheme& color_scheme);

}  // namespace monitor::ui
