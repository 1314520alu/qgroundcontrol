# ESC Config UI Redesign (Layout A)

**Date:** 2026-08-14  
**Status:** Locked — Layout **A** (left/right split) + numbered calibration steps  
**Scope:** [`src/AutoPilotPlugins/APM/APMESCComponent.qml`](../../../src/AutoPilotPlugins/APM/APMESCComponent.qml)  
**Visual lock:** [`assets/esc-config-ui-preview-unirc10-pro.png`](assets/esc-config-ui-preview-unirc10-pro.png)  
**Temporary verification device:** **SIYI UniRC 10 Pro** (10.1″ 1920×1200 landscape, Android 13) — use this for layout/screenshot checks until G20/MK32 verification is resumed  
**Also target (later):** Skydroid G20 / SIYI MK32-class short landscape remotes

## Problem

Current ESC page stacks **Configuration** and **Calibration** in one tall column. On landscape handhelds (Vehicle Setup sidebar + short content height), the calibrate control and procedure steps are clipped or require scrolling. Form chrome is sparse and underuses horizontal space.

## Goals

1. **Layout A only** — left configuration / right calibration; **do not** use stacked B or card-grid C.
2. **One content viewport** — primary controls visible without whole-page scroll on UniRC 10 Pro (and later G20-class short landscape). Steps list may scroll *inside* the right panel only if needed.
3. **Numbered step cards** — calibration procedure shown as compact ①…⑧ steps (not a long unmarked paragraph list; not collapsed-by-default).
4. Keep Fact System semantics (`MOT_*` / `Q_M_*`, `ESC_CALIBRATION` / `Q_ESC_CAL`, DShot extras).

## Non-goals

- PWM range visualizer / gauge diagrams
- Per-parameter mini-cards
- Changing ArduPilot ESC calibration protocol
- PX4 ESC calibration UI (`ESCCalibrationDialog.qml`)
- Replacing Vehicle Setup sidebar

## Locked layout (A)

```text
┌──────────────┬────────────────────────────┬──────────────────────────┐
│ Setup sidebar│ 配置 (~55%)                │ 校准 (~45%)               │
│ (unchanged)  │ 输出类型 + 重启提示 chip     │ 红色警告条                 │
│              │ PWM Min | Max 并排          │ [开始校准] 主按钮          │
│              │ Spin Arm / Min / Max 紧凑行 │ ①…⑧ 编号步骤卡            │
│              │ DShot 两行（协议≥DShot时）  │ （仅步骤区可轻滚动）        │
└──────────────┴────────────────────────────┴──────────────────────────┘
```

### Left — Configuration

| Control | Fact(s) | Notes |
|---------|---------|--------|
| 输出类型 | `MOT_PWM_TYPE` / `Q_M_PWM_TYPE` | Combo; muted chip「需要重启飞行器」 |
| 输出 PWM 最小 / 最大 | `*_PWM_MIN` / `*_PWM_MAX` | Side-by-side compact fields |
| 解锁时旋转 / 最小旋转 / 最大旋转 | `*_SPIN_ARM` / `*_SPIN_MIN` / `*_SPIN_MAX` | Dense labeled rows |
| DShot ESC type / rate | `SERVO_DSHOT_ESC` / `SERVO_DSHOT_RATE` | Visible only when PWM type is DShot |

### Right — Calibration

| Element | Behavior |
|---------|----------|
| Warning banner | Red text: remove props before calibration |
| 开始校准 | Sets `ESC_CALIBRATION` / `Q_ESC_CAL` rawValue = 3 when currently 0; disabled while already 3 |
| Steps ①–⑧ | Numbered short steps matching current procedure (power cycle, battery, tones, safety button, cell beeps, long beep, power up normally). Enable/highlight guidance when calibration active (`rawValue === 3`) |

## Visual lock (strict — match mockup)

**Source of truth:** [`assets/esc-config-ui-preview-unirc10-pro.png`](assets/esc-config-ui-preview-unirc10-pro.png). Implementation must match structure and chrome of this image; do not invent alternate layouts or card-heavy chrome.

| Element | Mock look | QGC mapping |
|---------|-----------|-------------|
| Page background | White content area | `qgcPal.window` |
| Content panels | Soft rounded white cards, light gray border, light fill | `Rectangle` fill `window` / `windowShadeLight` tint; `border.color: groupBorder`; `radius: ScreenTools.defaultBorderRadius` |
| Section titles | Bold「配置」「校准」 | `QGCLabel` bold, default point size |
| Subtitle | Muted「配置并校准电子调速器」 | small font, opacity ~0.55 |
| Output type | Combo + right-side muted pill「需要重启飞行器」 | `FactComboBox` + `SummaryChip` / muted `Rectangle` (not a primary button) |
| PWM Min \| Max | **Two equal columns side-by-side** | `RowLayout` 50/50; stacked label-above-field (not left-label Form) |
| Spin rows | Label + optional (i) + field | `LabelledFactTextField` with `textFieldShowHelp: true` where Fact has help |
| Warning | Light red / pink banner, warning icon, red text | fill ~`warningText` at low opacity or pale red; text `qgcPal.warningText`; icon triangle if available |
| CTA | Full-width **blue**「开始校准」 | `QGCButton` with `primary: true` **or** `backgroundColor: qgcPal.buttonHighlight` + white text so CTA reads as mockup blue (not muted gray primary) |
| Steps | Each step = white rounded row, 1px gray border, **blue circle** with white number + short text | `Repeater` of row `Rectangle`s; badge `color: qgcPal.buttonHighlight` |
| Spacing | Comfortable but dense enough for one viewport | panel padding ~`defaultFontPixelHeight * 0.6`; step gap ~`0.35 * fontHeight` |
| Forbidden | Tall `ValueSlider`, stacked config-above-cal on short landscape, decorative multi-shadow cards, PWM gauge diagram | — |

**Pixel structure checklist (acceptance):**

1. Left panel header + subtitle + type row + PWM pair + 3 spin rows (+ DShot if shown).  
2. Right panel header + warning banner + full-width CTA + 8 numbered step rows.  
3. Split ~55/45; both panels top-aligned in one `RowLayout`.  
4. Screenshot on UniRC 10 Pro must be recognizably the same composition as the lock PNG (sidebar is existing Setup chrome — do not restyle sidebar).  

## Device note — UniRC 10 Pro (temporary)

| Item | Value |
|------|--------|
| Product | SIYI UniRC 10 Pro |
| Screen | 10.1″ IPS, **1920×1200** landscape |
| Role | **Temporary** primary device for design verification and screenshots |
| Content viewport | After Vehicle Setup sidebar, treat as short-wide; still use **split**, not stacked |

Longer-term verification remains Skydroid G20 / SIYI MK15–MK32 class; do not optimize exclusively for 10″ if it breaks ~7″ short landscape.

## Acceptance

- [ ] On UniRC 10 Pro: config + warning + calibrate button visible without whole-page scroll; steps readable as numbered list (inner scroll OK).
- [ ] Later: same on G20-class short landscape (~780×520 content).
- [ ] DShot fields appear only when DShot protocol selected.
- [ ] Calibration still writes Fact `= 3`; no custom parameter storage.
- [ ] Desktop tall windows keep Layout A (may have slack; must not become stacked-only).
