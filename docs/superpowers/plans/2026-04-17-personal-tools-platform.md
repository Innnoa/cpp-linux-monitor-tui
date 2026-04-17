# Personal Tools Platform Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refactor the current monitor TUI into a reusable personal Linux tools platform, then build additional tools on top of the same system and UI foundations.

**Architecture:** Keep the current monitor app working while extracting reusable pieces into two stable layers: a system/core layer for data collection and actions, and a UI/core layer for TUI primitives and interaction flow. New tools such as logs and services should then be implemented as thin apps that reuse those layers instead of duplicating monitor-specific logic.

**Tech Stack:** `C++20`, `FTXUI`, Linux `/proc`, future `systemd`/`journalctl` integration, existing Catch2 suite

---

## Target Repository Shape

### Stable Layout

- `src/core/system/`
  - Linux-facing collection and action code
  - process actions
  - procfs readers
  - snapshot/history primitives
  - future journal/systemd adapters
- `src/core/ui/`
  - reusable TUI building blocks
  - controller state machines
  - list/table rendering helpers
  - input/status/focus/theme helpers
  - chart primitives
- `src/domain/monitor/`
  - monitor-specific models and app glue
  - cpu/memory/disk/network/process composition
- `src/apps/monitor/`
  - executable-facing wiring for the current monitor tool
- `src/apps/logs/`
  - future journal/log viewer
- `src/apps/services/`
  - future systemd service viewer/controller
- `tests/core/`
  - reusable system/UI layer tests
- `tests/domain/monitor/`
  - monitor-specific tests
- `tests/apps/`
  - app wiring and runtime smoke tests

### Mapping From Current Files

- `src/actions/process_actions.*`
  -> `src/core/system/process_actions.*`
- `src/store/history_buffer.h`
  -> `src/core/system/history_buffer.h`
- `src/store/snapshot_store.*`
  -> `src/core/system/snapshot_store.*`
- `src/ui/app_controller.*`
  -> split into `src/core/ui/app_controller.*` and later smaller UI state helpers if it grows
- `src/ui/theme.h`
  -> `src/core/ui/theme.h`
- `src/ui/dashboard_view.*`
  -> mostly `src/domain/monitor/dashboard_view.*`
- `src/collector/*.cpp`
  -> split between `src/core/system/procfs/*` and `src/domain/monitor/*`
- `src/app/application.*`
  -> `src/apps/monitor/application.*`
- `src/app/sampling_worker.*`
  -> `src/domain/monitor/sampling_worker.*`
- `src/model/system_snapshot.h`
  -> `src/domain/monitor/system_snapshot.h`

---

## Phase 1: Stabilize The Current Monitor As A Reusable Base

**Outcome:** the existing monitor remains fully usable, but its boundaries are cleaned up so future tools do not need to copy monitor-specific code.

### Task 1: Extract reusable system layer

**Files:**
- Move/modify: `src/actions/process_actions.*`
- Move/modify: `src/store/history_buffer.h`
- Move/modify: `src/store/snapshot_store.*`
- Create: `src/core/system/README.md`
- Move tests: `tests/actions/process_actions_test.cpp`
- Move tests: `tests/store/snapshot_store_test.cpp`

**What this phase should produce:**
- one place for process actions and permission semantics
- one place for reusable history/buffering primitives
- no monitor-specific naming in these files

### Task 2: Extract reusable UI layer

**Files:**
- Move/modify: `src/ui/app_controller.*`
- Move/modify: `src/ui/theme.h`
- Create: `src/core/ui/status_text.h`
- Create: `src/core/ui/list_navigation.h`
- Create: `src/core/ui/README.md`
- Move tests: `tests/ui/app_controller_test.cpp`

**What this phase should produce:**
- reusable focus/input/status/list-selection logic
- monitor app stops being the only consumer in the design
- UI state transitions are no longer tied to “dashboard” naming

### Task 3: Keep monitor-specific rendering isolated

**Files:**
- Move/modify: `src/ui/dashboard_view.*`
- Move/modify: `src/model/system_snapshot.h`
- Move/modify: `src/collector/*.cpp`
- Move tests: `tests/ui/dashboard_view_test.cpp`
- Move tests: `tests/collector/*.cpp`

**What this phase should produce:**
- monitor domain code stays together
- procfs parsing and monitor rendering are separated
- future tools can reuse core UI without dragging monitor panels along

### Task 4: Re-home app wiring

**Files:**
- Move/modify: `src/app/application.*`
- Move/modify: `src/app/sampling_worker.*`
- Move/modify: `src/main.cpp`
- Move tests: `tests/app/application_test.cpp`
- Move tests: `tests/app/app_config_test.cpp`

**What this phase should produce:**
- executable wiring lives under `apps/monitor`
- monitor-specific runtime boot remains intact

---

## Phase 2: Finish The Monitor Tool You Actually Want To Use

**Outcome:** the monitor app becomes a serious daily-use tool before you build siblings on the same platform.

### Priority Features

- real charts instead of fixed sparkline placeholders
- per-core CPU view
- richer memory/disk/network summaries
- process tree
- pause refresh
- follow selected process
- stronger filtering: `pid/name/user/cmdline`
- more process actions: signal menu beyond kill/renice
- fuller process detail pane

### Recommended File Additions

- `src/domain/monitor/charts/*`
- `src/domain/monitor/process_tree.*`
- `src/domain/monitor/process_filter.*`
- `src/domain/monitor/process_detail.*`
- `src/core/ui/chart_widget.*`
- `src/core/ui/table_widget.*`

### Exit Criteria

- you can use this tool for actual “find the bad process” work
- no placeholder graphs remain
- process view is the main workflow, not just a demo list

---

## Phase 3: Build The Next Tool On The Same Base

**Outcome:** prove the architecture by shipping a second app that reuses the base instead of bypassing it.

### Recommended Next App: Logs

**Why first:**
- closest interaction model to current monitor
- list + filter + detail + follow + pause all reuse existing concepts

**Target layout:**
- `src/domain/logs/`
- `src/apps/logs/`
- `tests/domain/logs/`

**Target capabilities:**
- tail-like live stream
- keyword filter
- severity/source filter
- selected-entry detail pane
- pause/follow behavior

### Third App: Services

**Why next:**
- same list/detail/action pattern
- good fit after process actions are already structured

**Target capabilities:**
- list systemd units
- filter by name/state
- start/stop/restart
- view status/details/log excerpt

---

## What To Explicitly Defer

- mouse support
- theme switching UI
- packaging/distribution work
- plugin systems
- cross-platform support
- “btop parity” as a goal by itself

These are valuable only after the reusable base and your top 2-3 daily tools exist.

---

## Execution Order Recommendation

1. Reorganize current monitor into `core + domain + apps`
2. Finish the monitor features you personally need most
3. Build `logs`
4. Build `services`
5. Revisit optional polish only after those are in use
