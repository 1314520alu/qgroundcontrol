# UniGCS-style Fly View Payload Overlay

**Date:** 2026-08-21  
**Status:** Approved  
**Plan:** `docs/superpowers/plans/2026-08-21-unigcs-payload-overlay.md`  
**Scope:** Landscape Fly View left/right video overlay matching UniGCS chrome; capability-gated buttons  
**Primary targets:** Landscape handheld remotes (Skydroid G20 7″ ~1920×1200, SIYI MK32/UniRC 10″)  
**Reference:** UniGCS main UI screenshot `~/Desktop/UniGCS-main-20260821-101846.png`

## Problem

QGC Fly View keeps photo/record, gimbal pad, and zoom in one top-right `PhotoVideoControl` strip. UniGCS uses translucent icon columns **on the video**: left functions, right capture. Operators on SIYI/Skydroid remotes expect that layout. Current TQ10N split strip is still a right-side panel, not a video overlay.

## Goals

1. Overlay on full-window video: left function column + right capture column, UniGCS-like circular icons with Chinese labels.
2. Show a button **only if the active camera advertises that capability**. No greyed-out stubs.
3. Tap icon to expand (gimbal D-pad, hold zoom/focus, exposure AUTO). One expand panel at a time.
4. Reuse existing capture/PTZ/zoom/media-library behavior; do not invent new payload protocols in Phase 1.

## Non-goals (Phase 1)

- UniGCS histogram / draw-edit icons (not requested)
- New laser / AI / follow / lens / exposure protocols (buttons stay hidden until a camera sets the flag)
- Replacing QGC toolbar, map PIP, instrument panel, or compass
- Overlay inside the small PIP video window
- Chrome-less `videoManager.fullScreen` (existing QGC hide-all-widgets behavior stays)

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Approach | **A** — new overlay, capability-driven |
| Layout | Float on video: left functions, right capture |
| Expand | Tap icon → panel; tap again or tap empty video → collapse |
| Visibility | Camera capability true → show; else hide |
| AUTO | Exposure AUTO (Fact `exposureMode`); no Fact → hide |
| 保存 | Open existing UniPod media gallery; no gallery → hide |
| PIP | Overlay off; keep compact `PhotoVideoControl` |
| Video full (main) | Overlay on; hide `PhotoVideoControl` and `FlyViewLocalVideoControls` |
| Out of scope icons | Histogram, pencil |

## Architecture

```text
Fly View (video is main window, map in PIP)
  FlyViewWidgetLayer
    FlyViewToolStrip          (existing, left edge)
    FlyViewPayloadOverlay     (NEW)
      left column  (after toolstrip)
      right column (video right edge)
      expand host  (inward toward video center)
    PhotoVideoControl         hidden while overlay visible

MavlinkCameraControlInterface
  hasGimbalPad / hasLensSwitch / hasLaserRange
  hasAiRecognition / hasFollowFlight / hasMediaLibrary
  + existing hasZoom / hasFocus / capturesPhotos / capturesVideo / exposureMode
```

Mount overlay in **`FlyViewWidgetLayer`**, not inside `FlyViewVideo`:

- Same z-layer as today’s photo strip, so it sits next to `FlyViewToolStrip` and respects toolbar height.
- Visible when `mapControl.pipState` is **not** full (video is the main item) and `QGroundControl.videoManager.hasVideo`.
- Hidden when map is main (video PIP), or `videoManager.fullScreen`.

## Components

| File | Role |
| --- | --- |
| `FlyViewPayloadOverlay.qml` | Root: camera binding, visibility, exclusive expand enum, tap-outside collapse |
| `FlyViewPayloadIconButton.qml` | Circular translucent icon + label; `minTouchPixels`; pressed highlight |
| `FlyViewPayloadSideBar.qml` | Vertical column of icon buttons; used twice (left/right) |
| `TopotekGimbalPad.qml` | Reused in gimbal expand (no visual rewrite) |
| `TopotekZoomHoldButtons.qml` | Reused for 变倍; clone/parameterize for 变焦 (`startFocus`/`stopFocus`) |

`FlyViewPayloadOverlay` owns expand state:

```text
none | gimbal | lens | range | recognize | follow | auto | zoom | focus
```

保存 does **not** use expand state: it opens `UnipodMt11MediaGallery` via existing `QGCPopupDialogFactory`.

### Left column (top → bottom)

| Button | Show when | Tap |
| --- | --- | --- |
| 云台 | `hasGimbalPad` | Expand D-pad + Home |
| 镜头 | `hasLensSwitch` | Expand lens/thermal switch (Phase 1: no camera sets this) |
| 测距 | `hasLaserRange` | Toggle/query laser (Phase 1: hidden) |
| 开启识别 | `hasAiRecognition` | Toggle AI/tracking (Phase 1: hidden; later may alias `hasTracking`) |
| 跟随飞行 | `hasFollowFlight` | Toggle follow (Phase 1: hidden) |

### Right column (top → bottom)

| Button | Show when | Tap |
| --- | --- | --- |
| AUTO | `exposureMode` Fact non-null | Expand auto/manual exposure |
| 拍照 | `capturesPhotos` | `takePhoto()` (existing double-ring visual language) |
| 录像 | `capturesVideo` | `toggleVideoRecording()` |
| 变倍 | `hasZoom` | Expand hold +/- → `startZoom`/`stopZoom` |
| 变焦 | `hasFocus` | Expand hold +/- → `startFocus`/`stopFocus` |
| 保存 | `hasMediaLibrary` | Open media gallery |

Do not show empty columns: if a side has zero visible buttons, hide that `FlyViewPayloadSideBar`.

### Interaction details

- Icon diameter ≥ `ScreenTools.minTouchPixels`.
- Label: `qsTr(...)` + `smallFontPointSize`.
- Fill: `Qt.rgba(window.r,g,b, 0.45–0.55)`; press uses `buttonHighlight`.
- Left bar `anchors.left` = `toolStrip.right` + margin. Right bar `anchors.right` = parent right.
- Vertical: upper-middle of remaining height so instrument panel / virtual joysticks at bottom stay clear.
- Expand panel opens toward video center; only one at a time.
- Tap empty video (MouseArea on overlay background, not on icons) collapses expand. Do not steal gimbal click-to-point unless expand is open (click-through when `expand == none`).
- Photo and record stay **immediate actions** (no expand).

### PhotoVideoControl coexistence

```text
overlayActive = hasVideo && video is main && at least one overlay button visible

PhotoVideoControl.visible        && !overlayActive   (plus existing camera checks)
FlyViewLocalVideoControls._show  && !overlayActive
```

When map is main (PIP video), operators still get the compact strip.

## Capability API

Add **non-pure** virtuals on `MavlinkCameraControlInterface` with default `false`, plus `Q_PROPERTY` + `infoChanged`. Existing subclasses need no stub unless they opt in.

| Property | Default | Phase 1 overrides |
| --- | --- | --- |
| `hasGimbalPad` | false | `TopotekTq10CameraControl` true (move off the extra-only property) |
| `hasLensSwitch` | false | none |
| `hasLaserRange` | false | none |
| `hasAiRecognition` | false | none |
| `hasFollowFlight` | false | none |
| `hasMediaLibrary` | false | `UnipodMt11CameraControl` true only when media client exists **and** `ready` |

Existing flags already used: `hasZoom`, `hasFocus`, `capturesPhotos`, `capturesVideo`, `exposureMode()`.

QML must null-check `_camera` before every capability read.

Phase 1 wiring:

| Camera | Visible overlay buttons |
| --- | --- |
| Topotek TQ10N | 云台, 拍照, 录像, 变倍 |
| UniPod MT11 | 拍照, 录像, 保存 (gallery client present) |
| Simulated / generic MAVLink | Only flags already true (often 拍照/录像; zoom/focus/AUTO if Camera Protocol reports them) |

## Error handling

- `_camera == null` or overlay has zero buttons → overlay hidden; compact strip rules unchanged.
- Camera swapped / disconnected while expanded → reset expand to `none`.
- Media client present but not `ready` → `hasMediaLibrary` stays false, 保存 hidden.
- Capture calls already no-op when capture state is Disabled; keep that.
- Expand MouseArea must not block `OnScreenGimbalController` / tracking when expand is `none`.

## Testing

- No new protocol unit tests in Phase 1 (UI + flags only).
- Camera unit tests: Topotek `hasGimbalPad == true`; UniPod `hasMediaLibrary` follows client; Simulated all new flags false.
- Manual on UniRC / G20 short landscape: video main → overlay; map main → compact strip; TQ10N expand gimbal/zoom; MT11 保存 opens gallery.
- `just build` Android-debug after QML/C++ edits.

## Files (expected)

### New

- `src/FlyView/FlyViewPayloadOverlay.qml`
- `src/FlyView/FlyViewPayloadIconButton.qml`
- `src/FlyView/FlyViewPayloadSideBar.qml`
- `src/FlyView/FlyViewPayloadZoomHoldButtons.qml` (optional; may parameterize existing zoom buttons for focus)

### Modify

- `src/FlyView/FlyViewWidgetLayer.qml` — host overlay, pass toolstrip width
- `src/FlyView/CMakeLists.txt` — QML_FILES
- `src/FlightMap/Widgets/PhotoVideoControl.qml` — hide when overlay active; **keep** the TQ10N gimbal/zoom split for map-main / PIP fallback
- `src/FlyView/FlyViewLocalVideoControls.qml` — hide when overlay active
- `src/Camera/MavlinkCameraControlInterface.h` — new properties
- `src/Camera/TopotekTq10CameraControl.h` — `hasGimbalPad` via interface
- `src/Camera/UnipodMt11CameraControl.h/.cc` — `hasMediaLibrary`
- `translations/qgc_source_zh_CN.ts` — 云台/镜头/测距/开启识别/跟随飞行/变倍/变焦/保存/AUTO

Icons: reuse `InstrumentValueIcons/gimbal-2.svg`, `camera.svg` / `camera_photo.svg`, `camera_video.svg`, `zoom-in.svg`, `save-disk.svg`, `TrackingIcon.svg`. Add small SVGs only if missing.

## Out of scope forever unless a later spec

- Pixel-perfect clone of UniGCS assets
- Gaode inset map / UniGCS bottom compass (QGC already has its own)
- Enabling 镜头/测距/识别/跟随 without a camera implementation behind the flag
