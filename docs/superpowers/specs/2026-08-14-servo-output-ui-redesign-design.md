# Servo Output UI Redesign (Layout C)

**Date:** 2026-08-14  
**Status:** Locked — Layout **C only**  
**Scope:** [`src/AutoPilotPlugins/APM/APMServoComponent.qml`](../../../src/AutoPilotPlugins/APM/APMServoComponent.qml)  
**Visual lock:** `.superpowers/brainstorm/servo-output-layout-preview.html` → option **C · 单列压缩表**  
**Target devices:** Landscape remotes (Skydroid G20 / SIYI), short content viewport ≈ 780×520 after Vehicle Setup sidebar

## Problem

Current page is a 7-column `GridLayout` with per-cell `− / FactTextField / +` steppers for Min/Trim/Max. On short landscape remotes, columns and rows overflow → horizontal and vertical scrolling. User cannot see all 16 outputs at once.

## Goals

1. **Layout C only** — single-column compressed matrix; **do not** use 4×4 cards (A) or dual-column rows (B).
2. **All data on one screen** — up to 16 servo rows + header + all columns visible; **no vertical or horizontal scroll** on G20-class short landscape.
3. **Visual parity with C mockup** — white matrix panel, header strip, equal-height rows, alternating row tint, blue index chip, green position bar, compact blue-border fields, pill reverse switch.
4. **Compact edit** — Min / Trim / Max are `FactTextField` only (tap → keyboard); **no** inline `±` buttons or hold-repeat timers.
5. Keep live PWM via existing `ServoOutputMonitorController`.

## Non-goals

- Card grid or two-column layouts
- Shared bottom editor / master-detail
- Changing SERVO_* parameter semantics or monitor C++ API
- PX4 actuator UI
- Replacing Vehicle Setup sidebar

## Locked layout (C)

```text
┌─────────────────────────────────────────────────────────────┐
│ 舵机输出                                                      │
│ Configure… (optional; hide on short screen via SetupPage)   │
│ ┌─────┬────────┬──────────┬─────┬──────┬─────┬──────┐      │
│ │  #  │ 位置   │ 功能     │ Min │ Trim │ Max │ 反向 │      │
│ ├─────┼────────┼──────────┼─────┼──────┼─────┼──────┤      │
│ │  1  │ [bar]  │ Function │ ### │ #### │ ### │  ⬭   │ ×16  │
│ │ ... equal row height filling remaining viewport ...      │
│ └─────┴────────┴──────────┴─────┴──────┴─────┴──────┘      │
└─────────────────────────────────────────────────────────────┘
```

### Columns (left → right, fixed order)

| Col | Content | Control |
|-----|---------|---------|
| # | 1…16 | Blue chip; dim gray when function is Disabled / no live PWM |
| 位置 | Live PWM | Track + green fill + centered value (or `-`) |
| 功能 | `SERVOn_FUNCTION` | `FactComboBox` (`indexModel: false`), elide, fill width |
| Min | `SERVOn_MIN` | Compact `FactTextField`, no units, no ± |
| Trim | `SERVOn_TRIM` | same |
| Max | `SERVOn_MAX` | same |
| 反向 | `SERVOn_REVERSED` | Switch-style (`FactCheckBoxSlider` / `QGCCheckBoxSlider`), not a plain checkbox |

### Visual tokens (match mock C)

- Panel: white / `windowShade` fill, 1px border, small corner radius  
- Header: light gray strip, muted small labels, centered  
- Rows: equal `Layout.fillHeight`; even rows slight tint  
- Index chip: primary blue; disabled → mid gray  
- Position bar: gray track, green progress, bold centered µs  
- Fields: light blue border, centered numeric text  
- No group-box chrome that steals vertical space beyond title + matrix  

## Behavior

- Root page size = `availableWidth` × `availableHeight` so `SetupPage` `QGCFlickable` stays non-interactive on target remotes.
- Show only channels where `SERVO{n}_FUNCTION` exists (`servoExists`); still target fitting **16** when present.
- Position bar ratio uses Min/Max facts and `servoMonitor.servoValue(index)` (same math as today).
- Parameter writes remain Fact System only.

## Out of scope for polish

- Do not “improve” into cards or split columns if space feels tight — compress row chrome / font / paddings within **C** instead.

## Acceptance

- [ ] On short landscape (simulate ~780×520 content): 16 rows + header + all 7 columns visible without scroll.
- [ ] No `±` steppers on Min/Trim/Max.
- [ ] Reverse is a switch; position bar + function + fields match mock C structure.
- [ ] Live PWM updates still work.
- [ ] Desktop tall windows: still Layout C (may have extra vertical slack in rows; must not become A/B).
