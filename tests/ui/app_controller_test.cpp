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
