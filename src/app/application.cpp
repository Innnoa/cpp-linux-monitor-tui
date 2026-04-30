#include "app/application.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <sys/statvfs.h>
#include <unistd.h>
#include <vector>

#include "app/sampling_worker.h"
#include "actions/process_actions.h"
#include "model/history_data.h"
#include "collector/cpu_collector.h"
#include "collector/disk_collector.h"
#include "collector/memory_collector.h"
#include "collector/network_collector.h"
#include "collector/process_collector.h"
#if MONITOR_HAS_FTXUI
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/terminal.hpp>

#include "ui/dashboard_view.h"
#include "ui/theme.h"
#endif

namespace monitor::app {

#if MONITOR_HAS_FTXUI
std::size_t visible_process_rows_for_terminal_height(int terminal_height) {
    constexpr int kBottomBarHeight = 3;
    constexpr int kHeaderHeight = 1;
    constexpr int kSeparatorCount = 2;
    constexpr int kResourceGridHeight = 10;
    constexpr int kProcessWindowChromeRows = 4;
    constexpr int kReservedRows =
        kBottomBarHeight + kHeaderHeight + kSeparatorCount + kResourceGridHeight + kProcessWindowChromeRows;

    return static_cast<std::size_t>(std::max(1, terminal_height - kReservedRows));
}

bool handle_shared_input_event(
    ui::AppController& controller,
    std::string& shared_input_buffer,
    int& shared_input_cursor,
    const ftxui::Event& event,
    std::mutex& state_mutex,
    const ftxui::Component& input_component) {
    if (event == ftxui::Event::Escape) {
        std::scoped_lock lock(state_mutex);
        controller.handle_key(27);
        shared_input_buffer.clear();
        shared_input_cursor = 0;
        return true;
    }
    return input_component->OnEvent(event);
}

namespace {

std::string read_file(std::string_view path) {
    std::ifstream input(std::string{path});
    if (!input) {
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

double root_used_percent() {
    struct statvfs stats {};
    if (::statvfs("/", &stats) != 0 || stats.f_blocks == 0) {
        return 0.0;
    }

    const auto total_blocks = static_cast<long double>(stats.f_blocks);
    const auto available_blocks = static_cast<long double>(stats.f_bavail);
    const auto used_blocks = (total_blocks > available_blocks) ? (total_blocks - available_blocks) : 0.0L;
    return static_cast<double>((used_blocks / total_blocks) * 100.0L);
}

bool is_numeric_directory_name(const std::string& name) {
    return !name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

std::vector<model::ProcessInfo> collect_processes(std::uint64_t total_memory_bytes, long clock_ticks_per_second) {
    std::vector<model::ProcessInfo> processes;

    std::error_code proc_error;
    auto iterator = std::filesystem::directory_iterator(
        "/proc", std::filesystem::directory_options::skip_permission_denied, proc_error);

    for (; !proc_error && iterator != std::filesystem::end(iterator); iterator.increment(proc_error)) {
        const auto& entry = *iterator;
        std::error_code entry_error;
        if (!entry.is_directory(entry_error) || entry_error) {
            continue;
        }

        const auto file_name = entry.path().filename().string();
        if (!is_numeric_directory_name(file_name)) {
            continue;
        }

        const auto stat_text = read_file((entry.path() / "stat").string());
        const auto status_text = read_file((entry.path() / "status").string());
        if (stat_text.empty() || status_text.empty()) {
            continue;
        }

        const auto pid = std::stoi(file_name);
        processes.push_back(
            collector::parse_process_info(pid, stat_text, status_text, total_memory_bytes, clock_ticks_per_second));
    }

    return collector::sort_processes(std::move(processes), collector::ProcessSortKey::Memory);
}

class ProcfsSampler final : public Sampler {
  public:
    model::SystemSnapshot collect() override {
        model::SystemSnapshot snapshot;
        snapshot.captured_at = std::chrono::system_clock::now();

        const auto cpu_text = read_file("/proc/stat");
        const auto current_cpu = collector::parse_cpu_sample(cpu_text);
        snapshot.cpu = previous_cpu_ ? collector::compute_cpu_metrics(*previous_cpu_, current_cpu)
                                     : collector::compute_cpu_metrics(current_cpu, current_cpu);
        previous_cpu_ = current_cpu;

        snapshot.memory = collector::parse_memory_info(read_file("/proc/meminfo"));

        const auto current_disks = collector::parse_disk_stats(read_file("/proc/diskstats"));
        auto disks = previous_disks_ ? collector::compute_disk_metrics(*previous_disks_, current_disks, "/")
                                     : collector::compute_disk_metrics(current_disks, current_disks, "/");
        previous_disks_ = current_disks;
        const auto used_percent = root_used_percent();
        if (disks.empty()) {
            disks.push_back({"/", used_percent, 0, 0});
        }
        for (auto& disk : disks) {
            disk.used_percent = used_percent;
        }
        snapshot.disks = std::move(disks);

        const auto current_networks = collector::parse_network_stats(read_file("/proc/net/dev"));
        snapshot.interfaces = previous_networks_
                                  ? collector::compute_network_metrics(*previous_networks_, current_networks)
                                  : collector::compute_network_metrics(current_networks, current_networks);
        previous_networks_ = current_networks;

        snapshot.processes = collect_processes(snapshot.memory.total_bytes, ::sysconf(_SC_CLK_TCK));
        return snapshot;
    }

  private:
    std::optional<collector::CpuSample> previous_cpu_;
    std::optional<std::vector<collector::DiskCounters>> previous_disks_;
    std::optional<std::vector<collector::NetworkCounters>> previous_networks_;
};

std::string command_input(const ui::AppController& controller) {
    return controller.shared_input_text();
}

std::vector<model::ProcessInfo> visible_processes_for_controller(
    const model::SystemSnapshot& snapshot,
    const ui::AppController& controller) {
    return collector::filter_processes(collector::sort_processes(snapshot.processes, controller.sort_key()),
                                       controller.filter_query());
}

void refresh_snapshot(
    SamplingWorker& worker,
    store::SnapshotStore& store,
    ui::AppController& controller,
    model::SystemSnapshot& latest_snapshot,
    std::optional<std::chrono::steady_clock::time_point>& transient_status_deadline) {
    if (transient_status_deadline && std::chrono::steady_clock::now() >= *transient_status_deadline) {
        controller.set_status_text("ready");
        transient_status_deadline.reset();
    }
    worker.tick_once();
    latest_snapshot = store.latest();
    controller.set_visible_process_count(visible_processes_for_controller(latest_snapshot, controller).size());
}

bool handle_process_action(
    ui::AppController& controller,
    const std::vector<model::ProcessInfo>& visible_processes,
    char key) {
    if (visible_processes.empty()) {
        return false;
    }

    const auto index = std::min(controller.selected_process_index(), visible_processes.size() - 1);
    const auto pid = visible_processes[index].pid;
    if (key == 'K') {
        controller.begin_kill(pid);
        return true;
    }
    if (key == 'R') {
        controller.begin_renice(pid);
        return true;
    }
    return false;
}

ftxui::Element input_bar_document(
    const ui::AppController& controller,
    const std::string& status_text,
    ftxui::Component input_component) {
    const auto& theme = ui::catppuccin_mocha();
    if (!controller.shared_input_active()) {
        return ui::render_dashboard_bottom_bar_document(controller);
    }
    const auto prefix = controller.command_input_active() ? ":" : "/";
    return ftxui::hbox({
        ui::themed_window(
            "Input",
            ftxui::hbox({
                ftxui::text(prefix) | ftxui::color(theme.sapphire) | ftxui::bold,
                input_component->Render() | ftxui::color(theme.text),
            }),
            controller.focus() == app::FocusZone::CommandBar)
            | ftxui::flex,
        ui::themed_window("Status", ftxui::text(status_text) | ftxui::color(ui::status_color(status_text)), false)
            | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 28),
    }) | ftxui::bgcolor(theme.base);
}

}  // namespace
#endif

Application::Application(AppConfig config)
    : config_(config), store_(config.history_size), controller_(config) {}

int Application::run() {
#if !MONITOR_HAS_FTXUI
    std::cout << "replace bootstrap with FTXUI event loop\n";
    return 0;
#else
    ProcfsSampler sampler;
    SamplingWorker worker(sampler, store_);
    actions::PosixProcessMutator mutator;
    actions::ProcessActions process_actions(mutator);
    model::SystemSnapshot latest_snapshot;
    std::string renice_input;
    std::string shared_input_buffer;
    int shared_input_cursor = 0;
    std::optional<std::chrono::steady_clock::time_point> transient_status_deadline;
    std::mutex state_mutex;

    const auto render_once = [&]() {
        controller_.set_process_window_height(visible_process_rows_for_terminal_height(40));
        refresh_snapshot(worker, store_, controller_, latest_snapshot, transient_status_deadline);
        history_.cpu_history.push(latest_snapshot.cpu.total_percent);
        double mem_pct = (latest_snapshot.memory.total_bytes > 0)
                             ? (static_cast<double>(latest_snapshot.memory.used_bytes) / latest_snapshot.memory.total_bytes * 100.0)
                             : 0.0;
        history_.memory_history.push(mem_pct);
        if (!latest_snapshot.disks.empty()) {
            history_.disk_read_history.push(static_cast<double>(latest_snapshot.disks.front().read_bytes_per_sec));
            history_.disk_write_history.push(static_cast<double>(latest_snapshot.disks.front().write_bytes_per_sec));
        }
        if (!latest_snapshot.interfaces.empty()) {
            history_.network_rx_history.push(static_cast<double>(latest_snapshot.interfaces.front().rx_bytes_per_sec));
            history_.network_tx_history.push(static_cast<double>(latest_snapshot.interfaces.front().tx_bytes_per_sec));
        }
        render_cache_.update_snapshot(latest_snapshot);
        render_cache_.update_history(history_);
        if (render_cache_.is_dirty()) {
            auto rendered = ui::render_dashboard_to_string(latest_snapshot, controller_, history_, 120, 40);
            render_cache_.set_cached_render_string(std::move(rendered));
        }
        return render_cache_.cached_render_string();
    };

    if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO)) {
        std::cout << render_once();
        return 0;
    }

    refresh_snapshot(worker, store_, controller_, latest_snapshot, transient_status_deadline);

    auto screen = ftxui::ScreenInteractive::Fullscreen();
    screen.ForceHandleCtrlC(true);

    ftxui::InputOption input_option;
    input_option.multiline = false;
    input_option.content = &shared_input_buffer;
    input_option.cursor_position = &shared_input_cursor;
    input_option.on_change = [&] {
        std::scoped_lock lock(state_mutex);
        controller_.handle_text(shared_input_buffer);
    };
    input_option.on_enter = [&] {
        std::scoped_lock lock(state_mutex);
        if (controller_.command_input_active()) {
            controller_.execute_command(
                command_input(controller_),
                [&](std::string_view command, int pid, std::optional<int> value) {
                    if (command == "kill") {
                        return process_actions.kill_process(pid).message;
                    }
                    if (command == "renice" && value.has_value()) {
                        return process_actions.renice_process(pid, *value).message;
                    }
                    return std::string{"unknown command"};
                });
            shared_input_buffer.clear();
            shared_input_cursor = 0;
            if (controller_.should_quit()) {
                screen.ExitLoopClosure()();
            }
            return;
        }
        if (controller_.filter_input_active()) {
            controller_.submit_filter();
            shared_input_buffer.clear();
            shared_input_cursor = 0;
        }
    };
    auto input_component = ftxui::Input(input_option);

    auto dashboard = ftxui::Renderer([&] {
        std::scoped_lock lock(state_mutex);
        const auto terminal_size = ftxui::Terminal::Size();
        controller_.set_process_window_height(visible_process_rows_for_terminal_height(terminal_size.dimy));
        return ftxui::vbox({
            ui::render_dashboard_body_document(latest_snapshot, controller_, history_)
                | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, terminal_size.dimx)
                | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, terminal_size.dimy - 3),
            input_bar_document(controller_, controller_.status_text(), input_component),
        });
    });

    dashboard |= ftxui::CatchEvent([&](ftxui::Event event) {
        bool shared_input_active = false;
        {
            std::scoped_lock lock(state_mutex);

            if (event == ftxui::Event::Custom) {
                return true;
            }

            shared_input_active = controller_.shared_input_active();

            if (!shared_input_active && controller_.mode() == ui::InputMode::ConfirmKill) {
                if (event == ftxui::Event::Escape || event == ftxui::Event::Character('n')
                    || event == ftxui::Event::Character('N')) {
                    controller_.handle_key(27);
                    return true;
                }
                if (event == ftxui::Event::Character('y') || event == ftxui::Event::Character('Y')
                    || event == ftxui::Event::Return) {
                    const auto pid = controller_.selected_pid();
                    const auto result = process_actions.kill_process(pid);
                    controller_.confirm_kill();
                    if (result.ok) {
                        controller_.set_status_text("killed " + std::to_string(pid) + " successfully");
                        transient_status_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                    } else {
                        controller_.set_status_text(result.message);
                        transient_status_deadline.reset();
                    }
                    return true;
                }
                return true;
            }

            if (!shared_input_active && controller_.mode() == ui::InputMode::Renice) {
                if (event == ftxui::Event::Escape) {
                    renice_input.clear();
                    controller_.handle_key(27);
                    return true;
                }
                if (event == ftxui::Event::Backspace) {
                    if (!renice_input.empty()) {
                        renice_input.pop_back();
                    }
                    controller_.set_status_text(
                        renice_input.empty() ? std::string{ui::kRenicePrompt}
                                             : "Enter new nice value: " + renice_input);
                    return true;
                }
                if (event == ftxui::Event::Return) {
                    if (renice_input.empty()) {
                        controller_.submit_renice(0);
                        controller_.set_status_text("invalid nice value");
                        transient_status_deadline.reset();
                    } else {
                        try {
                            const auto pid = controller_.selected_pid();
                            const auto nice_value = std::stoi(renice_input);
                            const auto result = process_actions.renice_process(pid, nice_value);
                            controller_.submit_renice(nice_value);
                            if (result.ok) {
                                controller_.set_status_text(
                                    "reniced " + std::to_string(pid) + " to " + std::to_string(nice_value));
                                transient_status_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                            } else {
                                controller_.set_status_text(result.message);
                                transient_status_deadline.reset();
                            }
                        } catch (const std::exception&) {
                            controller_.submit_renice(0);
                            controller_.set_status_text("invalid nice value");
                            transient_status_deadline.reset();
                        }
                    }
                    renice_input.clear();
                    return true;
                }
                if (event.is_character()) {
                    const auto text = event.character();
                    if (text.size() == 1) {
                        const auto ch = text.front();
                        if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || (ch == '-' && renice_input.empty())) {
                            renice_input.push_back(ch);
                            controller_.set_status_text("Enter new nice value: " + renice_input);
                        }
                    }
                    return true;
                }
                return true;
            }

            if (!shared_input_active && event == ftxui::Event::Character('q')) {
                screen.ExitLoopClosure()();
                return true;
            }

            if (!shared_input_active && (event == ftxui::Event::Character(':') || event == ftxui::Event::Character('/'))) {
                controller_.handle_key(event.character().front());
                shared_input_buffer = controller_.shared_input_text();
                shared_input_cursor = static_cast<int>(shared_input_buffer.size());
                return true;
            }

            if (!shared_input_active && (event == ftxui::Event::Character('h') || event == ftxui::Event::Character('l')
                || event == ftxui::Event::Character('s') || event == ftxui::Event::Character('j')
                || event == ftxui::Event::Character('k'))) {
                controller_.handle_key(event.character().front());
                return true;
            }

            if (!shared_input_active && (event == ftxui::Event::Character('K') || event == ftxui::Event::Character('R'))) {
                return handle_process_action(
                    controller_, visible_processes_for_controller(latest_snapshot, controller_),
                    event.character().front());
            }
        }

        if (shared_input_active) {
            return handle_shared_input_event(
                controller_, shared_input_buffer, shared_input_cursor, event, state_mutex, input_component);
        }

        return false;
    });

    std::atomic<bool> running{true};
    std::thread sampler_thread([&] {
        while (running.load()) {
            {
                std::scoped_lock lock(state_mutex);
                refresh_snapshot(worker, store_, controller_, latest_snapshot, transient_status_deadline);
                history_.cpu_history.push(latest_snapshot.cpu.total_percent);
                double mem_pct = (latest_snapshot.memory.total_bytes > 0)
                                     ? (static_cast<double>(latest_snapshot.memory.used_bytes) / latest_snapshot.memory.total_bytes * 100.0)
                                     : 0.0;
                history_.memory_history.push(mem_pct);
                if (!latest_snapshot.disks.empty()) {
                    history_.disk_read_history.push(static_cast<double>(latest_snapshot.disks.front().read_bytes_per_sec));
                    history_.disk_write_history.push(static_cast<double>(latest_snapshot.disks.front().write_bytes_per_sec));
                }
                if (!latest_snapshot.interfaces.empty()) {
                    history_.network_rx_history.push(static_cast<double>(latest_snapshot.interfaces.front().rx_bytes_per_sec));
                    history_.network_tx_history.push(static_cast<double>(latest_snapshot.interfaces.front().tx_bytes_per_sec));
                }
                render_cache_.update_snapshot(latest_snapshot);
                render_cache_.update_history(history_);
            }
            screen.PostEvent(ftxui::Event::Custom);
            std::this_thread::sleep_for(config_.refresh_interval);
        }
    });

    screen.Loop(dashboard);
    running = false;
    if (sampler_thread.joinable()) {
        sampler_thread.join();
    }
    return 0;
#endif
}

}  // namespace monitor::app
