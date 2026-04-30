#include <catch2/catch_test_macros.hpp>
#include "model/history_data.h"

TEST_CASE("RingBuffer basic operations", "[ring_buffer]") {
    monitor::model::RingBuffer<int, 3> buffer;

    SECTION("Initial state is empty") {
        REQUIRE(buffer.size() == 0);
        REQUIRE(buffer.full() == false);
    }

    SECTION("Push increases size") {
        buffer.push(1);
        REQUIRE(buffer.size() == 1);
        REQUIRE(buffer.full() == false);

        buffer.push(2);
        REQUIRE(buffer.size() == 2);

        buffer.push(3);
        REQUIRE(buffer.size() == 3);
        REQUIRE(buffer.full() == true);
    }

    SECTION("Push beyond capacity overwrites oldest") {
        buffer.push(1);
        buffer.push(2);
        buffer.push(3);
        buffer.push(4);

        REQUIRE(buffer.size() == 3);
        REQUIRE(buffer.full() == true);

        auto data = buffer.data();
        REQUIRE(data[0] == 2);
        REQUIRE(data[1] == 3);
        REQUIRE(data[2] == 4);
    }

    SECTION("Latest returns most recent value") {
        buffer.push(10);
        REQUIRE(buffer.latest() == 10);

        buffer.push(20);
        REQUIRE(buffer.latest() == 20);

        buffer.push(30);
        REQUIRE(buffer.latest() == 30);

        buffer.push(40);
        REQUIRE(buffer.latest() == 40);
    }

    SECTION("Clear resets buffer") {
        buffer.push(1);
        buffer.push(2);
        buffer.clear();

        REQUIRE(buffer.size() == 0);
        REQUIRE(buffer.full() == false);
    }
}

TEST_CASE("HistoryData operations", "[history_data]") {
    monitor::model::HistoryData history;

    SECTION("Initial state is empty") {
        REQUIRE(history.cpu_history.size() == 0);
        REQUIRE(history.memory_history.size() == 0);
    }

    SECTION("Push and retrieve data") {
        history.cpu_history.push(45.5);
        history.cpu_history.push(50.2);

        REQUIRE(history.cpu_history.size() == 2);
        REQUIRE(history.cpu_history.latest() == 50.2);
    }

    SECTION("Clear resets all histories") {
        history.cpu_history.push(1.0);
        history.memory_history.push(2.0);
        history.clear();

        REQUIRE(history.cpu_history.size() == 0);
        REQUIRE(history.memory_history.size() == 0);
    }
}
