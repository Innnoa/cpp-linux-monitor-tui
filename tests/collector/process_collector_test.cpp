#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "collector/process_collector.h"

TEST_CASE("process collector parses /proc status and stat lines") {
    constexpr std::string_view stat =
        "812 (postgres) R 1 1 1 0 0 0 0 0 0 0 0 300 200 0 0 5 0 1 0 0 8192 0\n";
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
    CHECK(info.memory_percent == Catch::Approx(0.0244).margin(0.001));
}
