#include <system_error>

#include <catch2/catch_test_macros.hpp>

#include "actions/process_actions.h"

namespace {
class FakeMutator final : public monitor::actions::ProcessMutator {
  public:
    std::error_code signal_error{};
    std::error_code renice_error{};

    std::error_code send_signal(int, int) override { return signal_error; }
    std::error_code set_priority(int, int) override { return renice_error; }
};
}  // namespace

TEST_CASE("kill action returns readable permission errors") {
    FakeMutator mutator;
    mutator.signal_error = std::make_error_code(std::errc::operation_not_permitted);
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.kill_process(812);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "permission denied");
}

TEST_CASE("renice action rejects values outside the Linux nice range") {
    FakeMutator mutator;
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.renice_process(812, 25);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "nice value must be between -20 and 19");
}
