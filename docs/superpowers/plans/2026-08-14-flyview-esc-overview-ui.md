# 飞行页电调状态概览 · 实现计划

> **For agentic workers:** 按任务逐步实现；步骤用 checkbox 跟踪。

**Goal:** 按 `docs/superpowers/specs/2026-08-14-flyview-esc-overview-ui-design.md` 重做飞行页 ESC 指示器浮层：宽半透明面板、中文、1–4/5–8/9–16 分档、错误着色、列宽节约。

**Architecture:** 只改 `EscIndicatorPage.qml`（主）+ `EscIndicator.qml`（工具栏文案与健康判断）。复用配置页电调遥测的 Fact 绑定与健康/故障文案逻辑，不抽共享组件。

**Tech Stack:** QML / ToolIndicatorPage / vehicle.escs Facts

## Global Constraints

- 中文 `qsTr` 源文案；半透明面板（地图可透出）
- `n = min(count, 16)`；默认尽量不滚动
- 列宽：无数据列窄、有数据列宽
- 不改配置页、不抽共享 QML、不改 MAVLink

---

### Task 1: 重写 EscIndicatorPage

**Files:**
- Modify: `src/Toolbar/EscIndicatorPage.qml`

- [x] 实现宽面板 + 三档布局 + 中文 + 半透明 + 错误色 + 窄列宽
- [x] 用 `_bitOnline(esc)`（`id % 4`）替代整表共用 `get(0).info`

### Task 2: 修正 EscIndicator 工具栏

**Files:**
- Modify: `src/Toolbar/EscIndicator.qml`

- [x] `OK`/`ERR` → `正常`/`异常`
- [x] 健康判断遍历全部 `_escs`（含 errorCount），不再只查前 4 路

### Task 3: 构建安装验证

- [x] `cmake --build build/Android-debug -j4` + `./run-qgc.sh --android`
