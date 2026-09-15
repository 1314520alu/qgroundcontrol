# UniGCS Gimbal Expand (Four Quick Actions)

**Date:** 2026-09-08  
**Status:** Approved (user: four UniGCS keys only; no D-pad)  
**Plan:** `docs/superpowers/plans/2026-09-08-unigcs-gimbal-expand.md`  
**Parent:** `docs/superpowers/specs/2026-08-21-unigcs-payload-overlay-design.md`  
**Reference screenshot:** UniGCS expand column (`assets/unigcs-gimbal-expanded.png` under Cursor project assets; UI dump labels: 回中 / 向下 / 偏航回中 / 俯仰朝下)

## Problem

Tapping **云台** in QGC’s UniGCS-style payload overlay opens `TopotekGimbalPad` (↑←Home→↓ hold-to-slew). UniGCS opens a **second vertical column** of four one-shot actions next to the left bar. Operators on SIYI remotes expect that pattern.

## Goals

1. Replace the gimbal D-pad expand with a UniGCS-like secondary column of four circular icon buttons + Chinese labels.
2. Wire actions through camera capability APIs; hide a button if that camera cannot perform it.
3. Keep existing expand exclusivity and tap-outside / re-tap collapse.

## Non-goals

- Continuous PTZ D-pad / speed hold UI (remove from this expand path; `TopotekGimbalPad` may remain in tree unused by overlay until a later cleanup).
- Separate long-press D-pad entry.
- Gimbal angle HUD / numeric pitch display.
- Changing right-bar photo/video feedback.

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Expand content | **Only** 回中 · 向下 · 偏航回中 · 俯仰朝下 |
| Layout | Second column immediately to the right of left payload bar (`leftBar`) |
| Style | Same `FlyViewPayloadIconButton` language as main bar; optional slightly stronger selected tint on「云台」while open |
| Collapse | Re-tap 云台, or tap empty video (existing overlay MouseArea) |
| Missing capability | **Hide** that sub-button (not greyed) |
| SIYI / UniPod / A8 Mini | Map to SDK `0x08` center modes (`center_pos` 1–4) |
| Topotek TQ10N | Only home (`PTZ` `05`) is confirmed → show **回中** only until more PTZ modes exist |

## UniGCS → SDK mapping (SIYI family)

Protocol: `UnipodMt11Protocol::buildCenterCommand(seq, mode)` CMD `0x08`.  
Enums from `docs/manuals/unipod-mt11/db/sdk.json` → `center_pos`:

| UniGCS label | `mode` | SDK label |
| --- | --- | --- |
| 回中 | 1 | 一键回中 |
| 向下 | 4 | 朝下 |
| 偏航回中 | 3 | 居中 |
| 俯仰朝下 | 2 | 居中朝下 |

`SiyiA8MiniCameraControl` already uses `UnipodMt11Client`; extend client/control with typed helpers rather than hard-coding mode numbers in QML.

## Architecture

```text
FlyViewPayloadOverlay
  leftBar (云台 selected while Expand.Gimbal)
  expandHost
    Column (NEW)  — four FlyViewPayloadIconButton
      visible when expand == Gimbal && hasGimbalPad
      each button visible if camera reports that quick action

MavlinkCameraControlInterface
  hasGimbalPad (existing)
  hasGimbalRecenter / hasGimbalLookDown / hasGimbalYawRecenter / hasGimbalPitchDown
    OR single hasGimbalQuickActions + per-action methods that no-op when unsupported
  Q_INVOKABLE gimbalRecenter() / gimbalLookDown() / gimbalYawRecenter() / gimbalPitchDown()

UnipodMt11Client (+ CameraControl wrappers)
  ptzHome() → mode 1 (existing)
  new: center command modes 2–4

TopotekTq10CameraControl
  only gimbalRecenter() → existing ptzHome(); other has* false
```

**Preferred API shape (minimal QML branching):**

```text
bool hasGimbalRecenter() const
bool hasGimbalLookDown() const
bool hasGimbalYawRecenter() const
bool hasGimbalPitchDown() const

void gimbalRecenter()
void gimbalLookDown()
void gimbalYawRecenter()
void gimbalPitchDown()
```

Defaults on `MavlinkCameraControlInterface`: all `has*` false; methods no-op.  
SIYI/UniPod/A8: all four true. Topotek: only recenter true.

## UI details

- Column `anchors.left: leftBar.right`; vertical align with「云台」row if practical, else top of expand host (same as today’s pad).
- Circle size: match `leftBar.circleSize` (or ~0.92× if four buttons need height).
- Labels: `qsTr("Recenter")` etc. with zh_CN translations 回中 / 向下 / 偏航回中 / 俯仰朝下.
- Icons: reuse gimbal-related SVGs under `InstrumentValueIcons` / `qmlimages`; prefer distinct glyphs if available, else same family with different labels (labels carry meaning).
- Short landscape: respect `bottomReserve`; do not cover PIP/compass.

## Testing

- Unit: protocol builders emit correct `0x08` payloads for modes 1–4; capability defaults false; SIYI/UniPod report four true; Topotek only recenter.
- Device: Android landscape remote — tap 云台 → four keys; each fires; collapse works; Topotek shows only 回中 if that camera is active.

## Success criteria

1. No D-pad in the 云台 expand path.
2. Visual/interaction match UniGCS second column (four actions beside left bar).
3. SIYI-family cameras drive all four via `0x08` modes.
4. Unsupported actions stay hidden.
