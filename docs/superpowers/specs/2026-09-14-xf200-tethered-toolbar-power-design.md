# XF200 Tethered Toolbar Power Icons

**Date:** 2026-09-14  
**Status:** Approved  
**Plan:** `docs/superpowers/plans/2026-09-14-xf200-tethered-toolbar-power.md`

## Problem

ZY-XF200 系留用四台 CX1500-28K-120 机载电源（1500 Vdc 隔离转为 120 Vdc，合计 28 kW）加一路母线电压模块。电源原始 CAN 经机上中转变成 DroneCAN `BatteryInfo` / `BatteryInfoAux`，飞控再发 MAVLink `BATTERY_STATUS`。QGC 顶栏仍按普通锂电池画一块电池，无法区分电源与母线，也无法同时看到电压和功率。

## Goals

1. When `appSettings.aircraftModel` is **ZY-XF200 Tethered** (`rawValue == 3`), replace the compact battery indicator with five fixed slots.
2. Slots 1–4: PSU icon, index label, output voltage + instant power.
3. Slot 5: existing battery icon, index label, bus voltage only.
4. Always show five slots; missing `BATTERY_STATUS` for that `battery_id` shows `—`.
5. Icon color/SVG follows MAVLink `chargeState` first, then CX1500-28K-120 voltage bands (not SOC %).
6. Tap a slot to open a drawer for **that slot only**.
7. Other aircraft models keep the current battery indicator.

## Non-goals

- Parsing the PSU vendor CAN in QGC
- DroneCAN stack in QGC
- Changing MAVLink / `BatteryFact` ingestion
- XF100 / XF100 tethered / Default toolbar behavior
- Showing `battery_id` other than 1–5 in the compact bar
- Auto-detecting airframe from the vehicle

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Activation | `appSettings.aircraftModel.rawValue === 3` (ZY-XF200 Tethered). No restart. |
| Implementation | Branch inside `BatteryIndicator.qml` |
| Data path | Vendor CAN → bridge → DroneCAN BatteryInfo/Aux → FC → `BATTERY_STATUS` → `BatteryFact` |
| IDs | UI slots 1–4 PSU, 5 bus. MAVLink `BATTERY_STATUS.id` is 0-based, so slot N reads id `N-1` (bus = 4). |
| Missing telemetry | Five placeholders, `—` |
| Compact labels | PSU 1–4 drawn inside the PSU icon; slot 5 has no external index |
| PSU values | Voltage (`N.N V`) and `instantPower` (existing V×I; one decimal, `kW` if ≥ 1000 W) |
| Slot 5 values | Voltage only |
| Consolidate / % display | Ignored in this mode |
| Tap | Per-slot drawer |
| Extra battery ids | Ignored in the compact bar |
| Empty vehicle | Hide indicator (no `activeVehicle`), same as today |
| Connected, no batteries yet | Show five `—` slots |

## Visual states

This is a regulated 120 V bus, not a discharging LiPo. Do not use Battery Display SOC thresholds (80% / 60%).

`chargeState` wins:

| `MAV_BATTERY_CHARGE_STATE` | Kind | Slot 5 SVG | PSU |
| --- | --- | --- | --- |
| `LOW` | Low | `BatteryOrange.svg` | PSU SVG, orange |
| `CRITICAL` | Critical | `BatteryCritical.svg` | PSU SVG, red |
| `EMERGENCY` / `FAILED` / `UNHEALTHY` | Emergency | `BatteryEMERGENCY.svg` | PSU SVG, red |
| other (including `OK`, `UNDEFINED`, `CHARGING`) | fall through to voltage | | |

Voltage (CX1500-28K-120: rated 120 V, 119–121 V; UV 115±2 V; OV 130±2 V):

| Voltage V | Kind | Slot 5 SVG | PSU |
| --- | --- | --- | --- |
| NaN / no fact | Empty | `Battery.svg`, text color | PSU SVG, text color |
| 119 ≤ V ≤ 121 | Normal | `BatteryGreen.svg` | green |
| 115 ≤ V < 119 | Warn | `BatteryYellow.svg` | yellow |
| 121 < V ≤ 130 | Warn | `BatteryYellow.svg` | yellow |
| V < 115 or V > 130 | Critical | `BatteryCritical.svg` | red |

## Drawer

- Title: `Power %1` for slots 1–4, `Bus` for slot 5.
- Fields: state, voltage, current, power, temperature. Missing → `—`. No remaining-%.
- Expand: hide “Battery Display” (consolidate / value / SOC coloring). Keep firmware low-battery failsafe loader and Vehicle Power configure if present.

## Architecture

```text
AppSettings.aircraftModel == 3
        │
        ▼
BatteryIndicator.qml
  ├─ default: existing batteries Repeater
  └─ XF200: slots 1–5
        │
        ├─ Xf200TetheredPowerVisual.kind(voltage, chargeState)
        ├─ BatteryFact by id (or null)
        └─ per-slot ToolIndicatorPage
```

## Testing

1. Unit test `Xf200TetheredPowerVisual::kind` / SVG paths (120 V green, 117 V yellow, 114 V red, 131 V red, NaN empty, FAILED emergency, LOW orange).
2. Manual: short landscape (G20 / MK32); five chips stay in the toolbar; switch model away and back.

## Files (expected)

| File | Change |
| --- | --- |
| `src/QmlControls/Xf200TetheredPowerVisual.h/.cc` | Kind mapping, QML singleton |
| `test/QmlControls/Xf200TetheredPowerVisualTest.*` | Unit tests |
| `src/Toolbar/BatteryIndicator.qml` | XF200 compact row + per-slot drawer |
| `src/Toolbar/Images/PowerSupply.svg` | PSU glyph (white, tinted) |
| `src/Toolbar/CMakeLists.txt` | Register SVG |
| `src/Settings/App.SettingsGroup.json` | `aircraftModel` longDesc now has a consumer |
| `translations/qgc_source_zh_CN.ts` | `Power %1`, `Bus` |
