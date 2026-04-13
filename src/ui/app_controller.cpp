#include "ui/app_controller.h"

#include <utility>

namespace monitor::ui {

AppController::AppController(app::AppConfig config)
    : config_(config), focus_(config.default_focus) {}

void AppController::handle_key(int key) {
    if (mode_ == InputMode::Filter && key == 27) {
        filter_query_.clear();
        mode_ = InputMode::Normal;
        return;
    }

    if (key == '/') {
        mode_ = InputMode::Filter;
        return;
    }

    if (key == 'h') {
        focus_ = focus_ == app::FocusZone::Cpu ? app::FocusZone::CommandBar
                                               : static_cast<app::FocusZone>(static_cast<int>(focus_) - 1);
    } else if (key == 'l') {
        focus_ = focus_ == app::FocusZone::CommandBar ? app::FocusZone::Cpu
                                                      : static_cast<app::FocusZone>(static_cast<int>(focus_) + 1);
    } else if (key == 's') {
        sort_key_ = sort_key_ == collector::ProcessSortKey::Name
                        ? collector::ProcessSortKey::Cpu
                        : static_cast<collector::ProcessSortKey>(static_cast<int>(sort_key_) + 1);
    }
}

void AppController::handle_text(std::string text) {
    if (mode_ == InputMode::Filter) {
        filter_query_ = std::move(text);
    }
}

app::FocusZone AppController::focus() const { return focus_; }
InputMode AppController::mode() const { return mode_; }
std::string AppController::filter_query() const { return filter_query_; }
collector::ProcessSortKey AppController::sort_key() const { return sort_key_; }

}  // namespace monitor::ui
