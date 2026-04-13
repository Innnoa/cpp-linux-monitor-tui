#pragma once

#include <string>

#include "model/system_snapshot.h"
#include "ui/app_controller.h"

namespace monitor::ui {

std::string render_dashboard_to_string(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    int width,
    int height);

}  // namespace monitor::ui
