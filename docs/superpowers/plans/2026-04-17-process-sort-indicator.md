# Process Sort Indicator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show the current process sort key in the process panel title while preserving the existing `s` cycle behavior.

**Architecture:** Keep sorting logic in `AppController` unchanged. Add a small render helper that maps the current `ProcessSortKey` to a title suffix and reuse the existing process panel title rendering path.

**Tech Stack:** `C++20`, `FTXUI`, existing `AppController`, existing Catch2 dashboard tests

---

### Task 1: Add render-level regression for the sort indicator

**Files:**
- Modify: `tests/ui/dashboard_view_test.cpp`

- [ ] **Step 1: Write the failing test**
- [ ] **Step 2: Run the focused test and confirm it fails**
- [ ] **Step 3: Implement the minimal title suffix**
- [ ] **Step 4: Re-run the focused test and confirm it passes**

### Task 2: Reconfirm compatibility

**Files:**
- Modify: `src/ui/dashboard_view.cpp`

- [ ] **Step 1: Run dashboard/controller focused tests**
- [ ] **Step 2: Run full `ctest`**
