#pragma once

#include <string>

#if MONITOR_HAS_FTXUI
#include <ftxui/dom/elements.hpp>
#endif

#include "model/system_snapshot.h"
#include "ui/app_controller.h"

namespace monitor::ui {

#if MONITOR_HAS_FTXUI
ftxui::Element render_dashboard_body_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller);

ftxui::Element render_dashboard_bottom_bar_document(
    const AppController& controller);

ftxui::Element render_dashboard_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    int width,
    int height);
#endif

std::string render_dashboard_to_string(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    int width,
    int height);

}  // namespace monitor::ui
