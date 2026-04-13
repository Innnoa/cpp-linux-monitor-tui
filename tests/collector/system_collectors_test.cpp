#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "collector/cpu_collector.h"
#include "collector/disk_collector.h"
#include "collector/memory_collector.h"
#include "collector/network_collector.h"

TEST_CASE("cpu collector computes total percent from two samples") {
    constexpr std::string_view first =
        "cpu  100 0 50 850 50 0 0 0 0 0\n"
        "cpu0 50 0 25 425 25 0 0 0 0 0 0\n";
    constexpr std::string_view second =
        "cpu  130 0 70 900 50 0 0 0 0 0\n"
        "cpu0 65 0 35 450 25 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_cpu_metrics(
        monitor::collector::parse_cpu_sample(first),
        monitor::collector::parse_cpu_sample(second));

    CHECK(metrics.total_percent == Catch::Approx(50.0));
    CHECK(metrics.core_percents.front() == Catch::Approx(50.0));
}

TEST_CASE("memory collector reports used and swap bytes") {
    constexpr std::string_view meminfo =
        "MemTotal:       65536000 kB\n"
        "MemAvailable:   32768000 kB\n"
        "SwapTotal:       8192000 kB\n"
        "SwapFree:        6144000 kB\n";

    const auto metrics = monitor::collector::parse_memory_info(meminfo);

    CHECK(metrics.total_bytes == 65536000ULL * 1024ULL);
    CHECK(metrics.used_bytes == 32768000ULL * 1024ULL);
    CHECK(metrics.swap_used_bytes == 2048000ULL * 1024ULL);
}

TEST_CASE("disk collector computes throughput from deltas") {
    constexpr std::string_view first = "   8       0 sda 1 0 100 0 1 0 200 0 0 0 0 0 0 0 0 0 0\n";
    constexpr std::string_view second = "   8       0 sda 1 0 300 0 1 0 500 0 0 0 0 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_disk_metrics(
        monitor::collector::parse_disk_stats(first),
        monitor::collector::parse_disk_stats(second),
        "/");

    CHECK(metrics.front().label == "/");
    CHECK(metrics.front().read_bytes_per_sec == 102400);
    CHECK(metrics.front().write_bytes_per_sec == 153600);
}

TEST_CASE("network collector computes rx and tx deltas") {
    constexpr std::string_view first =
        "Inter-|   Receive                                                |  Transmit\n"
        " eth0: 1000 0 0 0 0 0 0 0 2000 0 0 0 0 0 0 0\n";
    constexpr std::string_view second =
        "Inter-|   Receive                                                |  Transmit\n"
        " eth0: 4000 0 0 0 0 0 0 0 2600 0 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_network_metrics(
        monitor::collector::parse_network_stats(first),
        monitor::collector::parse_network_stats(second));

    CHECK(metrics.front().interface_name == "eth0");
    CHECK(metrics.front().rx_bytes_per_sec == 3000);
    CHECK(metrics.front().tx_bytes_per_sec == 600);
}

TEST_CASE("cpu collector tolerates mismatched core counts") {
    constexpr std::string_view first =
        "cpu  100 0 50 850 50 0 0 0 0 0\n"
        "cpu0 50 0 25 425 25 0 0 0 0 0 0\n"
        "cpu1 60 0 30 425 25 0 0 0 0 0 0\n";
    constexpr std::string_view second =
        "cpu  130 0 70 900 50 0 0 0 0 0\n"
        "cpu0 65 0 35 450 25 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_cpu_metrics(
        monitor::collector::parse_cpu_sample(first),
        monitor::collector::parse_cpu_sample(second));

    CHECK(metrics.core_percents.size() == 1);
    CHECK(metrics.core_percents.front() == Catch::Approx(50.0));
}

TEST_CASE("disk collector handles reordered devices") {
    constexpr std::string_view first =
        "   8       0 sda 1 0 100 0 1 0 200 0 0 0 0 0 0 0 0 0 0\n"
        "   8      16 sdb 1 0 200 0 1 0 400 0 0 0 0 0 0 0 0 0 0\n";
    constexpr std::string_view second =
        "   8      16 sdb 1 0 500 0 1 0 600 0 0 0 0 0 0 0 0 0 0\n"
        "   8       0 sda 1 0 300 0 1 0 400 0 0 0 0 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_disk_metrics(
        monitor::collector::parse_disk_stats(first),
        monitor::collector::parse_disk_stats(second),
        "/");

    REQUIRE(metrics.size() == 2);
    CHECK(metrics[0].read_bytes_per_sec == 153600);
    CHECK(metrics[0].write_bytes_per_sec == 102400);
    CHECK(metrics[1].read_bytes_per_sec == 102400);
    CHECK(metrics[1].write_bytes_per_sec == 102400);
}

TEST_CASE("network collector aligns interfaces by name") {
    constexpr std::string_view first =
        "Inter-|   Receive                                                |  Transmit\n"
        " eth1: 1000 0 0 0 0 0 0 0 2000 0 0 0 0 0 0 0\n"
        " eth0: 500 0 0 0 0 0 0 0 1000 0 0 0 0 0 0 0\n";
    constexpr std::string_view second =
        "Inter-|   Receive                                                |  Transmit\n"
        " eth0: 1500 0 0 0 0 0 0 0 1300 0 0 0 0 0 0 0\n"
        " eth1: 2000 0 0 0 0 0 0 0 2600 0 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_network_metrics(
        monitor::collector::parse_network_stats(first),
        monitor::collector::parse_network_stats(second));

    REQUIRE(metrics.size() == 2);
    CHECK(metrics[0].interface_name == "eth0");
    CHECK(metrics[0].rx_bytes_per_sec == 1000);
    CHECK(metrics[0].tx_bytes_per_sec == 300);
    CHECK(metrics[1].interface_name == "eth1");
    CHECK(metrics[1].rx_bytes_per_sec == 1000);
    CHECK(metrics[1].tx_bytes_per_sec == 600);
}

TEST_CASE("memory collector clamps underflow scenarios") {
    constexpr std::string_view meminfo =
        "MemTotal:       100 kB\n"
        "MemAvailable:   200 kB\n"
        "SwapTotal:       50 kB\n"
        "SwapFree:        70 kB\n";

    const auto metrics = monitor::collector::parse_memory_info(meminfo);

    CHECK(metrics.total_bytes == 100ULL * 1024ULL);
    CHECK(metrics.used_bytes == 0ULL);
    CHECK(metrics.swap_total_bytes == 50ULL * 1024ULL);
    CHECK(metrics.swap_used_bytes == 0ULL);
}

TEST_CASE("cpu collector returns zero on total rollback") {
    constexpr std::string_view first =
        "cpu  200 0 50 700 0\n"
        "cpu0 100 0 25 350 0\n";
    constexpr std::string_view second =
        "cpu  100 0 25 400 0\n"
        "cpu0 50 0 12 225 0\n";

    const auto metrics = monitor::collector::compute_cpu_metrics(
        monitor::collector::parse_cpu_sample(first),
        monitor::collector::parse_cpu_sample(second));

    CHECK(metrics.total_percent == 0.0);
    CHECK(metrics.core_percents.front() == 0.0);
}

TEST_CASE("disk collector clamps sector rollback") {
    constexpr std::string_view first =
        "   8       0 sda 1 0 500 0 1 0 900 0 0 0 0 0 0 0 0 0 0\n";
    constexpr std::string_view second =
        "   8       0 sda 1 0 300 0 1 0 600 0 0 0 0 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_disk_metrics(
        monitor::collector::parse_disk_stats(first),
        monitor::collector::parse_disk_stats(second),
        "/");

    REQUIRE(metrics.size() == 1);
    CHECK(metrics.front().read_bytes_per_sec == 0);
    CHECK(metrics.front().write_bytes_per_sec == 0);
}

TEST_CASE("network collector clamps byte rollback") {
    constexpr std::string_view first =
        "Inter-|   Receive                                                |  Transmit\n"
        " eth0: 4000 0 0 0 0 0 0 0 8000 0 0 0 0 0 0 0\n";
    constexpr std::string_view second =
        "Inter-|   Receive                                                |  Transmit\n"
        " eth0: 3000 0 0 0 0 0 0 0 6000 0 0 0 0 0 0 0\n";

    const auto metrics = monitor::collector::compute_network_metrics(
        monitor::collector::parse_network_stats(first),
        monitor::collector::parse_network_stats(second));

    REQUIRE(metrics.size() == 1);
    CHECK(metrics.front().rx_bytes_per_sec == 0);
    CHECK(metrics.front().tx_bytes_per_sec == 0);
}
