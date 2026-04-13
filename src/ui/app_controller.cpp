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
    if (mode_ == InputMode::Filter) {
        return;
    }

    if (mode_ == InputMode::Command && key == 27) {
        command_text_.clear();
        mode_ = InputMode::Normal;
        return;
    }
    if (mode_ == InputMode::Command) {
        return;
    }

    if ((mode_ == InputMode::ConfirmKill || mode_ == InputMode::Renice) && key == 27) {
        selected_pid_ = 0;
        status_text_ = "ready";
        mode_ = InputMode::Normal;
        return;
    }
    if (mode_ == InputMode::ConfirmKill || mode_ == InputMode::Renice) {
        return;
    }

    if (key == '/') {
        mode_ = InputMode::Filter;
        return;
    }

    if (key == ':') {
        mode_ = InputMode::Command;
        command_text_ = ":";
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
    } else if (mode_ == InputMode::Command) {
        if (text.empty()) {
            command_text_ = ":";
        } else if (text.front() == ':') {
            command_text_ = std::move(text);
        } else {
            command_text_ = ":" + text;
        }
    }
}

void AppController::begin_kill(int pid) {
    if (pid <= 0) {
        selected_pid_ = 0;
        status_text_ = "ready";
        mode_ = InputMode::Normal;
        return;
    }
    selected_pid_ = pid;
    mode_ = InputMode::ConfirmKill;
    status_text_ = "Kill PID " + std::to_string(pid) + "? [y/N]";
}

void AppController::begin_renice(int pid) {
    if (pid <= 0) {
        selected_pid_ = 0;
        status_text_ = "ready";
        mode_ = InputMode::Normal;
        return;
    }
    selected_pid_ = pid;
    mode_ = InputMode::Renice;
    status_text_ = "Enter new nice value";
}

void AppController::confirm_kill() {
    selected_pid_ = 0;
    status_text_ = "ready";
    mode_ = InputMode::Normal;
}

void AppController::submit_renice(int /*nice_value*/) {
    selected_pid_ = 0;
    status_text_ = "ready";
    mode_ = InputMode::Normal;
}

void AppController::set_status_text(std::string text) { status_text_ = std::move(text); }

app::FocusZone AppController::focus() const { return focus_; }
InputMode AppController::mode() const { return mode_; }
std::string AppController::command_text() const { return command_text_; }
std::string AppController::status_text() const { return status_text_; }
std::string AppController::filter_query() const { return filter_query_; }
collector::ProcessSortKey AppController::sort_key() const { return sort_key_; }
int AppController::selected_pid() const { return selected_pid_; }

}  // namespace monitor::ui
