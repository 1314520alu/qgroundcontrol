# Vehicle Setup Menu Visibility — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist which Vehicle Setup sidebar items are visible (remote-first defaults), with Summary always first and a fixed sort order.

**Architecture:** `AppSettings.vehicleSetupVisibleComponents` string Fact + helpers; General settings embeds `VehicleSetupMenuVisibilitySettings.qml`; `VehicleConfigView.qml` filters/sorts by stable IDs.

**Tech Stack:** Qt 6, Fact System, SettingsUI `component` embed, QML

## Global Constraints

- Default visible IDs: `frame,sensors,radio,flightModes,power,esc,escTelemetry,motors,flightSafety,failsafes`
- Summary / Parameters / Firmware / Optical Flow not checkbox-controlled
- Locale-independent ID resolution (QML paths / type needles), not translated names
- Landscape remotes first; compact two-column checkboxes
- Conventional Commits if committing (only when user asks)

---

### Task 1: Fact + AppSettings helpers

**Files:**
- Modify: `src/Settings/App.SettingsGroup.json`
- Modify: `src/Settings/AppSettings.h`, `src/Settings/AppSettings.cc`
- Test: `test/Settings/AppSettingsTest.cc`, `test/Settings/AppSettingsTest.h`

- [ ] Add string Fact `vehicleSetupVisibleComponents` with default above
- [ ] Add helpers: catalog, visible list, is/set/reset, resolveId, sortKey
- [ ] Unit tests for parse/toggle/reset/unknown

### Task 2: Settings UI

**Files:**
- Modify: `src/AppSettings/pages/General.SettingsUI.json`
- Create: `src/AppSettings/VehicleSetupMenuVisibilitySettings.qml`
- Modify: `src/AppSettings/CMakeLists.txt`

- [ ] Embed component group in General
- [ ] Two-column checkboxes + Restore defaults
- [ ] Register QML in module

### Task 3: Sidebar filter + sort

**Files:**
- Modify: `src/Vehicle/VehicleSetup/VehicleConfigView.qml`

- [ ] Build ordered visible index list from Fact + resolveId + sortKey
- [ ] Repeater uses ordered indices; keep original component index for navigation
- [ ] Selection recovery → Summary when current item hidden
- [ ] React to Fact changes

### Task 4: Verify

- [ ] `just build` (or incremental) and `ctest -R AppSettingsTest`
