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

## Default controls

- `h/l` or `Tab` / `Shift-Tab`: move focus
- `j/k`: move process selection
- `gg` / `G`: jump to start/end
- `/`: open filter
- `s`: cycle sort
- `:`: command bar
- `K`: confirm `kill`
- `R`: submit `renice`
