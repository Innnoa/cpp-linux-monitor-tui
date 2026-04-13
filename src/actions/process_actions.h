#pragma once

#include <string>
#include <system_error>

namespace monitor::actions {

struct ActionResult {
    bool ok{false};
    std::string message;
};

class ProcessMutator {
  public:
    virtual ~ProcessMutator() = default;
    virtual std::error_code send_signal(int pid, int signal_number) = 0;
    virtual std::error_code set_priority(int pid, int nice_value) = 0;
};

class PosixProcessMutator final : public ProcessMutator {
  public:
    std::error_code send_signal(int pid, int signal_number) override;
    std::error_code set_priority(int pid, int nice_value) override;
};

class ProcessActions {
  public:
    explicit ProcessActions(ProcessMutator& mutator) : mutator_(mutator) {}

    ActionResult kill_process(int pid);
    ActionResult renice_process(int pid, int nice_value);

  private:
    ProcessMutator& mutator_;
};

}  // namespace monitor::actions
