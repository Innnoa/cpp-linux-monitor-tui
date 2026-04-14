#pragma once

#include <cstddef>
#include <string>

#include "app/app_config.h"
#include "collector/process_collector.h"

namespace monitor::ui {

enum class InputMode {
    Normal,
    Filter,
    Command,
    ConfirmKill,
    Renice,
};

class AppController {
  public:
    explicit AppController(app::AppConfig config);

    void handle_key(int key);
    void handle_text(std::string text);
    void execute_command(std::string text);
    void begin_kill(int pid);
    void begin_renice(int pid);
    void confirm_kill();
    void submit_renice(int nice_value);
    void set_status_text(std::string text);
    void set_visible_process_count(std::size_t count);
    void set_process_window_height(std::size_t height);

    [[nodiscard]] app::FocusZone focus() const;
    [[nodiscard]] InputMode mode() const;
    [[nodiscard]] std::chrono::milliseconds refresh_interval() const;
    [[nodiscard]] std::string command_text() const;
    [[nodiscard]] std::string status_text() const;
    [[nodiscard]] std::string filter_query() const;
    [[nodiscard]] collector::ProcessSortKey sort_key() const;
    [[nodiscard]] int selected_pid() const;
    [[nodiscard]] std::size_t selected_process_index() const;
    [[nodiscard]] std::size_t process_window_start() const;
    [[nodiscard]] std::size_t process_window_height() const;
    [[nodiscard]] bool should_quit() const;

  private:
    void finish_command(std::string status_text);

    app::AppConfig config_;
    app::FocusZone focus_;
    InputMode mode_{InputMode::Normal};
    collector::ProcessSortKey sort_key_{collector::ProcessSortKey::Cpu};
    std::string filter_query_;
    std::string command_text_;
    std::string status_text_{"ready"};
    int selected_pid_{0};
    std::size_t visible_process_count_{0};
    std::size_t selected_process_index_{0};
    std::size_t process_window_start_{0};
    std::size_t process_window_height_{5};
    bool should_quit_{false};
};

}  // namespace monitor::ui
