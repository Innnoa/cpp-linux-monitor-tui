#include <system_error>

#include <catch2/catch_test_macros.hpp>

#include "actions/process_actions.h"

namespace {
class FakeMutator final : public monitor::actions::ProcessMutator {
  public:
    std::error_code signal_error{};
    std::error_code renice_error{};
    int signal_calls{0};
    int renice_calls{0};

    std::error_code send_signal(int, int) override {
        ++signal_calls;
        return signal_error;
    }
    std::error_code set_priority(int, int) override {
        ++renice_calls;
        return renice_error;
    }
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

TEST_CASE("kill action rejects invalid pid") {
    FakeMutator mutator;
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.kill_process(0);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "invalid pid");
    CHECK(mutator.signal_calls == 0);
}

TEST_CASE("kill action maps no such process") {
    FakeMutator mutator;
    mutator.signal_error = std::make_error_code(std::errc::no_such_process);
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.kill_process(812);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "process no longer exists");
}

TEST_CASE("kill action returns ok on success") {
    FakeMutator mutator;
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.kill_process(812);

    CHECK(result.ok);
    CHECK(result.message == "ok");
    CHECK(mutator.signal_calls == 1);
}

TEST_CASE("renice action rejects values outside the Linux nice range") {
    FakeMutator mutator;
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.renice_process(812, 25);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "nice value must be between -20 and 19");
}

TEST_CASE("renice action rejects invalid pid") {
    FakeMutator mutator;
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.renice_process(-1, 0);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "invalid pid");
    CHECK(mutator.renice_calls == 0);
}

TEST_CASE("renice action maps no such process") {
    FakeMutator mutator;
    mutator.renice_error = std::make_error_code(std::errc::no_such_process);
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.renice_process(812, 0);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "process no longer exists");
}

TEST_CASE("renice action explains privilege requirements on permission errors") {
    FakeMutator mutator;
    mutator.renice_error = std::make_error_code(std::errc::permission_denied);
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.renice_process(812, -5);

    CHECK_FALSE(result.ok);
    CHECK(result.message == "permission denied: lowering nice usually requires root or CAP_SYS_NICE");
}

TEST_CASE("renice action returns ok on success") {
    FakeMutator mutator;
    monitor::actions::ProcessActions actions(mutator);

    const auto result = actions.renice_process(812, 0);

    CHECK(result.ok);
    CHECK(result.message == "ok");
    CHECK(mutator.renice_calls == 1);
}
