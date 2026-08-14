# Servo Output UI Redesign (Layout C) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite ArduPilot Servo Outputs page as Layout **C** (single-column compressed matrix) so up to 16 channels and all fields fit on short landscape remotes without scrolling, matching the locked C mockup.

**Architecture:** Pure QML restyle of `APMServoComponent.qml`. Keep `FactPanelController` + `ServoOutputMonitorController`. Remove Min/Trim/Max `±` steppers. Fill `availableWidth` × `availableHeight` with a header strip + 16 equal-height rows.

**Tech Stack:** Qt 6 QML, `SetupPage`, `FactTextField` / `FactComboBox` / `FactCheckBoxSlider`, existing `ServoOutputMonitorController`.

## Global Constraints

- **Layout C only** — never A (4×4 cards) or B (dual columns). Visual must match C mockup.
- **No scroll** on G20-class short landscape for full 16-channel set.
- Min/Trim/Max: compact `FactTextField` only — **no** inline `±`.
- Reverse: switch-style (`FactCheckBoxSlider`), not plain checkbox.
- Fact System / vehicle null-checks unchanged.
- Conventional Commits if committing.
- Reference preview: `.superpowers/brainstorm/servo-output-layout-preview.html` option C; spec: `docs/superpowers/specs/2026-08-14-servo-output-ui-redesign-design.md`.

## File map

| File | Role |
|------|------|
| `src/AutoPilotPlugins/APM/APMServoComponent.qml` | Replace table UI with Layout C matrix |
| `docs/superpowers/specs/2026-08-14-servo-output-ui-redesign-design.md` | Locked design (already written) |

No new C++ unless QML cannot bind reverse switch (prefer existing `FactCheckBoxSlider`).

---

### Task 1: Rewrite `APMServoComponent.qml` to Layout C

**Files:**
- Modify: `src/AutoPilotPlugins/APM/APMServoComponent.qml`

- [ ] **Step 1:** Replace `pageComponent` root `Column` + `QGCGroupBox` + 7-column `GridLayout` with `Item` sized to `availableWidth` × `availableHeight`.
- [ ] **Step 2:** Build matrix: header row + `ColumnLayout` of up to 16 row delegates (`Layout.fillHeight: true`), columns `# | 位置 | 功能 | Min | Trim | Max | 反向` with shared column width ratios (match mock C).
- [ ] **Step 3:** Position cell: keep green progress bar + label; wire `servoMonitor` `onServoValueChanged` as today.
- [ ] **Step 4:** Function → `FactComboBox`; Min/Trim/Max → compact `FactTextField` only (delete all ± buttons and repeat Timers).
- [ ] **Step 5:** Reverse → `FactCheckBoxSlider` (or equivalent switch); index chip blue / dim when disabled / no PWM.
- [ ] **Step 6:** Compact paddings/fonts so 16 rows fit short landscape; no horizontal overflow (`clip` + elide on combo).
- [ ] **Step 7:** Incremental `just build` (or build APM QML target) and fix errors.

### Task 2: Verify against C acceptance

- [ ] Confirm no `±` remain in file.
- [ ] Confirm single-column matrix only (no `grid4` / dual `cols2` patterns).
- [ ] Manual: Vehicle Setup → Servo Outputs on short landscape / desktop; all channels visible without scroll when 16 SERVOs exist; edit Min/Trim/Max via field; reverse toggles; PWM bar updates.

---

## Device note (G20)

Skydroid G20 ≈ 7″ 1920×1200 landscape (~960×600 dp). After sidebar, height is the bottleneck. Layout **C** uses equal row heights inside `availableHeight`; do not fall back to A/B if tight — reduce chrome within C.
