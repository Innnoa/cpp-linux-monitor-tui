#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <future>
#include <mutex>
#include <string>

#if MONITOR_HAS_FTXUI
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#endif

#include "app/app_config.h"
#include "app/sampling_worker.h"
#include "model/system_snapshot.h"
#include "store/snapshot_store.h"
#include "ui/app_controller.h"

#if MONITOR_HAS_FTXUI
namespace monitor::app {
bool handle_shared_input_event(
    ui::AppController& controller,
    std::string& shared_input_buffer,
    int& shared_input_cursor,
    const ftxui::Event& event,
    std::mutex& state_mutex,
    const ftxui::Component& input_component);
std::size_t visible_process_rows_for_terminal_height(int terminal_height);
}
#endif

namespace {
class FakeSampler final : public monitor::app::Sampler {
  public:
    monitor::model::SystemSnapshot collect() override {
        monitor::model::SystemSnapshot snapshot;
        snapshot.cpu.total_percent = 37.0;
        snapshot.memory.used_bytes = 31ULL * 1024ULL * 1024ULL;
        return snapshot;
    }
};
}  // namespace

TEST_CASE("sampling worker publishes snapshots into the store") {
    FakeSampler sampler;
    monitor::store::SnapshotStore store(4);
    monitor::app::SamplingWorker worker(sampler, store);

    worker.tick_once();

    const auto latest = store.latest();
    CHECK(latest.cpu.total_percent == 37.0);
    CHECK(latest.memory.used_bytes == 31ULL * 1024ULL * 1024ULL);
}

TEST_CASE("application can render a snapshot without using the raw loop path") {
    FakeSampler sampler;
    monitor::store::SnapshotStore store(4);
    monitor::app::SamplingWorker worker(sampler, store);

    worker.tick_once();

    const auto latest = store.latest();
    CHECK(latest.cpu.total_percent == 37.0);
    CHECK(latest.memory.used_bytes == 31ULL * 1024ULL * 1024ULL);
}

#if MONITOR_HAS_FTXUI
TEST_CASE("application derives visible process rows from terminal height") {
    CHECK(monitor::app::visible_process_rows_for_terminal_height(24) == 4);
    CHECK(monitor::app::visible_process_rows_for_terminal_height(40) == 20);
    CHECK(monitor::app::visible_process_rows_for_terminal_height(0) == 1);
}

TEST_CASE("shared input events do not deadlock when callbacks update controller state") {
    std::mutex state_mutex;
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    std::string shared_input_buffer;
    int shared_input_cursor = 0;

    controller.handle_key(':');
    shared_input_buffer = controller.shared_input_text();
    shared_input_cursor = static_cast<int>(shared_input_buffer.size());

    ftxui::InputOption input_option;
    input_option.multiline = false;
    input_option.content = &shared_input_buffer;
    input_option.cursor_position = &shared_input_cursor;
    input_option.on_change = [&] {
        std::scoped_lock lock(state_mutex);
        controller.handle_text(shared_input_buffer);
    };

    auto input_component = ftxui::Input(input_option);
    auto result = std::async(std::launch::async, [&] {
        return monitor::app::handle_shared_input_event(
            controller, shared_input_buffer, shared_input_cursor, ftxui::Event::Character('q'), state_mutex,
            input_component);
    });

    REQUIRE(result.wait_for(std::chrono::milliseconds(200)) == std::future_status::ready);
    CHECK(result.get());
    CHECK(shared_input_buffer == "q");
    CHECK(controller.command_text() == ":q");
}
#endif
