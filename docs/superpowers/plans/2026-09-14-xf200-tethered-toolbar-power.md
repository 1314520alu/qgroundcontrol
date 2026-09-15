# XF200 Tethered Toolbar Power Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** On ZY-XF200 Tethered, show five toolbar power slots (PSU 1–4 voltage+power, bus 5 voltage) from existing `BatteryFact` ids, with spec-based icon states and per-slot drawers.

**Architecture:** Keep MAVLink `BATTERY_STATUS` ingestion unchanged. Extract voltage/`chargeState` → visual kind into a QML singleton used by `BatteryIndicator.qml`. Default aircraft models keep the current Repeater.

**Tech Stack:** Qt 6 QML, QGroundControl Fact system, existing `QGCColoredImage`, CTest `UnitTest`.

## Global Constraints

- Landscape remotes first (Skydroid G20 / SIYI MK32); compact two-line chips, `ScreenTools` sizing, `QGCPalette` colors.
- No `Q_ASSERT`; null-check `activeVehicle`.
- Do not parse vendor CAN or DroneCAN in QGC.
- Do not commit unless the user asks.
- `aircraftModel` ZY-XF200 Tethered is `rawValue == 3`.

---

### Task 1: Visual kind helper (TDD)

**Files:**
- Create: `src/QmlControls/Xf200TetheredPowerVisual.h`
- Create: `src/QmlControls/Xf200TetheredPowerVisual.cc`
- Create: `test/QmlControls/Xf200TetheredPowerVisualTest.h`
- Create: `test/QmlControls/Xf200TetheredPowerVisualTest.cc`
- Modify: `src/QmlControls/CMakeLists.txt` (add `.cc/.h` to `target_sources`)
- Modify: `test/QmlControls/CMakeLists.txt` (sources + `add_qgc_test`)

**Interfaces:**
- Consumes: `MAV_BATTERY_CHARGE_STATE_*` from `MAVLinkLib.h`
- Produces: `Xf200TetheredPowerVisual::Kind kind(double voltage, int chargeState)`, `batterySvg(Kind)`, `psuSvg(Kind)`, `kAircraftModelZyXf200Tethered = 3`

- [ ] **Step 1: Write the failing test**

```cpp
QCOMPARE(Xf200TetheredPowerVisual::kind(120.0, MAV_BATTERY_CHARGE_STATE_OK),
         Xf200TetheredPowerVisual::Normal);
QCOMPARE(Xf200TetheredPowerVisual::kind(117.0, MAV_BATTERY_CHARGE_STATE_UNDEFINED),
         Xf200TetheredPowerVisual::Warn);
QCOMPARE(Xf200TetheredPowerVisual::kind(114.0, MAV_BATTERY_CHARGE_STATE_OK),
         Xf200TetheredPowerVisual::Critical);
QCOMPARE(Xf200TetheredPowerVisual::kind(qQNaN(), MAV_BATTERY_CHARGE_STATE_UNDEFINED),
         Xf200TetheredPowerVisual::Empty);
QCOMPARE(Xf200TetheredPowerVisual::kind(120.0, MAV_BATTERY_CHARGE_STATE_FAILED),
         Xf200TetheredPowerVisual::Emergency);
QCOMPARE(Xf200TetheredPowerVisual::kind(120.0, MAV_BATTERY_CHARGE_STATE_LOW),
         Xf200TetheredPowerVisual::Low);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest -R Xf200TetheredPowerVisualTest --output-on-failure`  
Expected: FAIL (type not linked / kind not defined)

- [ ] **Step 3: Implement `Xf200TetheredPowerVisual`**

Charge-state override then voltage bands from the spec. `psuSvg` always `/qmlimages/PowerSupply.svg`. Battery SVG per kind as in the spec table.

- [ ] **Step 4: Run tests and make sure they pass**

Run: `ctest -R Xf200TetheredPowerVisualTest --output-on-failure`  
Expected: PASS

- [ ] **Step 5: Commit** — skip unless the user asks.

---

### Task 2: PSU SVG + BatteryIndicator XF200 UI

**Files:**
- Create: `src/Toolbar/Images/PowerSupply.svg`
- Modify: `src/Toolbar/CMakeLists.txt` (register SVG)
- Modify: `src/Toolbar/BatteryIndicator.qml`
- Modify: `src/Settings/App.SettingsGroup.json` (`aircraftModel` longDesc)
- Modify: `translations/qgc_source_zh_CN.ts` (`Power %1` → 电源 %1, `Bus` → 母线)

**Interfaces:**
- Consumes: `Xf200TetheredPowerVisual.kind/batterySvg/psuSvg`, `appSettings.aircraftModel`, `Vehicle.batteries`
- Produces: Five compact chips when model is 3; per-slot drawer via `pageProperties.slotId`

- [ ] **Step 1: Add white-tintable `PowerSupply.svg` and CMake resource**
- [ ] **Step 2: Branch `BatteryIndicator.qml`**
  - `_xf200Tethered` from aircraftModel === 3
  - `showIndicator` true for XF200 whenever `_activeVehicle` is set
  - Fixed Repeater model `[1,2,3,4,5]`
  - Lookup battery by `id.rawValue`
  - Slots 1–4: PSU icon + label + voltage + power; slot 5: battery SVG + label + voltage
  - Per-chip MouseArea sets `slotId` and opens drawer
  - Default-mode MouseArea / Repeater hidden in XF200 mode
  - XF200 drawer: one slot’s state/V/I/P/T; expand hides Battery Display group
- [ ] **Step 3: zh_CN + longDesc**
- [ ] **Step 4: `just build` (or existing debug tree) and `ctest -R Xf200TetheredPowerVisualTest`**
- [ ] **Step 5: Commit** — skip unless the user asks.
