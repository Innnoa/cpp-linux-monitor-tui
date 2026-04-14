#include "app/application.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <sys/statvfs.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "app/sampling_worker.h"
#include "actions/process_actions.h"
#include "collector/cpu_collector.h"
#include "collector/disk_collector.h"
#include "collector/memory_collector.h"
#include "collector/network_collector.h"
#include "collector/process_collector.h"
#if MONITOR_HAS_FTXUI
#include "ui/dashboard_view.h"
#endif

namespace monitor::app {

#if MONITOR_HAS_FTXUI
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

class ScopedRawMode {
  public:
    ScopedRawMode() {
        if (!::isatty(STDIN_FILENO)) {
            return;
        }
        if (::tcgetattr(STDIN_FILENO, &original_) != 0) {
            return;
        }

        auto raw = original_;
        raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
            enabled_ = true;
        }
    }

    ~ScopedRawMode() {
        if (enabled_) {
            ::tcsetattr(STDIN_FILENO, TCSANOW, &original_);
        }
    }

    [[nodiscard]] bool enabled() const { return enabled_; }

  private:
    bool enabled_{false};
    ::termios original_{};
};

std::optional<char> read_char_with_timeout(std::chrono::milliseconds timeout) {
    ::fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    ::timeval tv{};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    const auto ready = ::select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &tv);
    if (ready <= 0 || !FD_ISSET(STDIN_FILENO, &read_fds)) {
        return std::nullopt;
    }

    char ch = '\0';
    const auto count = ::read(STDIN_FILENO, &ch, 1);
    if (count != 1) {
        return std::nullopt;
    }
    return ch;
}

std::string command_input(const ui::AppController& controller) {
    auto command = controller.command_text();
    if (!command.empty() && command.front() == ':') {
        command.erase(0, 1);
    }
    return command;
}

std::vector<model::ProcessInfo> visible_processes_for_controller(
    const model::SystemSnapshot& snapshot,
    const ui::AppController& controller) {
    return collector::filter_processes(collector::sort_processes(snapshot.processes, controller.sort_key()),
                                       controller.filter_query());
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

    const auto render_once = [&]() {
        worker.tick_once();
        latest_snapshot = store_.latest();
        controller_.set_process_window_height(5);
        controller_.set_visible_process_count(visible_processes_for_controller(latest_snapshot, controller_).size());
        return ui::render_dashboard_to_string(latest_snapshot, controller_, 120, 40);
    };

    if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO)) {
        std::cout << render_once();
        return 0;
    }

    ScopedRawMode raw_mode;
    std::cout << "\x1b[?25l";

    bool running = true;
    while (running) {
        const auto frame = render_once();
        std::cout << "\x1b[2J\x1b[H" << frame << "\n[q quit]" << std::flush;

        const auto key = read_char_with_timeout(config_.refresh_interval);
        if (!key) {
            continue;
        }

        if (controller_.mode() == ui::InputMode::Filter) {
            if (*key == 27) {
                controller_.handle_key(27);
            } else if (*key == 127 || *key == '\b') {
                auto text = controller_.filter_query();
                if (!text.empty()) {
                    text.pop_back();
                }
                controller_.handle_text(text);
            } else if (*key != '\n' && *key != '\r') {
                auto text = controller_.filter_query();
                text.push_back(*key);
                controller_.handle_text(text);
            }
            continue;
        }

        if (controller_.mode() == ui::InputMode::Command) {
            if (*key == 27) {
                controller_.handle_key(27);
            } else if (*key == 127 || *key == '\b') {
                auto text = command_input(controller_);
                if (!text.empty()) {
                    text.pop_back();
                }
                controller_.handle_text(text);
            } else if (*key == '\n' || *key == '\r') {
                controller_.execute_command(command_input(controller_));
                if (controller_.should_quit()) {
                    running = false;
                }
            } else {
                auto text = command_input(controller_);
                text.push_back(*key);
                controller_.handle_text(text);
            }
            continue;
        }

        if (controller_.mode() == ui::InputMode::ConfirmKill || controller_.mode() == ui::InputMode::Renice) {
            if (controller_.mode() == ui::InputMode::ConfirmKill) {
                if (*key == 27 || *key == 'n' || *key == 'N') {
                    controller_.handle_key(27);
                } else if (*key == 'y' || *key == 'Y' || *key == '\n' || *key == '\r') {
                    const auto result = process_actions.kill_process(controller_.selected_pid());
                    controller_.confirm_kill();
                    controller_.set_status_text(result.message);
                }
                continue;
            }

            if (*key == 27) {
                renice_input.clear();
                controller_.handle_key(27);
            } else if (*key == 127 || *key == '\b') {
                if (!renice_input.empty()) {
                    renice_input.pop_back();
                }
                controller_.set_status_text(
                    renice_input.empty() ? "Enter new nice value" : "Enter new nice value: " + renice_input);
            } else if (*key == '\n' || *key == '\r') {
                if (renice_input.empty()) {
                    controller_.submit_renice(0);
                    controller_.set_status_text("invalid nice value");
                } else {
                    try {
                        const auto nice_value = std::stoi(renice_input);
                        const auto result = process_actions.renice_process(controller_.selected_pid(), nice_value);
                        controller_.submit_renice(nice_value);
                        controller_.set_status_text(result.message);
                    } catch (const std::exception&) {
                        controller_.submit_renice(0);
                        controller_.set_status_text("invalid nice value");
                    }
                }
                renice_input.clear();
            } else if (std::isdigit(static_cast<unsigned char>(*key)) != 0
                       || (*key == '-' && renice_input.empty())) {
                renice_input.push_back(*key);
                controller_.set_status_text("Enter new nice value: " + renice_input);
            } else {
                controller_.set_status_text("Enter new nice value" + (renice_input.empty() ? "" : ": " + renice_input));
            }
            continue;
        }

        switch (*key) {
            case 'q':
                running = false;
                break;
            case '/':
            case ':':
            case 'h':
            case 'l':
            case 's':
                controller_.handle_key(*key);
                break;
            case 'K':
                {
                    const auto visible_processes = visible_processes_for_controller(latest_snapshot, controller_);
                    if (!visible_processes.empty()) {
                        const auto index =
                            std::min(controller_.selected_process_index(), visible_processes.size() - 1);
                        controller_.begin_kill(visible_processes[index].pid);
                    }
                }
                break;
            case 'R':
                {
                    const auto visible_processes = visible_processes_for_controller(latest_snapshot, controller_);
                    if (!visible_processes.empty()) {
                        const auto index =
                            std::min(controller_.selected_process_index(), visible_processes.size() - 1);
                        controller_.begin_renice(visible_processes[index].pid);
                        renice_input.clear();
                    }
                }
                break;
            case 'j':
            case 'k':
                controller_.handle_key(*key);
                break;
            default:
                break;
        }
    }

    std::cout << "\x1b[2J\x1b[H\x1b[?25h";
    return 0;
#endif
}

}  // namespace monitor::app
