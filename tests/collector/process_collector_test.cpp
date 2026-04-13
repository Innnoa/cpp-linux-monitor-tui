#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "collector/process_collector.h"

TEST_CASE("process collector parses /proc status and stat lines") {
    constexpr std::string_view stat =
        "812 (postgres) R 1 1 1 0 0 0 0 0 0 0 300 200 0 0 5 7 1 0 0 8192 0\n";
    constexpr std::string_view status =
        "Name:\tpostgres\n"
        "State:\tR (running)\n"
        "Uid:\t0\t0\t0\t0\n"
        "VmRSS:\t2048 kB\n";

    const auto info = monitor::collector::parse_process_info(
        /*pid=*/812,
        stat,
        status,
        /*total_memory_bytes=*/8ULL * 1024ULL * 1024ULL,
        /*clock_ticks_per_second=*/100);

    CHECK(info.pid == 812);
    CHECK(info.name == "postgres");
    CHECK(info.state == 'R');
    CHECK(info.nice_value == 7);
    CHECK(info.memory_percent == Catch::Approx(25.0).margin(0.001));
}

TEST_CASE("process collector handles malformed stat text safely") {
    constexpr std::string_view stat = "";
    constexpr std::string_view status =
        "Uid:\t0\t0\t0\t0\n"
        "VmRSS:\t2048 kB\n";

    const auto info = monitor::collector::parse_process_info(
        /*pid=*/42,
        stat,
        status,
        /*total_memory_bytes=*/0,
        /*clock_ticks_per_second=*/100);

    CHECK(info.pid == 42);
    CHECK(info.name.empty());
    CHECK(info.state == 'S');
    CHECK(info.nice_value == 0);
    CHECK(info.memory_percent == 0.0);
}

TEST_CASE("process collector skips memory percent when total memory is zero") {
    constexpr std::string_view stat =
        "812 (postgres) R 1 1 1 0 0 0 0 0 0 0 300 200 0 0 5 7 1 0 0 8192 0\n";
    constexpr std::string_view status =
        "Uid:\t0\t0\t0\t0\n"
        "VmRSS:\t2048 kB\n";

    const auto info = monitor::collector::parse_process_info(
        /*pid=*/812,
        stat,
        status,
        /*total_memory_bytes=*/0,
        /*clock_ticks_per_second=*/100);

    CHECK(info.name == "postgres");
    CHECK(info.state == 'R');
    CHECK(info.nice_value == 7);
    CHECK(info.memory_percent == 0.0);
}
