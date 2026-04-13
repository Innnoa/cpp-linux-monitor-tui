#pragma once

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
    void begin_kill(int pid);
    void begin_renice(int pid);
    void confirm_kill();
    void submit_renice(int nice_value);

    [[nodiscard]] app::FocusZone focus() const;
    [[nodiscard]] InputMode mode() const;
    [[nodiscard]] std::string command_text() const;
    [[nodiscard]] std::string status_text() const;
    [[nodiscard]] std::string filter_query() const;
    [[nodiscard]] collector::ProcessSortKey sort_key() const;
    [[nodiscard]] int selected_pid() const;

  private:
    app::AppConfig config_;
    app::FocusZone focus_;
    InputMode mode_{InputMode::Normal};
    collector::ProcessSortKey sort_key_{collector::ProcessSortKey::Cpu};
    std::string filter_query_;
    std::string command_text_;
    std::string status_text_{"ready"};
    int selected_pid_{0};
};

}  // namespace monitor::ui
