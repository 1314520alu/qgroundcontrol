# Motor Test UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the sparse Motor Setup page with a split-layout UI: ArduPilot-aligned top-down diagram (auto topology, coaxial, selection highlight, spin feedback) and safe single-motor debug controls (default 10%, max 30%).

**Architecture:** Pure `MotorLayoutBuilder` (C++) owns letter→board-motor mapping and angles from ArduPilot diagrams. `MotorComponentController` exposes a `QVariantList` to QML. `MotorComponent.qml` + `MotorTestDiagram.qml` implement the split UI and call `Vehicle::motorTest` with board motor indices and clamped throttle.

**Tech Stack:** Qt 6 QML/C++, existing `SetupPage` / `ValueSlider` / `FactPanelController`, QGC unit tests (`UT_REGISTER_TEST`).

## Global Constraints

- Throttle default **10%**, hard max **30%**; clamp every `motorTest` percent.
- Auto-stop timeout remains **3** seconds.
- No frame picker on the page; layout from vehicle `motorCount` + APM `FRAME_CLASS`/`FRAME_TYPE` when present.
- Letters A… match Mission Planner diagram positions; `motorTest` uses **board** motor numbers (`MOTOR_TEST_ORDER_BOARD` already in `Vehicle::motorTest`).
- Max **16** motors; unknown count → grid fallback + warning.
- Null-check `Vehicle*` before use.
- Conventional Commits; no unrelated refactors.

## File map

| File | Role |
|------|------|
| `src/AutoPilotPlugins/Common/MotorLayoutBuilder.h/.cc` | Frame tables → motor entries |
| `src/AutoPilotPlugins/Common/MotorComponentController.h/.cc` | QML controller |
| `src/AutoPilotPlugins/Common/MotorTestDiagram.qml` | Left diagram |
| `src/AutoPilotPlugins/Common/MotorComponent.qml` | Split page UI |
| `src/AutoPilotPlugins/Common/CMakeLists.txt` | Sources + QML_FILES |
| `test/AutoPilotPlugins/MotorLayoutBuilderTest.*` | Unit tests |
| `test/AutoPilotPlugins/CMakeLists.txt` | Register test |

### Task 1: MotorLayoutBuilder + unit tests

**Files:**
- Create: `src/AutoPilotPlugins/Common/MotorLayoutBuilder.h`
- Create: `src/AutoPilotPlugins/Common/MotorLayoutBuilder.cc`
- Create: `test/AutoPilotPlugins/MotorLayoutBuilderTest.h`
- Create: `test/AutoPilotPlugins/MotorLayoutBuilderTest.cc`
- Modify: `src/AutoPilotPlugins/Common/CMakeLists.txt`
- Modify: `test/AutoPilotPlugins/CMakeLists.txt`

**Produces:** `MotorLayoutBuilder::build(frameClass, frameType, motorCount)` → `QList<MotorLayoutEntry>` with `letter`, `motorIndex` (1-based board), `angleDeg` (0=nose, CW+), `dir` (CW/CCW/Unknown), `layer` (Single/Top/Bottom).

- [x] **Step 1:** Add builder API + Quad X / + / Hexa X / Octo X / OctoQuad X / Y6B tables (ArduPilot letter↔number map)
- [x] **Step 2:** Unit tests asserting A→motor and dirs for those frames
- [x] **Step 3:** Wire CMake; run `ctest -R MotorLayoutBuilderTest`

### Task 2: MotorComponentController

**Files:**
- Create: `src/AutoPilotPlugins/Common/MotorComponentController.h/.cc`
- Modify: `src/AutoPilotPlugins/Common/CMakeLists.txt`

**Produces:** QML_ELEMENT with `motors`, `topologyName`, `spatialLayout`, `vehicleName`, `motorCountDisplay`; rebuilds from vehicle params.

- [x] **Step 1:** Implement controller reading `FRAME_CLASS`/`FRAME_TYPE` when present
- [x] **Step 2:** Expose helpers `clampThrottle(int)`, constants `defaultThrottle=10`, `maxThrottle=30`

### Task 3: MotorTestDiagram.qml + MotorComponent.qml redesign

**Files:**
- Create: `src/AutoPilotPlugins/Common/MotorTestDiagram.qml`
- Modify: `src/AutoPilotPlugins/Common/MotorComponent.qml`
- Modify: `src/AutoPilotPlugins/Common/CMakeLists.txt` (QML_FILES)

- [x] **Step 1:** Diagram: light frame, CW green / CCW cyan, letters, coaxial offset, selection highlight, spin via Rotation
- [x] **Step 2:** Page: split layout, safety, throttle 0–30, single-motor card, All/Stop secondary, status
- [x] **Step 3:** `just build` (or incremental) and fix errors

### Task 4: Verify

- [x] Run `ctest -R MotorLayoutBuilderTest --output-on-failure`
- [ ] `just lint` on touched paths if feasible
- [ ] Manual UI on Skydroid G20 (7" 1920×1200 landscape): split diagram|controls both visible

### Device note (G20)

Skydroid G20 ≈ 7″ 1920×1200 landscape (~960×600 dp). After Vehicle Setup sidebar, content is short-wide; layout must prefer **split** (not stacked) and compact `QGCSlider` so controls are not pushed off-screen.

## Spec coverage

| Spec item | Task |
|-----------|------|
| Split layout | 3 |
| Auto topology / no picker | 1–2 |
| Coaxial top/bottom | 1, 3 |
| A start FR / + | 1 |
| CW/CCW colors | 3 |
| Selection highlight | 3 |
| Single-motor primary | 3 |
| 10% / 30% | 2–3 |
| Safety gate | 3 |
| Max 16 / fallback | 1–3 |
| Unit tests layouts | 1 |
