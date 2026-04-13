#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "app/app_config.h"

TEST_CASE("default app config matches MVP contract") {
    const auto config = monitor::app::AppConfig::defaults();

    CHECK(config.refresh_interval == std::chrono::milliseconds{1000});
    CHECK(config.history_size == 60);
    CHECK(config.default_focus == monitor::app::FocusZone::Processes);
}
