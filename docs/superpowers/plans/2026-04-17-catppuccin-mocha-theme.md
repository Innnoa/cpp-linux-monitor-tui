# Catppuccin Mocha Theme Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a fixed Catppuccin Mocha theme to the existing FTXUI dashboard and shared input bar without changing current interaction behavior.

**Architecture:** Introduce a shared header-only theme definition for Mocha colors and styling helpers, then wire dashboard panels and the shared input bar to that theme. Keep controller logic unchanged; only rendering paths gain color styling.

**Tech Stack:** `C++20`, `FTXUI`, existing `AppController`, existing Catch2 test suite

---

### Task 1: Add a render-level regression test for themed colors

**Files:**
- Modify: `tests/ui/dashboard_view_test.cpp`

- [ ] **Step 1: Write the failing test**

Add a test that renders the dashboard to an `ftxui::Screen`, locates the focused `Processes *` title and the selected process row, and asserts they use themed colors.

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --test-dir build --output-on-failure -R "dashboard renders Catppuccin Mocha colors"`
Expected: FAIL because no color-specific assertions are currently satisfied.

- [ ] **Step 3: Write minimal implementation**

Add only the theme support needed to make the new color assertions pass.

- [ ] **Step 4: Run test to verify it passes**

Run: `ctest --test-dir build --output-on-failure -R "dashboard renders Catppuccin Mocha colors"`
Expected: PASS

### Task 2: Share Mocha theme helpers across dashboard and input bar

**Files:**
- Create: `src/ui/theme.h`
- Modify: `src/ui/dashboard_view.cpp`
- Modify: `src/app/application.cpp`

- [ ] **Step 1: Add shared theme constants**

Create a small header with the fixed Mocha palette and helper functions for focused titles, panel borders, and status colors.

- [ ] **Step 2: Apply theme to dashboard panels**

Style the resource boxes, process list, detail pane, and status bar with the shared theme.

- [ ] **Step 3: Apply theme to the shared input bar**

Style the active `:` and `/` input bar in `application.cpp` with the same theme.

- [ ] **Step 4: Run focused dashboard/application tests**

Run: `ctest --test-dir build --output-on-failure -R "dashboard|application"`
Expected: PASS

### Task 3: Reconfirm full compatibility

**Files:**
- Modify: `tests/ui/dashboard_view_test.cpp` (only if the final assertions need one more narrow regression)

- [ ] **Step 1: Run the full test suite**

Run: `ctest --test-dir build --output-on-failure`
Expected: all tests pass.

- [ ] **Step 2: Smoke-check non-interactive render output**

Run: `./build/monitor_tui | sed -n '1,20p'`
Expected: a single colored frame is emitted and the process exits.
