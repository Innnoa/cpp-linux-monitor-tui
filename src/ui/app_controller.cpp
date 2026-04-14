#include "ui/app_controller.h"

#include <utility>

namespace monitor::ui {

namespace {
constexpr std::size_t kScrollContextLines = 1;
}

AppController::AppController(app::AppConfig config)
    : config_(config), focus_(config.default_focus) {}

void AppController::handle_key(int key) {
    if (mode_ == InputMode::Filter && key == 27) {
        filter_query_.clear();
        status_text_ = "ready";
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
        status_text_ = "Filter mode";
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
    } else if (key == 'j') {
        if (selected_process_index_ + 1 < visible_process_count_) {
            ++selected_process_index_;
        }
        if (visible_process_count_ > process_window_height_ && process_window_height_ > 0
            && selected_process_index_ + kScrollContextLines + 1 > process_window_start_ + process_window_height_) {
            process_window_start_ =
                selected_process_index_ + kScrollContextLines + 1 - process_window_height_;
        }
    } else if (key == 'k') {
        if (selected_process_index_ > 0) {
            --selected_process_index_;
        }
        if (visible_process_count_ > process_window_height_
            && selected_process_index_ < process_window_start_ + kScrollContextLines) {
            process_window_start_ =
                (selected_process_index_ > kScrollContextLines) ? (selected_process_index_ - kScrollContextLines) : 0;
        }
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

void AppController::set_visible_process_count(std::size_t count) {
    visible_process_count_ = count;
    if (visible_process_count_ == 0 || selected_process_index_ >= visible_process_count_) {
        selected_process_index_ = 0;
    }
    const auto max_window_start =
        (visible_process_count_ > process_window_height_) ? (visible_process_count_ - process_window_height_) : 0;
    if (process_window_start_ > max_window_start) {
        process_window_start_ = max_window_start;
    }
}

void AppController::set_process_window_height(std::size_t height) {
    process_window_height_ = (height == 0) ? 1 : height;
    const auto max_window_start =
        (visible_process_count_ > process_window_height_) ? (visible_process_count_ - process_window_height_) : 0;
    if (process_window_start_ > max_window_start) {
        process_window_start_ = max_window_start;
    }
}

app::FocusZone AppController::focus() const { return focus_; }
InputMode AppController::mode() const { return mode_; }
std::chrono::milliseconds AppController::refresh_interval() const { return config_.refresh_interval; }
std::string AppController::command_text() const { return command_text_; }
std::string AppController::status_text() const { return status_text_; }
std::string AppController::filter_query() const { return filter_query_; }
collector::ProcessSortKey AppController::sort_key() const { return sort_key_; }
int AppController::selected_pid() const { return selected_pid_; }
std::size_t AppController::selected_process_index() const { return selected_process_index_; }
std::size_t AppController::process_window_start() const { return process_window_start_; }
std::size_t AppController::process_window_height() const { return process_window_height_; }

}  // namespace monitor::ui
