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
