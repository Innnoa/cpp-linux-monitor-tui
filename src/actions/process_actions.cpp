#include "actions/process_actions.h"

#include <cerrno>
#include <csignal>
#include <sys/resource.h>

namespace monitor::actions {

namespace {
std::string friendly_message(const std::error_code& error) {
    if (!error) {
        return "ok";
    }
    if (error == std::errc::operation_not_permitted) {
        return "permission denied";
    }
    if (error == std::errc::no_such_process) {
        return "process no longer exists";
    }
    return error.message();
}
}  // namespace

std::error_code PosixProcessMutator::send_signal(int pid, int signal_number) {
    if (::kill(pid, signal_number) == 0) {
        return {};
    }
    return {errno, std::generic_category()};
}

std::error_code PosixProcessMutator::set_priority(int pid, int nice_value) {
    if (::setpriority(PRIO_PROCESS, pid, nice_value) == 0) {
        return {};
    }
    return {errno, std::generic_category()};
}

ActionResult ProcessActions::kill_process(int pid) {
    if (pid <= 0) {
        return {.ok = false, .message = "invalid pid"};
    }
    const auto error = mutator_.send_signal(pid, SIGTERM);
    return {.ok = !error, .message = friendly_message(error)};
}

ActionResult ProcessActions::renice_process(int pid, int nice_value) {
    if (pid <= 0) {
        return {.ok = false, .message = "invalid pid"};
    }
    if (nice_value < -20 || nice_value > 19) {
        return {.ok = false, .message = "nice value must be between -20 and 19"};
    }
    const auto error = mutator_.set_priority(pid, nice_value);
    return {.ok = !error, .message = friendly_message(error)};
}

}  // namespace monitor::actions
