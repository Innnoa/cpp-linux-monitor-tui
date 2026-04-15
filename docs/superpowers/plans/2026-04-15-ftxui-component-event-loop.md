# FTXUI Component Event Loop Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the current hand-written raw input loop with a component-driven `FTXUI` event loop while keeping the current process list, shared command/filter input, and right-side detail pane behavior intact.

**Architecture:** Keep the existing controller, sampler, and renderer layers. Introduce a top-level `FTXUI` component tree that makes the process list the default focus, uses one transient input component for both `:` and `/`, and drives redraw/sampling through the `FTXUI` runtime instead of the current custom blocking loop. Preserve the non-TTY single-frame render path.

**Tech Stack:** `C++20`, `FTXUI`, existing `AppController`, existing `SnapshotStore`, existing procfs sampler, `Catch2`, `ctest`

---

## File Map

- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/app/application.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.h`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/app_controller_test.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/app/application_test.cpp`

## Task 1: Add controller state for shared input focus

**Files:**
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.h`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/app_controller_test.cpp`

- [ ] **Step 1: Write the failing controller tests**

Add these tests to `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/app_controller_test.cpp`:

```cpp
TEST_CASE("controller enters command and filter modes as shared input focus states") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.handle_key(':');
    CHECK(controller.mode() == monitor::ui::InputMode::Command);
    CHECK(controller.command_text() == ":");

    controller.handle_key(27);
    CHECK(controller.mode() == monitor::ui::InputMode::Normal);

    controller.handle_key('/');
    CHECK(controller.mode() == monitor::ui::InputMode::Filter);
    CHECK(controller.status_text() == "Filter mode");
}
```

- [ ] **Step 2: Run the targeted controller tests and confirm they fail only if the shared input state is not stable**

Run:

```bash
cmake --build /home/zazaki/Projects/cpp-linux-monitor-tui/build
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "controller"
```

Expected: new assertions fail if mode transitions are not already represented correctly.

- [ ] **Step 3: Implement minimal controller support**

Keep this task small. Only add the minimum explicit API needed by the component runtime to ask:

- whether the shared input is open
- whether it is in command or filter mode

If the current `InputMode` enum and existing getters already cover this cleanly, do not add extra state.

- [ ] **Step 4: Re-run controller tests**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "controller"
```

Expected: controller tests pass.

- [ ] **Step 5: Commit controller input-state support**

```bash
git -C /home/zazaki/Projects/cpp-linux-monitor-tui add src/ui/app_controller.h src/ui/app_controller.cpp tests/ui/app_controller_test.cpp
git -C /home/zazaki/Projects/cpp-linux-monitor-tui commit -m "feat: expose shared input state"
```

## Task 2: Replace the raw loop with an FTXUI event loop

**Files:**
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/app/application.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/app/application_test.cpp`

- [ ] **Step 1: Write the failing application test**

Add this test to `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/app/application_test.cpp`:

```cpp
TEST_CASE("application can render a snapshot without using the raw loop path") {
    FakeSampler sampler;
    monitor::store::SnapshotStore store(4);
    monitor::app::SamplingWorker worker(sampler, store);

    worker.tick_once();
    const auto latest = store.latest();

    CHECK(latest.cpu.total_percent == 37.0);
    CHECK(latest.memory.used_bytes == 31ULL * 1024ULL * 1024ULL);
}
```

This test is intentionally narrow: it keeps the application-side verification anchored while the runtime loop changes.

- [ ] **Step 2: Run application tests before changing runtime code**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "sampling worker|application"
```

Expected: current tests are green; use that as the pre-refactor baseline.

- [ ] **Step 3: Replace the TTY raw loop with an FTXUI-driven loop**

In `/home/zazaki/Projects/cpp-linux-monitor-tui/src/app/application.cpp`:

- remove the custom `select + read` polling loop
- create an `FTXUI` `ScreenInteractive`
- build a lightweight component that:
  - keeps process list as the default focus
  - routes `:` and `/` into the shared transient input behavior
  - triggers redraw after sampling ticks
- preserve the non-TTY path exactly:
  - sample once
  - render one frame
  - exit

Do **not** rewrite the renderer itself in this task. The goal is to swap the event loop, not to redesign the screen.

- [ ] **Step 4: Re-run build and focused tests**

Run:

```bash
cmake --build /home/zazaki/Projects/cpp-linux-monitor-tui/build
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "application|controller|dashboard"
```

Expected: application/controller/dashboard tests remain green.

- [ ] **Step 5: Manual smoke-check interactive behavior**

Run:

```bash
/home/zazaki/Projects/cpp-linux-monitor-tui/build/monitor_tui
```

Verify:

- process list starts focused
- `:` opens the command input
- `/` opens the filter input
- `Esc` returns focus to the process list
- `q` still quits

- [ ] **Step 6: Commit the component event loop**

```bash
git -C /home/zazaki/Projects/cpp-linux-monitor-tui add src/app/application.cpp tests/app/application_test.cpp
git -C /home/zazaki/Projects/cpp-linux-monitor-tui commit -m "feat: switch runtime to ftxui event loop"
```

## Task 3: Reconfirm full runtime compatibility

**Files:**
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/app/application.cpp` (only if needed)

- [ ] **Step 1: Re-run the full local test suite**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 2: Re-run the non-TTY smoke check**

Run:

```bash
/home/zazaki/Projects/cpp-linux-monitor-tui/build/monitor_tui
```

Expected:

- in non-interactive execution, it still prints a single snapshot and exits

- [ ] **Step 3: Commit any final compatibility fixes**

```bash
git -C /home/zazaki/Projects/cpp-linux-monitor-tui add src/app/application.cpp
git -C /home/zazaki/Projects/cpp-linux-monitor-tui commit -m "fix: preserve non-tty runtime compatibility"
```

## Self-Review

### Spec coverage

- shared transient input component: Task 1 + Task 2
- process-list-first focus: Task 2 manual verification
- FTXUI-driven runtime loop: Task 2
- non-TTY single-frame behavior preserved: Task 3

### Placeholder scan

- This plan contains no `TODO`, `TBD`, or “implement later” placeholders.

### Type consistency

- The plan intentionally reuses the existing `InputMode`, `command_text`, `filter_query`, `status_text`, and render paths instead of introducing new naming.
