# UI 美化与性能优化设计文档

**日期**: 2026-04-30
**版本**: 1.0
**状态**: 已批准

## 1. 概述

### 1.1 目标
对 cpp-linux-monitor-tui 进行全面的 UI 美化和性能优化，提升用户体验和视觉效果。

### 1.2 范围
- UI 布局和视觉效果改进
- 配色和主题增强
- 进程列表显示优化
- 添加新的可视化元素
- 性能优化

### 1.3 优先级
美化优先，性能优化随后进行。

## 2. 设计方案

### 2.1 整体布局：左右分栏的主副布局

#### 布局结构
```
┌─────────────────────────────────────────────────────────────────┐
│ host: local │ refresh 1000ms │ tab: proc                        │
├─────────────────────────────────────┬───────────────────────────┤
│           CPU                       │         Memory            │
│  ■■■■■■■□□□ 45%                    │  ■■■■■■■□□□ 6.2GB/16GB   │
│  ▁▂▃▄▅▆▅▄▃▂▁                      │  ▁▂▃▄▅▆▇█▇▆▅             │
│  Core 0: 32%  Core 1: 58%         │  Swap: 0.5GB/2GB          │
├─────────────────────────────────────┼───────────────────────────┤
│           Disk                      │        Network            │
│  / ■■■■■■■□□□ 65%                  │  eth0 ↓1.2MB/s ↑0.3MB/s  │
│  R: 12MB/s  W: 8MB/s              │  ▁▂▃▄▅▆▅▄▃▂▁             │
├─────────────────────────────────────┴───────────────────────────┤
│ PID   S   CPU   MEM   USER    NAME                             │
│ > 1234 S   45.2  12.3  root    chrome                          │
│   5678 R   23.1   8.5  user    firefox                         │
│   ...                                                          │
├─────────────────────────────────────────────────────────────────┤
│ [Status: ready]                                                │
└─────────────────────────────────────────────────────────────────┘
```

#### 布局规则
- **左侧主面板**（60% 宽度）：CPU 和内存监控
- **右侧副面板**（40% 宽度）：磁盘和网络监控
- **下方面板**：进程列表和状态栏
- **面板间距**：使用 FTXUI 的 separator 分隔

### 2.2 进度条设计

#### 方块进度条
- 使用 `■` 字符显示已用部分
- 使用 `□` 字符显示未用部分
- 根据使用率改变颜色：
  - 0-50%: 绿色 (`theme.green`)
  - 50-80%: 橙色 (`theme.peach`)
  - 80-100%: 红色 (`theme.red`)

#### 进度条长度
- 根据面板宽度动态调整
- 最小长度：10 个字符
- 最大长度：30 个字符

#### 示例
```
CPU  ■■■■■■■□□□ 45%  [绿色]
MEM  ■■■■■■■■□□ 80%  [橙色]
DISK ■■■■■■■■■□ 95%  [红色]
```

### 2.3 Sparkline 趋势图

#### 数据存储
- 使用环形缓冲区存储最近 60 个数据点
- 每个指标独立存储：CPU、内存、磁盘读写、网络收发
- 数据点类型：`double`（百分比或速率）

#### 渲染规则
- 使用 FTXUI 的 `sparkline` 功能
- 趋势图高度：2 行
- 颜色与面板主题色一致
- 显示最近 60 秒的数据（假设 1 秒刷新一次）

#### 示例
```
CPU 使用趋势：
▁▂▃▄▅▆▇█▇▆▅▄▃▂▁▂▃▄▅▆▇█▇▆▅▄▃▂▁
```

### 2.4 进程列表改进

#### 颜色编码
根据进程状态和资源使用率应用不同颜色：

**CPU 使用率**：
- CPU > 50%: 红色 (`theme.red`)
- CPU > 20%: 橙色 (`theme.peach`)
- CPU ≤ 20%: 默认颜色 (`theme.subtext1`)

**内存使用率**：
- 内存 > 1GB: 紫色 (`theme.mauve`)
- 内存 > 500MB: 蓝色 (`theme.blue`)
- 内存 ≤ 500MB: 默认颜色

**进程状态**：
- R (运行中): 绿色 (`theme.green`)
- S (睡眠): 灰色 (`theme.overlay1`)
- Z (僵尸): 红色高亮 (`theme.red`)
- D (不可中断): 黄色 (`theme.peach`)

#### 选中效果
- 选中行：反色显示 + 左侧箭头 `▸`
- 选中行背景：使用 `theme.surface0`
- 选中行文字：使用 `theme.text` + `bold`

#### 列布局
```
PID    S  CPU%  MEM%   USER     NAME            START TIME
1234   R  45.2  12.3   root     chrome          2024-01-15 10:30
5678   S  23.1   8.5   user     firefox         2024-01-15 09:15
```

**列宽规则**：
- PID: 6 字符
- S: 2 字符
- CPU%: 6 字符
- MEM%: 6 字符
- USER: 8 字符
- NAME: 15 字符（截断过长名称）
- START TIME: 16 字符

### 2.5 性能优化策略

#### 渲染优化
1. **脏标记机制**：
   - 只在数据变化时设置脏标记
   - 渲染前检查脏标记，避免不必要的重绘
   - 使用 `std::atomic<bool>` 保证线程安全

2. **缓存计算结果**：
   - 缓存格式化后的字符串
   - 缓存排序和过滤后的进程列表
   - 使用时间戳判断缓存是否过期

3. **减少 FTXUI 元素创建**：
   - 复用不变的 UI 元素
   - 使用 `ftxui::Maybe` 条件渲染
   - 避免在渲染函数中创建临时元素

#### 内存优化
1. **环形缓冲区**：
   - 限制历史数据长度（最近 60 个采样点）
   - 使用固定大小的数组避免动态分配
   - 覆盖旧数据而不是扩展容量

2. **对象池**：
   - 复用 ProcessInfo 对象
   - 避免频繁的 vector 分配
   - 使用 `reserve()` 预分配容量

3. **字符串优化**：
   - 使用 `std::string_view` 避免拷贝
   - 使用 `fmt::format` 或 `std::format` 替代 `ostringstream`
   - 缓存格式化结果

#### 数据采集优化
1. **批量读取**：
   - 一次性读取所有需要的 /proc 文件
   - 减少文件系统调用次数
   - 使用 `mmap()` 优化大文件读取

2. **增量更新**：
   - 只更新变化的数据
   - 使用差值计算网络和磁盘速率
   - 避免全量重新计算

3. **异步采集**：
   - 在独立线程中采集数据
   - 使用双缓冲避免锁竞争
   - 采样间隔可配置

## 3. 技术实现

### 3.1 数据结构扩展

#### 历史数据存储
```cpp
template<typename T, std::size_t N>
class RingBuffer {
public:
    void push(T value);
    std::span<const T> data() const;
    std::size_t size() const;
private:
    std::array<T, N> buffer_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

struct HistoryData {
    RingBuffer<double, 60> cpu_history;
    RingBuffer<double, 60> memory_history;
    RingBuffer<std::uint64_t, 60> network_rx_history;
    RingBuffer<std::uint64_t, 60> network_tx_history;
};
```

#### 进程信息扩展
```cpp
struct ProcessInfo {
    int pid{0};
    char state{'S'};
    double cpu_percent{0.0};
    double memory_percent{0.0};
    int nice_value{0};
    std::string user;
    std::string name;
    std::chrono::system_clock::time_point start_time;  // 新增
    std::string command_line;  // 新增
};
```

### 3.2 UI 组件

#### 进度条组件
```cpp
ftxui::Element progress_bar(
    double value,
    double max_value,
    int width,
    ftxui::Color low_color,
    ftxui::Color medium_color,
    ftxui::Color high_color);
```

#### Sparkline 组件
```cpp
ftxui::Element sparkline_chart(
    std::span<const double> data,
    ftxui::Color color,
    int height = 2);
```

#### 进程列表组件
```cpp
ftxui::Element process_list(
    std::span<const model::ProcessInfo> processes,
    std::size_t selected_index,
    std::size_t window_start,
    std::size_t window_height,
    const ProcessColorScheme& color_scheme);
```

### 3.3 性能监控

#### 渲染性能指标
```cpp
struct RenderMetrics {
    std::chrono::microseconds last_render_time{0};
    std::size_t render_count{0};
    std::size_t skip_count{0};  // 因脏标记跳过的渲染次数
};
```

## 4. 实施阶段

### 阶段 1：基础美化（1-2 天）
**目标**：改进进度条和颜色方案

**任务**：
1. 实现 `progress_bar()` 函数
2. 为 CPU/内存/磁盘面板添加进度条
3. 优化颜色方案和对比度
4. 改进进程列表的选中效果

**验收标准**：
- 进度条正确显示使用率
- 颜色根据阈值变化
- 选中行清晰可见

### 阶段 2：布局重构（2-3 天）
**目标**：实现左右分栏布局和趋势图

**任务**：
1. 重构 `render_dashboard_body_document()` 实现左右分栏
2. 实现 `sparkline_chart()` 函数
3. 添加历史数据存储
4. 优化面板间距和对齐

**验收标准**：
- 左右分栏布局正确
- 趋势图显示历史数据
- 面板间距均匀

### 阶段 3：性能优化（1-2 天）
**目标**：实现渲染和内存优化

**任务**：
1. 实现脏标记机制
2. 添加缓存层
3. 实现环形缓冲区
4. 优化数据采集

**验收标准**：
- CPU 占用降低 30%
- 内存使用稳定
- 渲染流畅无卡顿

### 阶段 4：高级功能（2-3 天）
**目标**：完善进程列表和交互

**任务**：
1. 实现进程颜色编码
2. 扩展进程信息列
3. 优化列对齐和截断
4. 添加键盘快捷键提示

**验收标准**：
- 进程颜色正确编码
- 列对齐整齐
- 信息完整可读

## 5. 测试策略

### 5.1 单元测试
- 进度条渲染测试
- Sparkline 数据处理测试
- 颜色编码逻辑测试
- 环形缓冲区测试

### 5.2 集成测试
- 布局渲染测试
- 性能基准测试
- 内存泄漏测试

### 5.3 手动测试
- 不同终端尺寸测试
- 长时间运行稳定性测试
- 交互响应测试

## 6. 风险和缓解

### 6.1 FTXUI 限制
**风险**：FTXUI 可能不支持某些可视化效果
**缓解**：使用 FTXUI 原生功能，必要时自定义渲染

### 6.2 性能回归
**风险**：新功能可能导致性能下降
**缓解**：实施性能监控，设置性能基准

### 6.3 终端兼容性
**风险**：某些终端可能不支持 Unicode 字符
**缓解**：提供 ASCII 回退方案

## 7. 附录

### 7.1 参考资料
- FTXUI 文档：https://github.com/ArthurSonzogni/FTXUI
- Catppuccin 配色：https://github.com/catppuccin/catppuccin

### 7.2 相关文件
- `src/ui/dashboard_view.cpp`：主要渲染逻辑
- `src/ui/theme.h`：主题和颜色定义
- `src/model/system_snapshot.h`：数据模型
- `src/app/application.cpp`：应用主循环
