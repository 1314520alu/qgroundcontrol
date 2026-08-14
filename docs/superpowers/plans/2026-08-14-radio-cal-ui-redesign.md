# Radio Calibration UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Match `radio-cal-preview-split.png`: landscape split cards (摇杆预览 | 姿态通道) + bottom 通道监视; remove Spektrum / CRSF / Copy Trims.

**Architecture:** Restyle shared `RemoteControlCalibration.qml` (Radio + Joystick). Keep controller aliases (`statusText` / `nextButton` / `cancelButton`) and `additionalSetup`/`additionalMonitor` loaders for Joystick. Strip bind/trim actions only from `RadioComponent.qml`.

**Tech Stack:** Qt 6 QML, existing `RemoteControlChannelValueDisplay` / `RemoteControlChannelMonitor` / `RadioComponentController`.

## Global Constraints

- Visual target: first preview `radio-cal-preview-split.png` (not wizard variant).
- Landscape-first (UniRC / Skydroid short height): top split, bottom monitor — no tall single column.
- Delete Spektrum Bind, CRSF Bind, Copy Trims from Radio page UI.
- Do not break Joystick page (shared calibration QML).
- Fact System / vehicle null-checks unchanged.
- Conventional Commits if committing.

## File map

| File | Role |
|------|------|
| `src/Vehicle/VehicleSetup/RemoteControlCalibration.qml` | Split card layout |
| `src/Vehicle/VehicleSetup/RemoteControlChannelMonitor.qml` | Title + 2-col default for Radio |
| `src/AutoPilotPlugins/Common/RadioComponent.qml` | Remove bind/trim; optional PX4 AUX only |

### Task 1: Layout to match preview

- [x] Rewrite `RemoteControlCalibration.qml`: card「摇杆预览」(Mode / 油门居中 / 双杆 / 开始校准·取消 / status) | card「姿态通道」(4 axes + value + 已映射) ; bottom「通道监视」two-column
- [x] Keep extension/aux rows only when enabled (Joystick)
- [x] Keep `additionalSetup` / `additionalMonitor` loaders below for Joystick tabs

### Task 2: Strip Radio extras

- [x] `RadioComponent.qml`: remove Spektrum/CRSF/Copy Trims + dialogs; keep PX4 AUX combo only when PX4

### Task 3: Verify

- [x] Incremental build / QML load
- [ ] Manual: Radio page matches split preview on landscape
