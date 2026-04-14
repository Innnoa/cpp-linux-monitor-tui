#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "app/app_config.h"
#include "ui/app_controller.h"

TEST_CASE("controller cycles focus and filter state with vim-style keys") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    CHECK(controller.focus() == monitor::app::FocusZone::Processes);

    controller.handle_key('h');
    CHECK(controller.focus() == monitor::app::FocusZone::Network);

    controller.handle_key('/');
    CHECK(controller.mode() == monitor::ui::InputMode::Filter);

    controller.handle_text("post");
    CHECK(controller.filter_query() == "post");

    controller.handle_key(27);
    CHECK(controller.filter_query().empty());
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);

    controller.handle_key(':');
    CHECK(controller.mode() == monitor::ui::InputMode::Command);
    controller.handle_text("sort cpu");
    CHECK(controller.command_text() == ":sort cpu");
    controller.handle_key('/');
    CHECK(controller.mode() == monitor::ui::InputMode::Command);
    CHECK(controller.command_text() == ":sort cpu");
    controller.handle_key(27);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.command_text().empty());

    controller.handle_key(':');
    controller.handle_text(":quit");
    CHECK(controller.command_text() == ":quit");
}

TEST_CASE("controller tracks selected process within visible list") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.set_visible_process_count(6);
    controller.set_process_window_height(3);
    CHECK(controller.selected_process_index() == 0);
    CHECK(controller.process_window_start() == 0);

    controller.handle_key('j');
    CHECK(controller.selected_process_index() == 1);
    CHECK(controller.process_window_start() == 0);
    controller.handle_key('j');
    CHECK(controller.selected_process_index() == 2);
    CHECK(controller.process_window_start() == 1);
    controller.handle_key('j');
    CHECK(controller.selected_process_index() == 3);
    CHECK(controller.process_window_start() == 2);

    controller.handle_key('k');
    CHECK(controller.selected_process_index() == 2);
    CHECK(controller.process_window_start() == 1);

    controller.set_visible_process_count(1);
    CHECK(controller.selected_process_index() == 0);
    CHECK(controller.process_window_start() == 0);

    controller.set_visible_process_count(0);
    CHECK(controller.selected_process_index() == 0);
    CHECK(controller.process_window_start() == 0);
}

TEST_CASE("controller executes internal commands") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.handle_key(':');
    controller.handle_text("sort name");
    controller.execute_command("sort name");
    CHECK(controller.sort_key() == monitor::collector::ProcessSortKey::Name);
    CHECK(controller.status_text() == "sort: name");
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.command_text().empty());

    controller.execute_command("filter postgres");
    CHECK(controller.filter_query() == "postgres");
    CHECK(controller.status_text() == "filter: postgres");

    controller.execute_command("clear");
    CHECK(controller.filter_query().empty());
    CHECK(controller.status_text() == "cleared");

    controller.execute_command("quit");
    CHECK(controller.should_quit());
    CHECK(controller.status_text() == "quitting");
}

TEST_CASE("controller reports unknown commands with examples") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.execute_command("srot cpu");

    CHECK_FALSE(controller.should_quit());
    CHECK(controller.status_text() == "unknown command: srot cpu (try: sort cpu, filter postgres, clear, quit)");
}

TEST_CASE("controller executes action commands through injected executor") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    std::string captured_command;
    int captured_pid = 0;
    std::optional<int> captured_value;

    const auto executor = [&](std::string_view command, int pid, std::optional<int> value) {
        captured_command = std::string{command};
        captured_pid = pid;
        captured_value = value;
        return std::string{"ok"};
    };

    controller.execute_command("kill 812", executor);
    CHECK(captured_command == "kill");
    CHECK(captured_pid == 812);
    CHECK_FALSE(captured_value.has_value());
    CHECK(controller.status_text() == "ok");

    controller.execute_command("renice 812 5", executor);
    CHECK(captured_command == "renice");
    CHECK(captured_pid == 812);
    REQUIRE(captured_value.has_value());
    CHECK(*captured_value == 5);
    CHECK(controller.status_text() == "ok");
}

TEST_CASE("controller rejects malformed action commands") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.execute_command("kill abc");
    CHECK(controller.status_text() == "unknown command: kill abc (try: sort cpu, filter postgres, clear, quit, kill 1234, renice 1234 5)");

    controller.execute_command("renice 123");
    CHECK(controller.status_text() == "unknown command: renice 123 (try: sort cpu, filter postgres, clear, quit, kill 1234, renice 1234 5)");

    controller.execute_command("renice 123 abc");
    CHECK(controller.status_text() == "unknown command: renice 123 abc (try: sort cpu, filter postgres, clear, quit, kill 1234, renice 1234 5)");
}
TEST_CASE("controller isolates confirm kill mode") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.begin_kill(4242);
    CHECK(controller.mode() == monitor::ui::InputMode::ConfirmKill);
    CHECK(controller.selected_pid() == 4242);
    CHECK(controller.status_text() == "Kill PID 4242? [y/N]");

    const auto starting_focus = controller.focus();
    const auto starting_sort = controller.sort_key();

    controller.handle_key('/');
    CHECK(controller.mode() == monitor::ui::InputMode::ConfirmKill);

    controller.handle_key('h');
    CHECK(controller.focus() == starting_focus);
    controller.handle_key('l');
    CHECK(controller.focus() == starting_focus);
    controller.handle_key('s');
    CHECK(controller.sort_key() == starting_sort);

    controller.handle_key(27);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.status_text() == "ready");
    CHECK(controller.selected_pid() == 0);
}

TEST_CASE("controller isolates renice mode") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.begin_renice(900);
    CHECK(controller.mode() == monitor::ui::InputMode::Renice);
    CHECK(controller.selected_pid() == 900);
    CHECK(controller.status_text() == "Enter new nice value");

    controller.handle_key(':');
    CHECK(controller.mode() == monitor::ui::InputMode::Renice);
    CHECK(controller.command_text().empty());

    controller.submit_renice(5);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.status_text() == "ready");
    CHECK(controller.selected_pid() == 0);
}

TEST_CASE("controller isolates confirm and renice modes") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.begin_kill(812);
    CHECK(controller.mode() == monitor::ui::InputMode::ConfirmKill);
    CHECK(controller.selected_pid() == 812);
    CHECK(controller.status_text() == "Kill PID 812? [y/N]");
    controller.handle_key('/');
    CHECK(controller.mode() == monitor::ui::InputMode::ConfirmKill);
    CHECK(controller.selected_pid() == 812);
    controller.handle_key(27);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.selected_pid() == 0);
    CHECK(controller.status_text() == "ready");

    controller.begin_renice(441);
    CHECK(controller.mode() == monitor::ui::InputMode::Renice);
    CHECK(controller.selected_pid() == 441);
    CHECK(controller.status_text() == "Enter new nice value");
    controller.handle_key(':');
    CHECK(controller.mode() == monitor::ui::InputMode::Renice);
    controller.submit_renice(5);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.selected_pid() == 0);
    CHECK(controller.status_text() == "ready");

    controller.begin_kill(0);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.selected_pid() == 0);
    CHECK(controller.status_text() == "ready");

    controller.begin_renice(-1);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.selected_pid() == 0);
    CHECK(controller.status_text() == "ready");
}
