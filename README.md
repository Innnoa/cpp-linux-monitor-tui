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

- In a real TTY, `monitor_tui` runs a minimal live dashboard loop and refreshes from local `/proc` data.
- In a non-interactive environment, it renders one snapshot and exits so smoke checks do not hang.
- The current runtime supports quitting with `q`, opening filter/command modes, and showing `kill` / `renice` prompts for the top listed process.

## Current controls

- `h/l` or `Tab` / `Shift-Tab`: move focus
- `j/k`: move process selection
- `gg` / `G`: jump to start/end
- `/`: open filter
- `s`: cycle sort
- `:`: command bar
- `K`: show `kill` confirmation for the top listed process
- `R`: show `renice` prompt for the top listed process
- `Esc`: leave filter / command / confirm modes
- `q`: quit
