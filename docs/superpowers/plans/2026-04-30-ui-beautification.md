# UI 美化与性能优化实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 对 cpp-linux-monitor-tui 进行全面的 UI 美化和性能优化，包括进度条、趋势图、进程列表改进和性能优化。

**Architecture:** 在现有 FTXUI 架构基础上，添加新的 UI 组件（进度条、趋势图），重构布局为左右分栏，优化进程列表显示，并实现性能优化机制（脏标记、缓存、环形缓冲区）。

**Tech Stack:** C++20, FTXUI v5.0.0, Linux /proc 文件系统

---

## 文件结构

### 新增文件
- `src/ui/progress_bar.h` - 进度条组件头文件
- `src/ui/progress_bar.cpp` - 进度条组件实现
- `src/ui/sparkline_chart.h` - Sparkline 趋势图组件头文件
- `src/ui/sparkline_chart.cpp` - Sparkline 趋势图组件实现
- `src/ui/process_list.h` - 进程列表组件头文件
- `src/ui/process_list.cpp` - 进程列表组件实现
- `src/model/history_data.h` - 历史数据存储（环形缓冲区）
- `src/model/history_data.cpp` - 历史数据存储实现
- `tests/ui/progress_bar_test.cpp` - 进度条单元测试
- `tests/ui/sparkline_chart_test.cpp` - 趋势图单元测试
- `tests/ui/process_list_test.cpp` - 进程列表单元测试
- `tests/model/history_data_test.cpp` - 历史数据单元测试

### 修改文件
- `src/ui/dashboard_view.cpp` - 重构布局为左右分栏
- `src/ui/theme.h` - 添加新的颜色常量和工具函数
- `src/model/system_snapshot.h` - 扩展 ProcessInfo 结构
- `src/app/application.cpp` - 集成新的 UI 组件和历史数据
- `CMakeLists.txt` - 添加新的源文件

---

## Task 1: 实现环形缓冲区（历史数据存储）

**Files:**
- Create: `src/model/history_data.h`
- Create: `src/model/history_data.cpp`
- Create: `tests/model/history_data_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 创建环形缓冲区头文件**

```cpp
// src/model/history_data.h
#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace monitor::model {

template<typename T, std::size_t N>
class RingBuffer {
public:
    static_assert(N > 0, "RingBuffer capacity must be greater than 0");

    void push(T value) {
        buffer_[head_] = value;
        head_ = (head_ + 1) % N;
        if (size_ < N) {
            ++size_;
        }
    }

    std::span<const T> data() const {
        if (size_ < N) {
            return std::span<const T>(buffer_.data(), size_);
        }
        // 返回从 head 开始的完整缓冲区
        return std::span<const T>(buffer_.data(), N);
    }

    std::size_t size() const {
        return size_;
    }

    bool full() const {
        return size_ == N;
    }

    void clear() {
        head_ = 0;
        size_ = 0;
    }

    T latest() const {
        if (size_ == 0) {
            return T{};
        }
        std::size_t index = (head_ == 0) ? (N - 1) : (head_ - 1);
        return buffer_[index];
    }

private:
    std::array<T, N> buffer_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

struct HistoryData {
    RingBuffer<double, 60> cpu_history;
    RingBuffer<double, 60> memory_history;
    RingBuffer<double, 60> disk_read_history;
    RingBuffer<double, 60> disk_write_history;
    RingBuffer<double, 60> network_rx_history;
    RingBuffer<double, 60> network_tx_history;

    void clear() {
        cpu_history.clear();
        memory_history.clear();
        disk_read_history.clear();
        disk_write_history.clear();
        network_rx_history.clear();
        network_tx_history.clear();
    }
};

}  // namespace monitor::model
```

- [ ] **Step 2: 创建环形缓冲区实现文件**

```cpp
// src/model/history_data.cpp
#include "model/history_data.h"

// 环形缓冲区是模板类，实现已在头文件中
// 此文件用于未来可能的扩展
```

- [ ] **Step 3: 创建环形缓冲区单元测试**

```cpp
// tests/model/history_data_test.cpp
#include <catch2/catch_test_macros.hpp>
#include "model/history_data.h"

TEST_CASE("RingBuffer basic operations", "[ring_buffer]") {
    monitor::model::RingBuffer<int, 3> buffer;

    SECTION("Initial state is empty") {
        REQUIRE(buffer.size() == 0);
        REQUIRE(buffer.full() == false);
    }

    SECTION("Push increases size") {
        buffer.push(1);
        REQUIRE(buffer.size() == 1);
        REQUIRE(buffer.full() == false);

        buffer.push(2);
        REQUIRE(buffer.size() == 2);

        buffer.push(3);
        REQUIRE(buffer.size() == 3);
        REQUIRE(buffer.full() == true);
    }

    SECTION("Push beyond capacity overwrites oldest") {
        buffer.push(1);
        buffer.push(2);
        buffer.push(3);
        buffer.push(4);

        REQUIRE(buffer.size() == 3);
        REQUIRE(buffer.full() == true);

        auto data = buffer.data();
        REQUIRE(data[0] == 2);
        REQUIRE(data[1] == 3);
        REQUIRE(data[2] == 4);
    }

    SECTION("Latest returns most recent value") {
        buffer.push(10);
        REQUIRE(buffer.latest() == 10);

        buffer.push(20);
        REQUIRE(buffer.latest() == 20);

        buffer.push(30);
        REQUIRE(buffer.latest() == 30);

        buffer.push(40);
        REQUIRE(buffer.latest() == 40);
    }

    SECTION("Clear resets buffer") {
        buffer.push(1);
        buffer.push(2);
        buffer.clear();

        REQUIRE(buffer.size() == 0);
        REQUIRE(buffer.full() == false);
    }
}

TEST_CASE("HistoryData operations", "[history_data]") {
    monitor::model::HistoryData history;

    SECTION("Initial state is empty") {
        REQUIRE(history.cpu_history.size() == 0);
        REQUIRE(history.memory_history.size() == 0);
    }

    SECTION("Push and retrieve data") {
        history.cpu_history.push(45.5);
        history.cpu_history.push(50.2);

        REQUIRE(history.cpu_history.size() == 2);
        REQUIRE(history.cpu_history.latest() == 50.2);
    }

    SECTION("Clear resets all histories") {
        history.cpu_history.push(1.0);
        history.memory_history.push(2.0);
        history.clear();

        REQUIRE(history.cpu_history.size() == 0);
        REQUIRE(history.memory_history.size() == 0);
    }
}
```

- [ ] **Step 4: 更新 CMakeLists.txt 添加新文件**

在 `MONITOR_CORE_SOURCES` 中添加：
```cmake
set(MONITOR_CORE_SOURCES
  src/store/snapshot_store.cpp
  src/app/application.cpp
  src/app/sampling_worker.cpp
  src/collector/cpu_collector.cpp
  src/collector/disk_collector.cpp
  src/collector/memory_collector.cpp
  src/collector/network_collector.cpp
  src/collector/process_collector.cpp
  src/actions/process_actions.cpp
  src/model/history_data.cpp  # 新增
  src/ui/app_controller.cpp)
```

在 `MONITOR_TEST_SOURCES` 中添加：
```cmake
set(MONITOR_TEST_SOURCES
  tests/app/app_config_test.cpp
  tests/app/application_test.cpp
  tests/store/snapshot_store_test.cpp
  tests/collector/system_collectors_test.cpp
  tests/collector/process_collector_test.cpp
  tests/actions/process_actions_test.cpp
  tests/model/history_data_test.cpp  # 新增
  tests/ui/app_controller_test.cpp)
```

- [ ] **Step 5: 编译并运行测试**

```bash
cd /home/zazaki/Projects/cpp-linux-monitor-tui
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DMONITOR_FETCH_DEPS=ON
cmake --build build
./build/monitor_tests "[ring_buffer]"
```

预期输出：所有测试通过

- [ ] **Step 6: 提交**

```bash
git add src/model/history_data.h src/model/history_data.cpp tests/model/history_data_test.cpp CMakeLists.txt
git commit -m "feat: add ring buffer for history data storage"
```

---

## Task 2: 实现进度条组件

**Files:**
- Create: `src/ui/progress_bar.h`
- Create: `src/ui/progress_bar.cpp`
- Create: `tests/ui/progress_bar_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 创建进度条头文件**

```cpp
// src/ui/progress_bar.h
#pragma once

#include <string>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace monitor::ui {

struct ProgressBarConfig {
    int width = 20;
    char filled_char = '■';
    char empty_char = '□';
    bool show_percentage = true;
    bool show_value = false;
};

ftxui::Element progress_bar(
    double value,
    double max_value,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color);

ftxui::Element progress_bar(
    double percentage,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color);

std::string format_progress_bar(
    double percentage,
    int width,
    char filled_char,
    char empty_char);

ftxui::Color threshold_color(
    double percentage,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color);

}  // namespace monitor::ui
```

- [ ] **Step 2: 创建进度条实现文件**

```cpp
// src/ui/progress_bar.cpp
#include "ui/progress_bar.h"

#include <iomanip>
#include <sstream>
#include <algorithm>

namespace monitor::ui {

namespace {

constexpr double kLowThreshold = 50.0;
constexpr double kMediumThreshold = 80.0;

}  // namespace

ftxui::Color threshold_color(
    double percentage,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color) {
    if (percentage >= kMediumThreshold) {
        return high_color;
    }
    if (percentage >= kLowThreshold) {
        return medium_color;
    }
    return low_color;
}

std::string format_progress_bar(
    double percentage,
    int width,
    char filled_char,
    char empty_char) {
    const auto clamped = std::clamp(percentage, 0.0, 100.0);
    const auto filled_count = static_cast<int>((clamped / 100.0) * width);
    const auto empty_count = width - filled_count;

    std::string result;
    result.reserve(width);
    result.append(filled_count, filled_char);
    result.append(empty_count, empty_char);
    return result;
}

ftxui::Element progress_bar(
    double value,
    double max_value,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color) {
    if (max_value <= 0.0) {
        return ftxui::text(std::string(config.width, config.empty_char));
    }

    const auto percentage = (value / max_value) * 100.0;
    return progress_bar(percentage, config, low_color, medium_color, high_color);
}

ftxui::Element progress_bar(
    double percentage,
    const ProgressBarConfig& config,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color) {
    const auto bar = format_progress_bar(
        percentage, config.width, config.filled_char, config.empty_char);

    const auto color = threshold_color(percentage, low_color, medium_color, high_color);

    std::ostringstream display;
    display << bar;
    if (config.show_percentage) {
        display << " " << std::fixed << std::setprecision(0) << percentage << "%";
    }

    return ftxui::text(display.str()) | ftxui::color(color);
}

}  // namespace monitor::ui
```

- [ ] **Step 3: 创建进度条单元测试**

```cpp
// tests/ui/progress_bar_test.cpp
#include <catch2/catch_test_macros.hpp>
#include "ui/progress_bar.h"

TEST_CASE("format_progress_bar", "[progress_bar]") {
    SECTION("Empty bar at 0%") {
        auto result = monitor::ui::format_progress_bar(0.0, 10, '■', '□');
        REQUIRE(result == "□□□□□□□□□□");
    }

    SECTION("Full bar at 100%") {
        auto result = monitor::ui::format_progress_bar(100.0, 10, '■', '□');
        REQUIRE(result == "■■■■■■■■■■");
    }

    SECTION("Half bar at 50%") {
        auto result = monitor::ui::format_progress_bar(50.0, 10, '■', '□');
        REQUIRE(result == "■■■■■□□□□□");
    }

    SECTION("Clamps values above 100%") {
        auto result = monitor::ui::format_progress_bar(150.0, 10, '■', '□');
        REQUIRE(result == "■■■■■■■■■■");
    }

    SECTION("Clamps values below 0%") {
        auto result = monitor::ui::format_progress_bar(-10.0, 10, '■', '□');
        REQUIRE(result == "□□□□□□□□□□");
    }
}

TEST_CASE("threshold_color", "[progress_bar]") {
    auto low = ftxui::Color::Green;
    auto medium = ftxui::Color::Yellow;
    auto high = ftxui::Color::Red;

    SECTION("Low threshold") {
        auto color = monitor::ui::threshold_color(30.0, low, medium, high);
        REQUIRE(color == low);
    }

    SECTION("Medium threshold") {
        auto color = monitor::ui::threshold_color(60.0, low, medium, high);
        REQUIRE(color == medium);
    }

    SECTION("High threshold") {
        auto color = monitor::ui::threshold_color(90.0, low, medium, high);
        REQUIRE(color == high);
    }

    SECTION("Boundary at 50%") {
        auto color = monitor::ui::threshold_color(50.0, low, medium, high);
        REQUIRE(color == medium);
    }

    SECTION("Boundary at 80%") {
        auto color = monitor::ui::threshold_color(80.0, low, medium, high);
        REQUIRE(color == high);
    }
}
```

- [ ] **Step 4: 更新 CMakeLists.txt 添加新文件**

在 `MONITOR_CORE_SOURCES` 中添加：
```cmake
src/ui/progress_bar.cpp  # 新增
```

在 `MONITOR_TEST_SOURCES` 中添加：
```cmake
tests/ui/progress_bar_test.cpp  # 新增
```

- [ ] **Step 5: 编译并运行测试**

```bash
cd /home/zazaki/Projects/cpp-linux-monitor-tui
cmake --build build
./build/monitor_tests "[progress_bar]"
```

预期输出：所有测试通过

- [ ] **Step 6: 提交**

```bash
git add src/ui/progress_bar.h src/ui/progress_bar.cpp tests/ui/progress_bar_test.cpp CMakeLists.txt
git commit -m "feat: add progress bar component with threshold colors"
```

---

## Task 3: 实现 Sparkline 趋势图组件

**Files:**
- Create: `src/ui/sparkline_chart.h`
- Create: `src/ui/sparkline_chart.cpp`
- Create: `tests/ui/sparkline_chart_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 创建 Sparkline 头文件**

```cpp
// src/ui/sparkline_chart.h
#pragma once

#include <span>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace monitor::ui {

struct SparklineConfig {
    int height = 2;
    int width = 40;
    bool show_latest = true;
    bool show_min_max = false;
};

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    const SparklineConfig& config = {});

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    int width);

std::string format_sparkline(
    std::span<const double> data,
    int width);

}  // namespace monitor::ui
```

- [ ] **Step 2: 创建 Sparkline 实现文件**

```cpp
// src/ui/sparkline_chart.cpp
#include "ui/sparkline_chart.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace monitor::ui {

namespace {

constexpr wchar_t kSparkChars[] = {
    L'▁', L'▂', L'▃', L'▄', L'▅', L'▆', L'▇', L'█'
};
constexpr int kSparkLevels = 8;

char spark_char(double normalized) {
    const auto index = std::clamp(
        static_cast<int>(normalized * kSparkLevels), 0, kSparkLevels - 1);
    return static_cast<char>(kSparkChars[index]);
}

}  // namespace

std::string format_sparkline(
    std::span<const double> data,
    int width) {
    if (data.empty() || width <= 0) {
        return {};
    }

    // 找到数据范围
    const auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());
    const auto min_val = *min_it;
    const auto max_val = *max_it;
    const auto range = max_val - min_val;

    // 采样数据到指定宽度
    std::string result;
    result.reserve(width);

    const auto step = static_cast<double>(data.size()) / width;
    for (int i = 0; i < width; ++i) {
        const auto index = static_cast<std::size_t>(i * step);
        if (index >= data.size()) {
            break;
        }

        const auto value = data[index];
        const auto normalized = (range > 0.0) ? ((value - min_val) / range) : 0.5;
        result.push_back(spark_char(normalized));
    }

    return result;
}

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    const SparklineConfig& config) {
    if (data.empty()) {
        return ftxui::text(std::string(config.width, ' '));
    }

    const auto sparkline = format_sparkline(data, config.width);

    std::ostringstream display;
    display << sparkline;

    if (config.show_latest && !data.empty()) {
        display << " " << std::fixed << std::setprecision(1) << data.back();
    }

    return ftxui::text(display.str()) | ftxui::color(color);
}

ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    int width) {
    SparklineConfig config;
    config.width = width;
    return sparkline_chart(data, color, config);
}

}  // namespace monitor::ui
```

- [ ] **Step 3: 创建 Sparkline 单元测试**

```cpp
// tests/ui/sparkline_chart_test.cpp
#include <catch2/catch_test_macros.hpp>
#include "ui/sparkline_chart.h"
#include <vector>

TEST_CASE("format_sparkline", "[sparkline]") {
    SECTION("Empty data returns empty string") {
        std::vector<double> data;
        auto result = monitor::ui::format_sparkline(data, 10);
        REQUIRE(result.empty());
    }

    SECTION("Single value returns single char") {
        std::vector<double> data = {50.0};
        auto result = monitor::ui::format_sparkline(data, 10);
        REQUIRE(result.size() == 1);
    }

    SECTION("Constant values returns middle spark") {
        std::vector<double> data = {5.0, 5.0, 5.0, 5.0, 5.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE(result.size() == 5);
        // 所有字符应该相同（中间值）
        REQUIRE(result[0] == result[4]);
    }

    SECTION("Increasing trend") {
        std::vector<double> data = {0.0, 25.0, 50.0, 75.0, 100.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE(result.size() == 5);
        // 第一个字符应该低于最后一个
        REQUIRE(result[0] < result[4]);
    }

    SECTION("Width larger than data") {
        std::vector<double> data = {1.0, 2.0, 3.0};
        auto result = monitor::ui::format_sparkline(data, 10);
        // 应该采样到3个字符（数据量限制）
        REQUIRE(result.size() == 3);
    }

    SECTION("Width smaller than data") {
        std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
        auto result = monitor::ui::format_sparkline(data, 5);
        REQUIRE(result.size() == 5);
    }
}
```

- [ ] **Step 4: 更新 CMakeLists.txt 添加新文件**

在 `MONITOR_CORE_SOURCES` 中添加：
```cmake
src/ui/sparkline_chart.cpp  # 新增
```

在 `MONITOR_TEST_SOURCES` 中添加：
```cmake
tests/ui/sparkline_chart_test.cpp  # 新增
```

- [ ] **Step 5: 编译并运行测试**

```bash
cd /home/zazaki/Projects/cpp-linux-monitor-tui
cmake --build build
./build/monitor_tests "[sparkline]"
```

预期输出：所有测试通过

- [ ] **Step 6: 提交**

```bash
git add src/ui/sparkline_chart.h src/ui/sparkline_chart.cpp tests/ui/sparkline_chart_test.cpp CMakeLists.txt
git commit -m "feat: add sparkline chart component for trend visualization"
```

---

## Task 4: 实现进程列表组件

**Files:**
- Create: `src/ui/process_list.h`
- Create: `src/ui/process_list.cpp`
- Create: `tests/ui/process_list_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 创建进程列表头文件**

```cpp
// src/ui/process_list.h
#pragma once

#include <cstddef>
#include <ftxui/dom/elements.hpp>

#include "model/system_snapshot.h"
#include "ui/theme.h"

namespace monitor::ui {

struct ProcessColorScheme {
    ftxui::Color high_cpu = ftxui::Color::RGB(243, 139, 168);    // theme.red
    ftxui::Color medium_cpu = ftxui::Color::RGB(250, 179, 135);  // theme.peach
    ftxui::Color high_memory = ftxui::Color::RGB(203, 166, 247); // theme.mauve
    ftxui::Color medium_memory = ftxui::Color::RGB(137, 180, 250); // theme.blue
    ftxui::Color running = ftxui::Color::RGB(166, 227, 161);     // theme.green
    ftxui::Color sleeping = ftxui::Color::RGB(127, 132, 156);    // theme.overlay1
    ftxui::Color zombie = ftxui::Color::RGB(243, 139, 168);      // theme.red
    ftxui::Color default_text = ftxui::Color::RGB(186, 194, 222); // theme.subtext1
};

ftxui::Element process_list_row(
    const model::ProcessInfo& process,
    bool is_selected,
    const ProcessColorScheme& color_scheme);

ftxui::Element process_list_header();

ftxui::Element process_list(
    std::span<const model::ProcessInfo> processes,
    std::size_t selected_index,
    std::size_t window_start,
    std::size_t window_height,
    const ProcessColorScheme& color_scheme);

std::string format_process_row(
    const model::ProcessInfo& process,
    bool is_selected);

ftxui::Color process_row_color(
    const model::ProcessInfo& process,
    const ProcessColorScheme& color_scheme);

}  // namespace monitor::ui
```

- [ ] **Step 2: 创建进程列表实现文件**

```cpp
// src/ui/process_list.cpp
#include "ui/process_list.h"

#include <iomanip>
#include <sstream>
#include <algorithm>

namespace monitor::ui {

namespace {

constexpr double kHighCpuThreshold = 50.0;
constexpr double kMediumCpuThreshold = 20.0;
constexpr double kHighMemoryThreshold = 10.0;  // 10%
constexpr double kMediumMemoryThreshold = 5.0;  // 5%

}  // namespace

ftxui::Color process_row_color(
    const model::ProcessInfo& process,
    const ProcessColorScheme& color_scheme) {
    // 僵尸进程优先
    if (process.state == 'Z') {
        return color_scheme.zombie;
    }

    // 高 CPU 使用率
    if (process.cpu_percent >= kHighCpuThreshold) {
        return color_scheme.high_cpu;
    }

    // 高内存使用率
    if (process.memory_percent >= kHighMemoryThreshold) {
        return color_scheme.high_memory;
    }

    // 中等 CPU 使用率
    if (process.cpu_percent >= kMediumCpuThreshold) {
        return color_scheme.medium_cpu;
    }

    // 中等内存使用率
    if (process.memory_percent >= kMediumMemoryThreshold) {
        return color_scheme.medium_memory;
    }

    // 运行中进程
    if (process.state == 'R') {
        return color_scheme.running;
    }

    // 默认颜色
    return color_scheme.default_text;
}

std::string format_process_row(
    const model::ProcessInfo& process,
    bool is_selected) {
    std::ostringstream line;

    // 选中标记
    line << (is_selected ? "▸ " : "  ");

    // PID（6字符）
    line << std::setw(6) << process.pid << " ";

    // 状态（2字符）
    line << process.state << " ";

    // CPU%（6字符）
    line << std::setw(6) << std::fixed << std::setprecision(1) << process.cpu_percent << " ";

    // MEM%（6字符）
    line << std::setw(6) << std::fixed << std::setprecision(1) << process.memory_percent << " ";

    // USER（8字符，截断）
    std::string user = process.user;
    if (user.size() > 8) {
        user = user.substr(0, 7) + "…";
    }
    line << std::setw(8) << std::left << user << " ";

    // NAME（15字符，截断）
    std::string name = process.name;
    if (name.size() > 15) {
        name = name.substr(0, 14) + "…";
    }
    line << std::setw(15) << std::left << name;

    return line.str();
}

ftxui::Element process_list_row(
    const model::ProcessInfo& process,
    bool is_selected,
    const ProcessColorScheme& color_scheme) {
    const auto text = format_process_row(process, is_selected);
    const auto color = process_row_color(process, color_scheme);

    auto element = ftxui::text(text) | ftxui::color(color);

    if (is_selected) {
        element = element | ftxui::bgcolor(ftxui::Color::RGB(49, 50, 68)) | ftxui::bold;
    }

    return element;
}

ftxui::Element process_list_header() {
    return ftxui::text("  PID    S   CPU%   MEM%   USER     NAME           ")
           | ftxui::color(ftxui::Color::RGB(137, 180, 250))
           | ftxui::bold;
}

ftxui::Element process_list(
    std::span<const model::ProcessInfo> processes,
    std::size_t selected_index,
    std::size_t window_start,
    std::size_t window_height,
    const ProcessColorScheme& color_scheme) {
    std::vector<ftxui::Element> rows;

    // 表头
    rows.push_back(process_list_header());

    if (processes.empty()) {
        rows.push_back(
            ftxui::text("  no matching processes")
            | ftxui::color(ftxui::Color::RGB(250, 179, 135)));
    } else {
        const auto clamped_selected = std::min(selected_index, processes.size() - 1);
        const auto clamped_start = std::min(window_start, processes.size() - 1);
        const auto end_index = std::min(
            processes.size(),
            clamped_start + std::max<std::size_t>(1, window_height));

        for (std::size_t i = clamped_start; i < end_index; ++i) {
            const auto is_selected = (i == clamped_selected);
            rows.push_back(process_list_row(processes[i], is_selected, color_scheme));
        }
    }

    return ftxui::vbox(rows);
}

}  // namespace monitor::ui
```

- [ ] **Step 3: 创建进程列表单元测试**

```cpp
// tests/ui/process_list_test.cpp
#include <catch2/catch_test_macros.hpp>
#include "ui/process_list.h"

TEST_CASE("format_process_row", "[process_list]") {
    monitor::model::ProcessInfo process;
    process.pid = 1234;
    process.state = 'R';
    process.cpu_percent = 45.5;
    process.memory_percent = 12.3;
    process.user = "root";
    process.name = "chrome";

    SECTION("Selected row has arrow") {
        auto result = monitor::ui::format_process_row(process, true);
        REQUIRE(result.starts_with("▸ "));
    }

    SECTION("Unselected row has spaces") {
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.starts_with("  "));
    }

    SECTION("Contains PID") {
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("1234") != std::string::npos);
    }

    SECTION("Contains process name") {
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("chrome") != std::string::npos);
    }

    SECTION("Long name is truncated") {
        process.name = "very_long_process_name_here";
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("…") != std::string::npos);
    }

    SECTION("Long user is truncated") {
        process.user = "very_long_username";
        auto result = monitor::ui::format_process_row(process, false);
        REQUIRE(result.find("…") != std::string::npos);
    }
}

TEST_CASE("process_row_color", "[process_list]") {
    monitor::ui::ProcessColorScheme scheme;
    monitor::model::ProcessInfo process;

    SECTION("Zombie process gets zombie color") {
        process.state = 'Z';
        process.cpu_percent = 0.0;
        process.memory_percent = 0.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.zombie);
    }

    SECTION("High CPU gets high cpu color") {
        process.state = 'S';
        process.cpu_percent = 60.0;
        process.memory_percent = 1.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.high_cpu);
    }

    SECTION("Running process gets running color") {
        process.state = 'R';
        process.cpu_percent = 5.0;
        process.memory_percent = 1.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.running);
    }

    SECTION("Default process gets default color") {
        process.state = 'S';
        process.cpu_percent = 1.0;
        process.memory_percent = 1.0;
        auto color = monitor::ui::process_row_color(process, scheme);
        REQUIRE(color == scheme.default_text);
    }
}
```

- [ ] **Step 4: 更新 CMakeLists.txt 添加新文件**

在 `MONITOR_CORE_SOURCES` 中添加：
```cmake
src/ui/process_list.cpp  # 新增
```

在 `MONITOR_TEST_SOURCES` 中添加：
```cmake
tests/ui/process_list_test.cpp  # 新增
```

- [ ] **Step 5: 编译并运行测试**

```bash
cd /home/zazaki/Projects/cpp-linux-monitor-tui
cmake --build build
./build/monitor_tests "[process_list]"
```

预期输出：所有测试通过

- [ ] **Step 6: 提交**

```bash
git add src/ui/process_list.h src/ui/process_list.cpp tests/ui/process_list_test.cpp CMakeLists.txt
git commit -m "feat: add process list component with color coding"
```

---

## Task 5: 重构 Dashboard 布局为左右分栏

**Files:**
- Modify: `src/ui/dashboard_view.cpp`
- Modify: `src/ui/dashboard_view.h`

- [ ] **Step 1: 更新 dashboard_view.h 添加新函数声明**

```cpp
// src/ui/dashboard_view.h
#pragma once

#include <string>

#if MONITOR_HAS_FTXUI
#include <ftxui/dom/elements.hpp>
#endif

#include "model/system_snapshot.h"
#include "model/history_data.h"
#include "ui/app_controller.h"

namespace monitor::ui {

#if MONITOR_HAS_FTXUI
ftxui::Element render_dashboard_body_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history);

ftxui::Element render_dashboard_bottom_bar_document(
    const AppController& controller);

ftxui::Element render_dashboard_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history,
    int width,
    int height);
#endif

std::string render_dashboard_to_string(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history,
    int width,
    int height);

}  // namespace monitor::ui
```

- [ ] **Step 2: 重构 dashboard_view.cpp 实现左右分栏布局**

```cpp
// src/ui/dashboard_view.cpp
#include "ui/dashboard_view.h"

#include "collector/process_collector.h"
#include "ui/theme.h"
#include "ui/progress_bar.h"
#include "ui/sparkline_chart.h"
#include "ui/process_list.h"

#include <iomanip>
#include <sstream>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

namespace monitor::ui {

namespace {

std::string format_percent(double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(0) << value << "%";
    return output.str();
}

std::string format_mb(std::uint64_t bytes) {
    const auto mb = bytes / (1024ULL * 1024ULL);
    return std::to_string(mb) + " MB";
}

std::string format_gb(std::uint64_t bytes) {
    const auto gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    std::ostringstream output;
    output << std::fixed << std::setprecision(1) << gb << " GB";
    return output.str();
}

std::string format_speed(std::uint64_t bytes_per_sec) {
    if (bytes_per_sec >= 1024ULL * 1024ULL) {
        const auto mb = static_cast<double>(bytes_per_sec) / (1024.0 * 1024.0);
        std::ostringstream output;
        output << std::fixed << std::setprecision(1) << mb << " MB/s";
        return output.str();
    }
    const auto kb = bytes_per_sec / 1024ULL;
    return std::to_string(kb) + " KB/s";
}

std::string process_sort_title(collector::ProcessSortKey sort_key) {
    switch (sort_key) {
        case collector::ProcessSortKey::Cpu:
            return "Processes [sort: cpu]";
        case collector::ProcessSortKey::Memory:
            return "Processes [sort: memory]";
        case collector::ProcessSortKey::Pid:
            return "Processes [sort: pid]";
        case collector::ProcessSortKey::Name:
            return "Processes [sort: name]";
    }
    return "Processes";
}

ftxui::Element resource_panel(
    const std::string& title,
    double percentage,
    const std::string& summary,
    std::span<const double> history,
    ftxui::Color accent_color,
    bool focused) {
    const auto& theme = catppuccin_mocha();

    ProgressBarConfig bar_config;
    bar_config.width = 20;
    bar_config.show_percentage = true;

    SparklineConfig spark_config;
    spark_config.width = 25;

    auto body = ftxui::vbox({
        progress_bar(percentage, bar_config, theme.green, theme.peach, theme.red),
        sparkline_chart(history, accent_color, spark_config),
        ftxui::text(summary) | ftxui::color(theme.subtext1),
    });

    return themed_window(title, body, focused);
}

ftxui::Element render_left_panel(
    const model::SystemSnapshot& snapshot,
    const model::HistoryData& history,
    const AppController& controller) {
    const auto& theme = catppuccin_mocha();

    const auto cpu_summary = "Total: " + format_percent(snapshot.cpu.total_percent);
    const auto memory_summary =
        format_gb(snapshot.memory.used_bytes) + " / " + format_gb(snapshot.memory.total_bytes);

    auto cpu_panel = resource_panel(
        "CPU",
        snapshot.cpu.total_percent,
        cpu_summary,
        history.cpu_history.data(),
        theme.red,
        controller.focus() == app::FocusZone::Cpu);

    auto memory_percentage = 0.0;
    if (snapshot.memory.total_bytes > 0) {
        memory_percentage = (static_cast<double>(snapshot.memory.used_bytes) /
                             static_cast<double>(snapshot.memory.total_bytes)) * 100.0;
    }

    auto memory_panel = resource_panel(
        "Memory",
        memory_percentage,
        memory_summary,
        history.memory_history.data(),
        theme.green,
        controller.focus() == app::FocusZone::Memory);

    return ftxui::vbox({
        cpu_panel,
        ftxui::separator() | ftxui::color(theme.surface2),
        memory_panel,
    });
}

ftxui::Element render_right_panel(
    const model::SystemSnapshot& snapshot,
    const model::HistoryData& history,
    const AppController& controller) {
    const auto& theme = catppuccin_mocha();

    std::string disk_summary = "n/a";
    double disk_percentage = 0.0;
    if (!snapshot.disks.empty()) {
        const auto& disk = snapshot.disks.front();
        disk_summary = disk.label + " " + format_percent(disk.used_percent);
        disk_percentage = disk.used_percent;
    }

    std::string network_summary = "n/a";
    if (!snapshot.interfaces.empty()) {
        const auto& network = snapshot.interfaces.front();
        network_summary = network.interface_name + " ↓" + format_speed(network.rx_bytes_per_sec)
                         + " ↑" + format_speed(network.tx_bytes_per_sec);
    }

    auto disk_panel = resource_panel(
        "Disk",
        disk_percentage,
        disk_summary,
        history.disk_read_history.data(),
        theme.peach,
        controller.focus() == app::FocusZone::Disk);

    auto network_panel = resource_panel(
        "Network",
        0.0,  // 网络没有百分比
        network_summary,
        history.network_rx_history.data(),
        theme.sapphire,
        controller.focus() == app::FocusZone::Network);

    return ftxui::vbox({
        disk_panel,
        ftxui::separator() | ftxui::color(theme.surface2),
        network_panel,
    });
}

}  // namespace

ftxui::Element render_dashboard_body_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history) {
    const auto& theme = catppuccin_mocha();

    const auto header = ftxui::hbox({
        ftxui::text("host: local") | ftxui::color(theme.subtext1),
        ftxui::separator() | ftxui::color(theme.surface2),
        ftxui::text("refresh " + std::to_string(controller.refresh_interval().count()) + "ms")
            | ftxui::color(theme.subtext1),
        ftxui::separator() | ftxui::color(theme.surface2),
        ftxui::text("tab: proc") | ftxui::color(theme.rosewater) | ftxui::bold,
    }) | ftxui::bgcolor(theme.base);

    auto left_panel = render_left_panel(snapshot, history, controller);
    auto right_panel = render_right_panel(snapshot, history, controller);

    auto resources = ftxui::hbox({
        left_panel | ftxui::flex,
        ftxui::separator() | ftxui::color(theme.surface2),
        right_panel | ftxui::flex,
    });

    auto visible_processes =
        collector::filter_processes(collector::sort_processes(snapshot.processes, controller.sort_key()),
                                    controller.filter_query());

    ProcessColorScheme color_scheme;
    auto process_list_element = process_list(
        visible_processes,
        controller.selected_process_index(),
        controller.process_window_start(),
        controller.process_window_height(),
        color_scheme);

    const auto process_window = themed_window(
        process_sort_title(controller.sort_key()),
        process_list_element,
        controller.focus() == app::FocusZone::Processes);

    ftxui::Element detail_body;
    if (visible_processes.empty()) {
        detail_body = ftxui::vbox({
            ftxui::text("No process selected") | ftxui::color(theme.overlay1),
        });
    } else {
        const auto selected_index = std::min(controller.selected_process_index(), visible_processes.size() - 1);
        const auto& selected = visible_processes[selected_index];
        std::ostringstream memory_line;
        memory_line << std::fixed << std::setprecision(1) << selected.memory_percent;
        detail_body = ftxui::vbox({
            ftxui::text("PID: " + std::to_string(selected.pid)) | ftxui::color(theme.text),
            ftxui::text("Name: " + selected.name) | ftxui::color(theme.text),
            ftxui::text("User: " + selected.user) | ftxui::color(theme.text),
            ftxui::text(std::string{"State: "} + selected.state) | ftxui::color(theme.text),
            ftxui::text("Memory %: " + memory_line.str()) | ftxui::color(theme.text),
            ftxui::text("Nice: " + std::to_string(selected.nice_value)) | ftxui::color(theme.text),
            ftxui::separator() | ftxui::color(theme.surface2),
            ftxui::text("K kill") | ftxui::color(theme.red) | ftxui::bold,
            ftxui::text("R renice") | ftxui::color(theme.peach) | ftxui::bold,
        });
    }
    const auto detail = themed_window("Selected Process", detail_body, false);
    const auto lower = ftxui::hbox({
        process_window | ftxui::flex,
        detail | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32),
    });

    return ftxui::vbox({
               header,
               ftxui::separator() | ftxui::color(theme.surface2),
               resources,
               ftxui::separator() | ftxui::color(theme.surface2),
               lower,
           })
           | ftxui::bgcolor(theme.base);
}

ftxui::Element render_dashboard_bottom_bar_document(const AppController& controller) {
    const auto& theme = catppuccin_mocha();
    if (controller.shared_input_active()) {
        return ftxui::hbox({
            themed_window(
                "Input",
                ftxui::text(controller.command_text()) | ftxui::color(theme.text),
                controller.focus() == app::FocusZone::CommandBar)
                | ftxui::flex,
            themed_window("Status", ftxui::text(controller.status_text()) | ftxui::color(status_color(controller.status_text())), false)
                | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 28),
        }) | ftxui::bgcolor(theme.base);
    }
    return themed_window(
               "Status",
               ftxui::text(controller.status_text()) | ftxui::color(status_color(controller.status_text())),
               controller.focus() == app::FocusZone::CommandBar)
           | ftxui::bgcolor(theme.base);
}

ftxui::Element render_dashboard_document(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history,
    int width,
    int height) {
    auto document =
        ftxui::vbox({render_dashboard_body_document(snapshot, controller, history), render_dashboard_bottom_bar_document(controller)});
    return document | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width)
           | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);
}

std::string render_dashboard_to_string(
    const model::SystemSnapshot& snapshot,
    const AppController& controller,
    const model::HistoryData& history,
    int width,
    int height) {
    auto document = render_dashboard_document(snapshot, controller, history, width, height);
    auto screen = ftxui::Screen(width, height);
    ftxui::Render(screen, document);
    return screen.ToString();
}

}  // namespace monitor::ui
```

- [ ] **Step 3: 更新 application.cpp 集成历史数据**

需要修改 `application.cpp` 以：
1. 添加 `HistoryData` 成员变量
2. 在采样时更新历史数据
3. 将历史数据传递给渲染函数

关键修改点：
```cpp
// 在 Application 类中添加成员
model::HistoryData history_;

// 在 render_once 和 dashboard renderer 中传递 history_
// 在采样线程中更新 history_
```

- [ ] **Step 4: 编译并测试布局**

```bash
cd /home/zazaki/Projects/cpp-linux-monitor-tui
cmake --build build
./build/monitor_tui
```

预期输出：左右分栏布局，进度条和趋势图正常显示

- [ ] **Step 5: 提交**

```bash
git add src/ui/dashboard_view.cpp src/ui/dashboard_view.h src/app/application.cpp
git commit -m "feat: refactor dashboard to left-right panel layout with progress bars and sparklines"
```

---

## Task 6: 性能优化 - 脏标记和缓存

**Files:**
- Create: `src/ui/render_cache.h`
- Create: `src/ui/render_cache.cpp`
- Modify: `src/app/application.cpp`

- [ ] **Step 1: 创建渲染缓存头文件**

```cpp
// src/ui/render_cache.h
#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include "model/system_snapshot.h"
#include "model/history_data.h"
#include "ui/app_controller.h"

namespace monitor::ui {

class RenderCache {
public:
    using Clock = std::chrono::steady_clock;

    void mark_dirty();
    bool is_dirty() const;

    void update_snapshot(const model::SystemSnapshot& snapshot);
    void update_history(const model::HistoryData& history);

    const model::SystemSnapshot& cached_snapshot() const;
    const model::HistoryData& cached_history() const;

    std::string cached_render_string() const;
    void set_cached_render_string(std::string str);

    void clear();

private:
    mutable std::atomic<bool> dirty_{true};
    model::SystemSnapshot snapshot_;
    model::HistoryData history_;
    std::string render_string_;
    Clock::time_point last_update_;
};

}  // namespace monitor::ui
```

- [ ] **Step 2: 创建渲染缓存实现文件**

```cpp
// src/ui/render_cache.cpp
#include "ui/render_cache.h"

namespace monitor::ui {

void RenderCache::mark_dirty() {
    dirty_.store(true, std::memory_order_release);
}

bool RenderCache::is_dirty() const {
    return dirty_.load(std::memory_order_acquire);
}

void RenderCache::update_snapshot(const model::SystemSnapshot& snapshot) {
    snapshot_ = snapshot;
    mark_dirty();
}

void RenderCache::update_history(const model::HistoryData& history) {
    history_ = history;
    mark_dirty();
}

const model::SystemSnapshot& RenderCache::cached_snapshot() const {
    return snapshot_;
}

const model::HistoryData& RenderCache::cached_history() const {
    return history_;
}

std::string RenderCache::cached_render_string() const {
    return render_string_;
}

void RenderCache::set_cached_render_string(std::string str) {
    render_string_ = std::move(str);
    dirty_.store(false, std::memory_order_release);
}

void RenderCache::clear() {
    snapshot_ = {};
    history_ = {};
    render_string_.clear();
    dirty_.store(true, std::memory_order_release);
}

}  // namespace monitor::ui
```

- [ ] **Step 3: 更新 application.cpp 使用缓存**

在 `application.cpp` 中集成 `RenderCache`：
1. 添加 `RenderCache` 成员
2. 在渲染前检查脏标记
3. 只在数据变化时重新渲染

- [ ] **Step 4: 编译并测试性能**

```bash
cd /home/zazaki/Projects/cpp-linux-monitor-tui
cmake --build build
./build/monitor_tui
```

使用 `top` 或 `htop` 监控 `monitor_tui` 的 CPU 使用率，应该有所下降。

- [ ] **Step 5: 提交**

```bash
git add src/ui/render_cache.h src/ui/render_cache.cpp src/app/application.cpp
git commit -m "perf: add render cache with dirty flag to reduce unnecessary redraws"
```

---

## Task 7: 更新测试和文档

**Files:**
- Modify: `tests/ui/dashboard_view_test.cpp`
- Modify: `README.md`

- [ ] **Step 1: 更新 dashboard_view_test.cpp**

更新测试以适配新的函数签名（添加 `history` 参数）。

- [ ] **Step 2: 运行所有测试**

```bash
cd /home/zazaki/Projects/cpp-linux-monitor-tui
cmake --build build
./build/monitor_tests
```

预期输出：所有测试通过

- [ ] **Step 3: 更新 README.md**

更新功能描述和控制说明。

- [ ] **Step 4: 最终提交**

```bash
git add tests/ui/dashboard_view_test.cpp README.md
git commit -m "docs: update tests and documentation for new UI features"
```

---

## 验收检查清单

- [ ] 进度条正确显示 CPU/内存/磁盘使用率
- [ ] 颜色根据阈值变化（绿→橙→红）
- [ ] Sparkline 趋势图显示历史数据
- [ ] 左右分栏布局正确
- [ ] 进程列表颜色编码正确
- [ ] 选中行清晰可见
- [ ] 性能有所提升（CPU 占用降低）
- [ ] 所有测试通过
- [ ] 文档已更新
