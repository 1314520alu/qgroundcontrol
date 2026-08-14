# Motor Test UI Redesign

**Date:** 2026-08-14  
**Status:** Approved (mockup)  
**Scope:** `src/AutoPilotPlugins/Common/MotorComponent.qml` (+ shared helpers as needed)  
**Reference:** [ArduPilot Connect ESCs and Motors](https://ardupilot.org/copter/docs/connect-escs-and-motors.html)

## Problem

The current Motor Setup page is a sparse column: throttle slider, prop warning, letter/number buttons, All/Stop, and a safety switch. It does not:

- Show spatial motor layout for the connected airframe
- Distinguish planar vs coaxial topologies
- Match ArduPilot Mission Planner motor-test lettering / spin directions
- Prioritize single-motor debugging with a safe throttle ceiling

## Goals

1. **Safety-first flow** — prop-removed interlock before any motor output
2. **Clear split layout** — diagram left, controls right
3. **Auto airframe topology** — layout derived from the vehicle; no user frame picker on this page
4. **Single-motor debug as primary action** — select one motor, then test it
5. **Safe throttle defaults** — default **10%**, hard max **30%**
6. **ArduPilot-aligned labeling** — letter **A** starts at front-right (X) / nose (+), then clockwise; coaxial top-then-bottom per arm
7. **Support up to 16 motors**; unknown count falls back to a numbered grid with warning

## Non-goals

- Changing MAVLink `motorTest` protocol or firmware behavior
- Replacing Vehicle Setup sidebar / navigation
- Full 3D / CAD rendering in production QML (2D SVG-quality diagram is enough)
- Editing FRAME_CLASS / FRAME_TYPE from this page

## UX Layout (Option B — Split)

```text
┌──────────────────────────────┬─────────────────────────────┐
│  Top-down motor diagram      │  Vehicle meta (read-only)   │
│  (auto topology)             │  Safety interlock           │
│  • Select → highlight        │  Throttle 0–30% (def 10%)   │
│  • Spin feedback while test  │  Single-motor debug card    │
│                              │    [测试此电机] [停止]        │
│                              │  Secondary: All / Stop all  │
│                              │  Status line                │
└──────────────────────────────┴─────────────────────────────┘
```

Narrow / mobile: stack diagram above controls (same components).

## Behavior

### Safety interlock

- Default: motors and throttle disabled
- Label (off): confirm propellers removed — enable controls
- Label (on): careful — motors enabled
- Turning **off** immediately stops all motors and resets throttle to **0**
- Enabling sets throttle to default **10%**

### Throttle

| Rule | Value |
|------|--------|
| Default when enabled | 10% |
| UI / command hard ceiling | 30% |
| Slider range | 0–30 only |
| Auto-stop timeout | keep existing ~3s per `motorTest` |

All `Vehicle::motorTest(...)` percent arguments must be clamped to `≤ 30` in the UI layer.

### Single-motor debug (primary)

1. User enables safety switch  
2. User taps a motor on the diagram → that motor is **selected and highlighted** (ring + scale + letter badge); others dim  
3. User taps **测试此电机** → only that motor runs at current throttle for the timeout  
4. **停止** stops output immediately  

Optional: tapping an already-selected motor while enabled may also start a test (same as “测试此电机”) — implement if it feels natural; not required for v1.

### All / Stop (secondary)

- **全部**: start test on every motor at current throttle (still ≤30%)  
- **停止全部**: stop all  

### Selection highlight

- Selected motor: blue selection ring, slight scale-up, letter badge filled blue with white letter  
- Non-selected: reduced opacity while a selection exists  
- Selection cleared when safety switch turns off or vehicle/frame topology rebuilds  

### Spin feedback (diagram)

While a motor is under test:

- Propeller blades rotate continuously in the correct CW/CCW direction  
- Speed scales with throttle (within 0–30%)  
- Higher throttle increases motion-blur disc opacity  

Idle motors show static blades + CW/CCW color cue (green CW / cyan CCW per ArduPilot diagrams).

## Airframe / topology (automatic)

### Source of truth

- Motor **count**: existing `Vehicle::motorCount()` (`MAV_TYPE` + submarine `FRAME_CONFIG` where applicable)  
- Motor **layout + spin direction + lettering**: new helper driven by firmware frame class/type when available (APM `FRAME_CLASS` / `FRAME_TYPE`; PX4 equivalent mapping where possible)  
- Page must **not** expose a frame picker; meta row is read-only (“自动”)

### Lettering rules (Mission Planner Motor Test compatible)

- **X frames**: first motor to the **right of nose** = **A**, then **clockwise**  
- **+ frames**: motor at **nose** = **A**, then **clockwise**  
- **Coaxial (OctoQuad / Y6)**: at each arm, **top then bottom** (A top, B bottom, …), proceeding clockwise  
- Display letters **A…** (existing `userLetterMotorIndices` path); map letter → `motorTest` index `1…N`

### Colors (diagram)

| Meaning | Color |
|---------|--------|
| CW | `#33cc33` (green) |
| CCW | `#00b8e6` (cyan) |
| Letter (idle) | red badge |
| Frame | light fill `#ebebeb`, muted purple stroke |
| Selection | blue `#2563eb` |

### Supported topologies (v1)

Minimum set aligned with ArduPilot diagrams:

- Quad X / Quad +  
- Hexa X  
- Octo X  
- OctoQuad X (coaxial X8)  
- Y6B (coaxial)  

Additional frames (Deca, Dodecahexa, etc.): place motors on a correct angular layout with A-start clockwise when count ≤ 16; if spin table unknown, still show layout + letters but omit guaranteed CW/CCW (or use best-effort from docs).

### Fallback

- `motorCount == -1`: warning text + numbered grid (cap **16**), no spatial diagram claim  
- Count > 16: show first 16 with warning that extras are not diagrammed (should be rare)

## Architecture

```text
MotorComponent.qml
  ├── SafetySwitch / throttle (0–30)
  ├── MotorTestDiagram.qml      // left: SVG/QML canvas, selection, spin anim
  └── MotorTestControls.qml     // right: meta, debug card, all/stop
         │
         └── MotorLayoutModel (C++ or JS)
               inputs: vehicle type, frame class/type, motorCount
               outputs: [{ letter, motorIndex, x, y, dir, layer }]
```

Prefer a small reusable `MotorLayoutModel` so diagram and tests stay consistent with ArduPilot order tables.

Keep calling existing:

```cpp
vehicle->motorTest(motorIndex, percent, timeoutSecs, showError);
```

with `percent` clamped in QML/controller.

## Error / edge cases

- No active vehicle: page already gated by Vehicle Setup; still null-check `controller.vehicle`  
- Armed vehicle: follow existing Vehicle Setup armed lockout if present; do not enable motor test while armed if platform already forbids it  
- Safety off: ignore diagram clicks for test; may still allow selection only after enable (v1: both selection and test require enable)  
- Throttle 0: show status hint; do not send spin commands  

## Testing

- Unit-test `MotorLayoutModel` letter/order/dir for Quad X, Quad +, Hexa X, Octo X, OctoQuad X, Y6B against ArduPilot reference  
- UI: safety off blocks test; enable → default 10%; slider cannot exceed 30%; selection highlight; single-motor and all/stop call `motorTest` with expected indices  
- Manual: compare diagram to ArduPilot doc screenshots for Quad X and X8  

## Out of scope follow-ups

- PX4-only frame variants with different lettering  
- Showing real airframe bitmap from Airframe component assets under the motors  
- Persist last throttle (not desired — always return to safe default on enable)

## Acceptance checklist

- [ ] Split layout with auto diagram + controls  
- [ ] No frame picker on the page  
- [ ] Coaxial arms show top/bottom and correct A… order  
- [ ] A starts front-right (X) / nose (+)  
- [ ] CW green / CCW cyan  
- [ ] Selected motor clearly highlighted  
- [ ] Primary: select + 测试此电机  
- [ ] Default throttle 10%, hard max 30%  
- [ ] Safety switch gates controls; off stops all and zeros throttle  
- [ ] Max 16 motors with unknown-count fallback  
