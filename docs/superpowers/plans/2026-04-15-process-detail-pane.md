# Process Detail Pane Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a right-side process detail pane that follows the selected process and tighten runtime action feedback so `kill` and `renice` results reconcile cleanly after refresh.

**Architecture:** Keep the current controller + renderer structure. Extend `AppController` with the minimum detail/selection reconciliation state, derive the right-side detail pane directly from the selected visible process, and let the runtime loop refresh selection after actions based on the next sampled visible list. Avoid introducing a separate detail view-model layer in this iteration.

**Tech Stack:** `C++20`, `FTXUI`, `/proc`, existing `SnapshotStore`, existing `ProcessActions`, `Catch2`, `ctest`

---

## File Map

- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.h`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/dashboard_view.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/app/application.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/app_controller_test.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/dashboard_view_test.cpp`

## Task 1: Add controller state for detail-pane reconciliation

**Files:**
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.h`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/app_controller_test.cpp`

- [ ] **Step 1: Write the failing controller tests**

Add these tests to `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/app_controller_test.cpp`:

```cpp
TEST_CASE("controller clamps selected row when visible process list shrinks") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.set_visible_process_count(4);
    controller.set_process_window_height(3);
    controller.handle_key('j');
    controller.handle_key('j');
    controller.handle_key('j');

    CHECK(controller.selected_process_index() == 3);

    controller.set_visible_process_count(2);

    CHECK(controller.selected_process_index() == 0);
    CHECK(controller.process_window_start() == 0);
}

TEST_CASE("controller exposes selected process detail fields after action reset") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.begin_kill(812);
    controller.confirm_kill();

    CHECK(controller.mode() == monitor::ui::InputMode::Normal);
    CHECK(controller.selected_pid() == 0);
    CHECK(controller.status_text() == "ready");
}
```

- [ ] **Step 2: Run the targeted controller tests and confirm the new assertions fail**

Run:

```bash
cmake --build /home/zazaki/Projects/cpp-linux-monitor-tui/build
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "controller"
```

Expected: at least one of the new controller assertions fails because shrink/reconciliation behavior is not fully implemented yet.

- [ ] **Step 3: Implement the minimal controller reconciliation behavior**

Update `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/app_controller.cpp` so `set_visible_process_count()` keeps selection stable when still valid and only clamps to the nearest valid row when the visible list shrinks below the selected index.

Required code shape:

```cpp
void AppController::set_visible_process_count(std::size_t count) {
    visible_process_count_ = count;
    if (visible_process_count_ == 0) {
        selected_process_index_ = 0;
        process_window_start_ = 0;
        return;
    }
    if (selected_process_index_ >= visible_process_count_) {
        selected_process_index_ = visible_process_count_ - 1;
    }
    const auto max_window_start =
        (visible_process_count_ > process_window_height_) ? (visible_process_count_ - process_window_height_) : 0;
    if (process_window_start_ > max_window_start) {
        process_window_start_ = max_window_start;
    }
}
```

- [ ] **Step 4: Re-run the controller tests**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "controller"
```

Expected: controller tests pass again.

- [ ] **Step 5: Commit controller reconciliation**

```bash
git -C /home/zazaki/Projects/cpp-linux-monitor-tui add src/ui/app_controller.h src/ui/app_controller.cpp tests/ui/app_controller_test.cpp
git -C /home/zazaki/Projects/cpp-linux-monitor-tui commit -m "feat: add process detail pane controller state"
```

## Task 2: Render the right-side detail pane

**Files:**
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/dashboard_view.cpp`
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/dashboard_view_test.cpp`

- [ ] **Step 1: Write the failing detail-pane render test**

Add this test to `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/dashboard_view_test.cpp`:

```cpp
TEST_CASE("dashboard shows selected process details in the right pane") {
    monitor::model::SystemSnapshot snapshot;
    snapshot.processes.push_back({812, 'R', 98.0, 2.1, 5, "root", "postgres"});
    snapshot.processes.push_back({301, 'S', 12.0, 1.0, 0, "user", "nginx"});

    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());
    controller.set_visible_process_count(2);
    controller.handle_key('j');

    const auto output = monitor::ui::render_dashboard_to_string(snapshot, controller, 120, 40);

    CHECK(output.find("Selected Process") != std::string::npos);
    CHECK(output.find("PID: 301") != std::string::npos);
    CHECK(output.find("Name: nginx") != std::string::npos);
    CHECK(output.find("User: user") != std::string::npos);
    CHECK(output.find("State: S") != std::string::npos);
    CHECK(output.find("Memory %: 1.0") != std::string::npos);
    CHECK(output.find("Nice: 0") != std::string::npos);
    CHECK(output.find("K kill") != std::string::npos);
    CHECK(output.find("R renice") != std::string::npos);
}
```

- [ ] **Step 2: Run the targeted dashboard tests and confirm the new test fails**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "dashboard"
```

Expected: the new detail-pane assertions fail because the right pane is not rendered yet.

- [ ] **Step 3: Implement the right-side detail pane**

Update `/home/zazaki/Projects/cpp-linux-monitor-tui/src/ui/dashboard_view.cpp` so the lower section becomes a two-column layout:

```cpp
const auto process_list = ftxui::window(ftxui::text("Processes"), ftxui::vbox(process_rows));

ftxui::Element detail_body;
if (visible_processes.empty()) {
    detail_body = ftxui::vbox({
        ftxui::text("No process selected"),
    });
} else {
    const auto selected_index = std::min(controller.selected_process_index(), visible_processes.size() - 1);
    const auto& selected = visible_processes[selected_index];
    std::ostringstream memory_line;
    memory_line << std::fixed << std::setprecision(1) << selected.memory_percent;
    detail_body = ftxui::vbox({
        ftxui::text("PID: " + std::to_string(selected.pid)),
        ftxui::text("Name: " + selected.name),
        ftxui::text("User: " + selected.user),
        ftxui::text(std::string{"State: "} + selected.state),
        ftxui::text("Memory %: " + memory_line.str()),
        ftxui::text("Nice: " + std::to_string(selected.nice_value)),
        ftxui::separator(),
        ftxui::text("K kill"),
        ftxui::text("R renice"),
    });
}

const auto detail = ftxui::window(ftxui::text("Selected Process"), detail_body);
const auto lower = ftxui::hbox({
    process_list | ftxui::flex,
    detail | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32),
});
```

Then compose `lower` into the main document instead of the old single process window.

- [ ] **Step 4: Re-run dashboard tests**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "dashboard"
```

Expected: dashboard tests pass, including the new detail-pane assertions.

- [ ] **Step 5: Commit the detail-pane renderer**

```bash
git -C /home/zazaki/Projects/cpp-linux-monitor-tui add src/ui/dashboard_view.cpp tests/ui/dashboard_view_test.cpp
git -C /home/zazaki/Projects/cpp-linux-monitor-tui commit -m "feat: add selected process detail pane"
```

## Task 3: Wire runtime feedback after actions

**Files:**
- Modify: `/home/zazaki/Projects/cpp-linux-monitor-tui/src/app/application.cpp`

- [ ] **Step 1: Write the failing runtime selection test shape first**

Add this test to `/home/zazaki/Projects/cpp-linux-monitor-tui/tests/ui/app_controller_test.cpp`:

```cpp
TEST_CASE("controller keeps nearest valid selection after list shrink") {
    monitor::ui::AppController controller(monitor::app::AppConfig::defaults());

    controller.set_visible_process_count(5);
    controller.set_process_window_height(3);
    controller.handle_key('j');
    controller.handle_key('j');
    controller.handle_key('j');
    controller.handle_key('j');

    CHECK(controller.selected_process_index() == 4);

    controller.set_visible_process_count(3);

    CHECK(controller.selected_process_index() == 2);
}
```

- [ ] **Step 2: Run controller tests and confirm the nearest-valid-row case fails if not already covered**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "controller"
```

Expected: this test fails until the clamp logic keeps the nearest valid row instead of resetting to zero.

- [ ] **Step 3: Update runtime loop to refresh detail/selection coherently after actions**

In `/home/zazaki/Projects/cpp-linux-monitor-tui/src/app/application.cpp`, keep the current action execution path but ensure the next `render_once()` call updates `visible_process_count` from the post-action sampled list so the selected row clamps naturally after a killed process disappears.

No large refactor is needed here; the key is to rely on `set_visible_process_count()` after each sample, not manual resets to the top row.

- [ ] **Step 4: Re-run controller and full tests**

Run:

```bash
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure -R "controller|dashboard"
ctest --test-dir /home/zazaki/Projects/cpp-linux-monitor-tui/build --output-on-failure
```

Expected: controller/dashboard tests pass and the full suite remains green.

- [ ] **Step 5: Manual runtime verification**

Run:

```bash
/home/zazaki/Projects/cpp-linux-monitor-tui/build/monitor_tui
```

Verify:

- `j/k` changes the selected process
- the right pane follows the selected process
- `K` / `R` target the selected process
- after a process disappears or the filtered list shrinks, selection clamps to the nearest valid row

- [ ] **Step 6: Commit runtime feedback wiring**

```bash
git -C /home/zazaki/Projects/cpp-linux-monitor-tui add src/app/application.cpp tests/ui/app_controller_test.cpp
git -C /home/zazaki/Projects/cpp-linux-monitor-tui commit -m "feat: align runtime action feedback with detail pane"
```

## Self-Review

### Spec coverage

- Right-side detail pane: covered by Task 2.
- Minimal detail fields: covered by Task 2 assertions and render output.
- Selection reconciliation after actions/list shrink: covered by Tasks 1 and 3.
- Action result feedback remains in the status bar: preserved in Task 3.

### Placeholder scan

- This plan contains no `TODO`, `TBD`, or “implement later” placeholders.

### Type consistency

- The plan consistently uses `selected_process_index`, `process_window_start`, `process_window_height`, and the existing `status_text_` channel for runtime feedback.
