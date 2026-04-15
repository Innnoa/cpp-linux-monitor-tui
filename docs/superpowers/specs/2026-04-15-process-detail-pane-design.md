# Process Detail Pane Design

## Summary

Add a right-side process detail pane to `cpp-linux-monitor-tui` so the currently selected process has a stable, always-visible detail card, and tighten runtime feedback so `kill` / `renice` outcomes are reflected immediately in the status bar and naturally reconcile on the next sample refresh.

## Scope

This iteration adds:

- a right-side always-visible detail pane
- minimal selected-process detail fields
- runtime handling so action results and selection recovery feel coherent

This iteration does not add:

- a full-screen detail modal
- a dense `/proc/<pid>/status` field dump
- a second standalone detail view module

## Layout Direction

- Keep the current top resource summary area.
- Split the lower area into two columns:
  - left: process list
  - right: selected process details
- The process list remains the primary navigation surface.
- The right pane is a live projection of the selected list row.

## Detail Fields

The right-side detail pane should show these fields only:

- `PID`
- `Name`
- `User`
- `State`
- `Memory %`
- `Nice`

It should also include a short hint row:

- `K kill`
- `R renice`

## Selection Behavior

- The detail pane always follows the current selected process.
- If the list is empty or the filter removes every process:
  - show an empty-state detail card
  - clear selection/window state to `0`
- If the selected process disappears after a successful `kill`:
  - move selection to the nearest valid row in the newly sampled visible list
  - if the list becomes empty, selection resets to `0`

## Action Feedback Rules

### `kill`

- On success:
  - status bar shows `ok`
  - the next sample refresh removes the process if it has exited
  - selection falls to the nearest remaining visible row
- On failure:
  - status bar shows the existing action error message
  - selection stays on the same row if still present

### `renice`

- On success:
  - status bar shows `ok`
  - the next sample refresh updates the `Nice` field in the detail pane
- On failure:
  - status bar shows the existing action error message
  - selection and detail focus stay where they are

## Architecture

- Keep this iteration inside the current controller + renderer structure.
- Do not introduce a separate `process_detail_view_model` layer yet.
- Add only the minimum controller state needed for:
  - selected row stability
  - visible list reconciliation after refresh
- Renderer should derive the detail pane directly from the selected visible process.

## Testing

Required tests:

- selected process fields appear in the detail pane
- changing selected row changes the detail pane
- empty/filtered-out list shows an empty-state detail pane
- if visible process count shrinks, selection is clamped to the nearest valid row
- action success/failure status still appears correctly after runtime action handling

Runtime verification:

- start the TUI
- move selection with `j/k`
- confirm the right pane tracks the selected process
- trigger `K` / `R` on different selected rows and confirm the status/result feels coherent

## Non-Goals

- No modal process inspector
- No historical per-process charts
- No deep field expansion beyond the six agreed fields
