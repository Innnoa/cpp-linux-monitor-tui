# Catppuccin Mocha Theme Design

## Goal

为当前 `FTXUI` 终端监控界面接入一套固定的 `Catppuccin Mocha` 配色，让主面板、输入栏、状态栏、焦点标题和选中行都有一致且可见的主题样式，同时不改动现有布局和按键语义。

## Scope

- 使用固定 `Mocha` flavor，不做运行时或编译期切换。
- 覆盖主界面资源框、进程面板、详情面板、底部状态栏、共享输入栏。
- 保留当前交互：`h/l` 焦点、`j/k` 选中、`:` 和 `/` 输入、`K/R` 动作。

## Non-goals

- 不新增主题设置界面。
- 不调整现有布局比例和信息结构。
- 不修改现有命令、过滤、进程操作语义。

## Approach

新增一个共享的 `ui/theme.h`，集中定义 `Catppuccin Mocha` 颜色常量和少量样式辅助函数。`dashboard_view.cpp` 和 `application.cpp` 统一从这里取色，保证主界面和输入栏视觉一致。

界面样式采用以下映射：

- 全局背景：`base` / `mantle`
- 面板边框：普通 `surface1`，焦点 `blue`
- 标题文字：普通 `text`，焦点 `blue`
- 普通正文：`text` / `subtext1`
- 帮助和次级信息：`overlay1`
- 选中进程行：`surface0` 背景 + `text` 前景
- `ready` / 成功状态：`green`
- 输入提示与模式说明：`sapphire`
- 警告与危险动作：`peach` / `red`

## Testing

- 渲染级像素测试：确认焦点标题和选中行真的带上目标前景/背景色。
- 现有文本渲染测试继续保留，避免配色改动破坏文案和布局。
- 全量 `ctest` 回归。
