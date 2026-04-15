# Process Detail Pane Runtime Design

## Summary

Add a right-side always-visible process detail pane to `cpp-linux-monitor-tui` so the currently selected process has stable contextual details, while keeping the current runtime loop and controller state model intact.

## Scope

This iteration adds:

- a right-side process detail pane in the lower area
- minimal selected-process fields
- selection reconciliation when the visible process list shrinks after refresh
- action feedback consistency between list, detail pane, and status bar

This iteration does not add:

- a modal detail inspector
- a dense `/proc/<pid>/status` dump
- a separate detail view-model layer

## Layout

- Keep the current top resource summary area unchanged.
- Split the lower area into two columns:
  - left: process list
  - right: selected process detail card
- Left column remains dominant.
- Right column has a fixed width suitable for the agreed minimal field set.

## Detail Fields

The right-side detail pane shows only:

- `PID`
- `Name`
- `User`
- `State`
- `Memory %`
- `Nice`

And one short action hint row:

- `K kill`
- `R renice`

## Empty State

When no visible process exists:

- left side shows `no matching processes`
- right side shows `No process selected`

## Selection Rules

- The detail pane always follows the currently selected visible process.
- If the visible list shrinks and the current selected index becomes invalid:
  - clamp to the nearest valid row
  - if no rows remain, reset selection/window state to `0`

## Action Feedback

### `kill`

- On success:
  - status bar shows `ok`
  - after the next sample, if the process has exited, selection falls to the nearest still-visible row
- On failure:
  - status bar shows the action error
  - selection and detail remain where they are

### `renice`

- On success:
  - status bar shows `ok`
  - the next sample refresh updates the detail pane’s `Nice`
- On failure:
  - status bar shows the action error
  - selection and detail remain where they are

## Architecture

- Keep the current controller + renderer structure.
- Renderer derives the detail pane directly from:
  - filtered visible process list
  - selected process index
- No separate detail view-model layer in this iteration.
- Runtime loop continues to rely on controller state plus next-sample reconciliation.

## Testing

Required tests:

- selected process details appear in the right pane
- changing selected row changes the detail pane
- empty visible list shows `No process selected`
- when visible list shrinks, selection clamps to nearest valid row
- existing action feedback remains visible in the status bar after kill/renice attempts

Runtime verification:

- start the TUI
- move selection with `j/k`
- confirm the right pane tracks the selected process
- trigger `K` / `R` on different selected rows
- confirm selection stays coherent after refresh

## Non-Goals

- No modal details view
- No per-process charts
- No extra fields beyond the agreed six
