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

## Features

- **Left-right panel layout** — CPU and Memory stats on the left, Disk and Network on the right
- **Progress bars** with color-coded thresholds: green (< 50%), orange (50–80%), red (> 80%)
- **Sparkline trend charts** showing historical data for CPU, Memory, Disk, and Network
- **Process list** with color coding: zombie → red, high CPU → red, running → green
- **Ring buffer** history storage for efficient trend data
- **Render cache** for optimized refresh performance

## Current status

- In a real TTY, `monitor_tui` runs a minimal live dashboard loop and refreshes from local `/proc` data.
- In a non-interactive environment, it renders one snapshot and exits so smoke checks do not hang.
- The current runtime supports quitting with `q`, opening filter/command modes, and showing `kill` / `renice` prompts for the top listed process.

## Current controls

- `h/l`: move focus
- `j/k`: move process selection
- `/`: open filter and apply live filtering to the process list by name, or by pid when the query is numeric
- `s`: cycle sort
- `:`: command bar
- `K`: open `kill` confirmation for the selected process
- `R`: open `renice` prompt for the selected process
- `y/N`: confirm or cancel `kill`
- type digits then `Enter`: submit `renice`
- `Esc`: leave filter / command / confirm modes
- `q`: quit

## Current command-bar commands

- `:sort cpu|memory|pid|name`
- `:filter <text>` (`<text>` is matched against process name, or against pid when numeric)
- `:clear`
- `:quit`
- `:kill <pid>`
- `:renice <pid> <value>`
