# cpp-linux-monitor-tui

Local Linux terminal monitor built with `C++` and `FTXUI`.

## Build

~~~bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
~~~

## Run

~~~bash
./build/monitor_tui
~~~

## Current status

The current milestone starts the binary and prints a placeholder message:

`replace bootstrap with FTXUI event loop`

The full interactive dashboard shell is scaffolded in code, but the runtime event loop is not wired yet.

## Planned controls

- `h/l` or `Tab` / `Shift-Tab`: move focus
- `j/k`: move process selection
- `gg` / `G`: jump to start/end
- `/`: open filter
- `s`: cycle sort
- `:`: command bar
- `K`: confirm `kill`
- `R`: submit `renice`
