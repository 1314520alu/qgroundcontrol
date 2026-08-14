# ESC Telemetry UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Vehicle Setup page **ESC Telemetry** with adaptive cards/table UI bound to `vehicle.escs` (MAVLink `ESC_INFO` / `ESC_STATUS`), matching the three visual locks.

**Architecture:** `APMESCTelemetryComponent` + QML `SetupPage`. Register in `APMAutoPilotPlugin` after ESC. Menu id `escTelemetry` in AppSettings catalog/sort/default. Pure QML layout; no new Fact storage.

**Tech Stack:** Qt 6 QML/C++, `SetupPage`, `SummaryChip`, `SummaryStatusPill`, existing `EscStatusFactGroup`.

## Global Constraints

- Adaptive: ≤4 cards · 5–8 compact table · 9–16 dense table; max 16.
- Read-only; ESC config page unchanged.
- Null-check vehicle; temperature 32767 → —; display °C as raw/100.
- Visual locks under `docs/superpowers/specs/assets/esc-telemetry-ui-preview-*-unirc10-pro.png`.
- Needle `ESCTelemetry` **before** `ESC` in resolve table.
- Conventional Commits if committing.

## File map

| File | Role |
|------|------|
| `APMESCTelemetryComponent.h/.cc` | VehicleComponent |
| `APMESCTelemetryComponent.qml` | Adaptive page UI |
| `APMESCTelemetryComponentSummary.qml` | Summary chips |
| `APMAutoPilotPlugin.*` | Register after ESC |
| `APM/CMakeLists.txt` | Sources + QML |
| `AppSettings.cc` / `.h` / `App.SettingsGroup.json` | Menu id + default |
| `AppSettingsTest.cc` | Resolve + default |
| `VehicleSummary.qml` | Optional summary order |
| Spec/plan under `docs/superpowers/` | Design lock |

### Task 1: Menu id + component skeleton

- [x] Add `escTelemetry` to catalog, needles (before ESC), sort (after esc), default visible list + JSON default
- [x] Add `APMESCTelemetryComponent` + CMake + plugin registration
- [x] Update `AppSettingsTest` for resolve/default

### Task 2: QML page + summary

- [x] Implement adaptive UI + empty state + failure chips
- [x] Summary chips; VehicleSummary order entry

### Task 3: Verify

- [x] `just build` (or incremental)
- [ ] Manual: MockLink ESC → 4 cards; mentally verify 8/16 branches
