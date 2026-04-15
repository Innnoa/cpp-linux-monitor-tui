# FTXUI Component Event Loop Design

## Summary

Replace the current hand-written raw input loop with a component-driven `FTXUI` event loop while preserving the existing process list, right-side detail pane, and status feedback model. The default interaction focus remains on the process list, and command/filter entry uses a single shared transient input component.

## Scope

This iteration adds:

- a top-level `FTXUI` component tree for runtime interaction
- process-list-first focus handling
- one shared transient input component for both command and filter entry
- timer/event-driven redraw integration under `FTXUI`

This iteration keeps:

- current `/proc`-based sampling
- current controller state model
- non-TTY “render once and exit” behavior for smoke checks

This iteration does not add:

- a permanent bottom command line
- a separate filter widget and command widget
- a full command palette or fuzzy completion

## Focus Model

- Default focus is the process list.
- Process list remains the primary interaction surface.
- Right-side detail pane is read-only in this iteration.
- The bottom bar normally shows status only.
- Entering `:` or `/` temporarily moves focus into the shared input component.

## Shared Input Component

- One input component is reused for both:
  - command mode (`:`)
  - filter mode (`/`)
- It displays the active prefix inline:
  - `:sort cpu`
  - `/postgres`
- `Enter` dispatches based on the current mode:
  - command mode → command execution
  - filter mode → filter update
- `Esc` cancels input and returns focus to the process list.
- After submit/cancel, the bottom area collapses back to the normal status bar.

## Component Tree

Top-level component layout:

1. Resource overview area
2. Main lower split area
   - left: process list component
   - right: selected process detail pane
3. Bottom bar
   - normal mode: status line
   - input mode: shared transient input component

## Event Flow

### Process list component

Handles:

- `j/k`
- selection movement
- action shortcuts `K` / `R`
- `:` to open command input
- `/` to open filter input

### Shared input component

Handles:

- text entry
- backspace
- `Enter`
- `Esc`

### Refresh model

- Stop using the hand-written raw `select + read` loop as the main interaction engine.
- Use `FTXUI`’s event/render loop as the primary runtime driver.
- Sampling and redraw scheduling should be triggered from the `FTXUI` runtime rather than a custom blocking terminal loop.

## Runtime Compatibility

- TTY mode:
  - uses the `FTXUI` component event loop
  - interactive
- Non-TTY mode:
  - keep the current single-frame render-and-exit path
  - do not force component loop startup in smoke/non-interactive environments

## Testing

Required tests:

- entering `:` activates the shared input in command mode
- entering `/` activates the shared input in filter mode
- `Esc` from shared input returns focus to the process list
- `Enter` dispatches correctly based on the active input mode
- process list remains the default focus after startup
- existing list/detail rendering tests remain green

Runtime verification:

- start the TUI in a real terminal
- confirm focus starts on the process list
- press `:` and type a command, then `Enter`
- press `/` and type a filter, then `Enter`
- press `Esc` from input mode and confirm the list regains focus

## Non-Goals

- No full component rewrite of every visual unit beyond what is needed to replace the main loop
- No change to current command vocabulary
- No new command history, completion, or suggestions in this iteration
