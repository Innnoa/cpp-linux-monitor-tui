# Command Bar Execution Design

## Summary

Add a minimal executable command bar to `cpp-linux-monitor-tui` so typed commands can mutate controller state without introducing a full parser subsystem. This iteration is intentionally limited to internal commands and status-bar feedback.

## Scope

Commands in scope:

- `sort cpu|memory|pid|name`
- `filter <text>`
- `clear`
- `quit`

Feedback rules:

- Command execution results appear in the bottom status bar.
- Invalid commands return `unknown command: ...` plus short examples.
- Executed command text does not remain active after dispatch.

## Command Semantics

### `sort <field>`

- Updates `AppController` sort state.
- Accepted fields: `cpu`, `memory`, `pid`, `name`.
- Success message format: `sort: <field>`.

### `filter <text>`

- Updates the same filter state used by `/`.
- Success message format: `filter: <text>`.
- Empty or missing filter text is treated as invalid input.

### `clear`

- Clears the active filter and command text state.
- Success message: `cleared`.

### `quit`

- Sets a controller-owned quit flag that the runtime loop checks after command execution.
- Success message: `quitting`.

### Invalid commands

- Do not mutate sort/filter/quit state.
- Status message format:
  `unknown command: <input> (try: sort cpu, filter postgres, clear, quit)`

## Architecture

- Keep command dispatch inside `AppController`.
- Add a small `execute_command(std::string)` entry point rather than a new parser module.
- Reuse existing controller state:
  - `sort_key_`
  - `filter_query_`
  - `command_text_`
  - `status_text_`
- Add a controller-owned `should_quit_` flag for runtime loop exit.

## Runtime Behavior

- `:` still enters command mode.
- Typing still updates `command_text_`.
- Pressing `Enter` in command mode dispatches `execute_command(...)`.
- The runtime loop checks `should_quit()` after command execution and exits when set.

## Testing

Required tests:

- `sort name` updates sort state and status text.
- `filter postgres` updates filter state and status text.
- `clear` clears filter state and command text.
- `quit` sets quit state and status text.
- Invalid command returns the expected error text and leaves quit state unset.

Runtime verification:

- `:sort pid`
- `:filter bash`
- `:clear`
- `:quit`

## Non-Goals

- No external/system command execution.
- No fuzzy command correction.
- No standalone command parser module.
- No action commands such as `kill <pid>` or `renice <pid> <value>` in this iteration.
