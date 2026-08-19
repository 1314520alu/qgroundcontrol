# UniPod MT11 Photo / Video Control

**Date:** 2026-08-15  
**Status:** Implemented — pending HIL (final-review Important fixes applied)  
**Plan:** `docs/superpowers/plans/2026-08-15-unipod-mt11-photo-video-control.md`  
**Scope:** Onboard photo + record for UniPod MT11 via external SDK, wired to existing Fly view red-circle controls  
**Primary targets:** Landscape handheld remotes (SIYI / Skydroid) with radio ethernet `192.168.144.x`  
**Camera / SDK:** 锐川 UniPod MT11 — `docs/manuals/unipod-mt11/` (`db/sdk.db` is command authority)

## Problem

1. Fly view photo/video strip (`PhotoVideoControl`) drives `MavlinkCameraControlInterface` (typically `SimulatedCameraControl`: `DIGICAM_CONTROL` + **local** GStreamer record). That does **not** trigger UniPod MT11 **onboard** capture on the payload TF card.
2. Video source **UniPod MT11** already pulls RTSP and waits for `192.168.144.x`; SDK binary control (`0x0C` / `0x0A` / `0x0B` over UDP `37260`) is not wired.

## Goals

1. **Onboard photo** — when video source is UniPod MT11 and ethernet is ready, photo button sends SDK `0x0C` `func_type=0`.
2. **Onboard record toggle** — video button sends SDK `0x0C` `func_type=2`.
3. **Recording UI state** — drive capturing / idle / no-card from `0x0A` ACK `record_sta` and optional `0x0B` pushes.
4. **Reuse existing Fly UI** — keep `PhotoVideoControl.qml`; no second control strip for MT11.

## Non-goals (Phase 1)

- TCP transport / `0x00` heartbeat
- Gimbal, zoom, thermal, laser, AI tracking
- Presenting local GStreamer recording as onboard recording
- Web Server media browse/delete UI
- Full MAVLink Camera Protocol emulation for MT11
- Changing RTSP URL / multi-stream layout (`0x10`/`0x11`)
- Phase 2+ SDK features

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Scope | Photo + record only (user option 1); zoom later |
| Approach | **A** — new `UnipodMt11CameraControl` + UDP client |
| Transport | UDP `192.168.144.25:37260` |
| UI | Existing `PhotoVideoControl` via `MavlinkCameraControlInterface` |
| Status | Poll `0x0A` (~1 Hz while active) + apply `0x0B` when received |
| Modes UI | `hasModes() = false` Phase 1 (photo allowed without photo/video mode toggle) |
| Record destination | Payload TF card (SDK), not GCS local file |

## Architecture

```text
VideoSettings.videoSource == UniPod MT11
        │
        ├─► VideoManager          RTSP + ethernet gate (existing)
        │
        └─► UnipodMt11Client      UDP 192.168.144.25:37260
                 │  frame + CRC16
                 │  send 0x0C / request 0x0A / recv 0x0A ACK + 0x0B
                 ▼
           UnipodMt11CameraControl : MavlinkCameraControlInterface
                 │
                 ▼
           QGCCameraManager (prefer / inject when MT11 video source active)
                 │
                 ▼
           PhotoVideoControl.qml
```

### Module responsibilities

| Unit | Responsibility |
| --- | --- |
| `UnipodMt11Protocol` | STX/CTRL/LEN/SEQ/CMD/DATA/CRC16 pack & unpack (LE); CRC matches SDK PDF |
| `UnipodMt11Client` | `QUdpSocket`; open when source is MT11 and `192.168.144.x` ready; send/recv; emit status |
| `UnipodMt11CameraControl` | Map SDK ↔ `takePhoto` / `toggleVideoRecording` / `captureVideoState` / `capturePhotosState` |
| `QGCCameraManager` (small glue) | When MT11 video source active, expose UniPod control as the instance `PhotoVideoControl` uses |

### Lifecycle

1. User selects video source **UniPod MT11**.
2. Existing ethernet gate becomes ready.
3. Client opens UDP; starts `0x0A` poll while Fly view needs camera UI.
4. Photo / record buttons call client; UI follows `record_sta` / `0x0B`.
5. Video source leaves MT11 or teardown → stop poll, close socket.

Do **not** start the client only because a vehicle is connected without MT11 video source.

### Camera manager inject rule

- `PhotoVideoControl` requires `activeVehicle` (unchanged QGC behavior).
- When `videoSource == UniPod MT11` and client is ready, **prefer** `UnipodMt11CameraControl` over `SimulatedCameraControl` for the current camera driven by the strip.
- If a real MAVLink camera also exists, MT11 video source still wins for this strip while source remains UniPod (avoid DIGICAM-only photo path). Exact list bookkeeping is an implementation detail; must not leave Simulated DIGICAM as the only photo path under MT11 source.
- Non-MT11 sources: existing Simulated / MAVLink camera behavior unchanged.

## Protocol mapping (Phase 1)

Authority: `docs/manuals/unipod-mt11/db/sdk.db`.

| Action | CMD | Payload | Notes |
| --- | --- | --- | --- |
| Take photo | `0x0C` | `func_type=0` | No ACK; example `55 66 01 01 00 00 00 0C 00 34 CE` |
| Toggle record | `0x0C` | `func_type=2` | No ACK; example `… 0C 02 76 EE` |
| Query system | `0x0A` | empty | ACK includes `record_sta` |
| Function feedback | `0x0B` | push | photo success/fail, start/end record, etc. |

### `0x0A` `record_sta` → UI

| Value | UI |
| --- | --- |
| 0 | Record idle |
| 1 | Recording |
| 2 | No TF card — disable record + user message |
| 3 | Recording with drop warning — treat as recording + optional warning |

### Camera interface mapping

| QGC API | MT11 behavior |
| --- | --- |
| `capturesPhotos()` | `true` when client ready |
| `capturesVideo()` | `true` when client ready (onboard, not local GST) |
| `hasVideoStream()` | follow `VideoManager::decoding()` |
| `hasModes()` | `false` Phase 1 |
| `takePhoto()` | send `0x0C`/0 |
| `toggleVideoRecording()` | send `0x0C`/2; state from `record_sta` |
| `startVideoRecording` / `stop` | implement via same toggle path (UI uses toggle) |
| Local GST record | Do **not** call for MT11 strip |

## Error handling

| Condition | Behavior |
| --- | --- |
| No `192.168.144.x` | Client not started; photo/record actions disabled or unavailable until ready |
| UDP send failure | Log; optional short toast |
| `record_sta=2` | “No storage card” style message; recording controls disabled |
| Photo with no `0x0B` | Fire-and-forget OK; do not block UI |
| CRC / short packet | Drop; keep polling |

## Testing

| Case | Expect |
| --- | --- |
| MT11 + eth ready + photo | UDP 37260 packet matches handbook example CRC |
| Record toggle | `record_sta` 0↔1; button state matches |
| No card | `record_sta=2` → message, not false “recording” |
| Switch source away from MT11 | Client stops; no leftover sockets |
| Non-MT11 RTSP | Simulated / DIGICAM behavior unchanged |
| Unit | CRC + frame pack/unpack without hardware |

Hardware-in-loop on SIYI remote is primary validation.

## Related (optional, same PR only if already broken)

If Fly view strip visibility toggle (`showRecControl` / `FactCheckBoxSlider`) is confirmed broken in this fork, fix it in the same change set so users can show the strip. Not required for SDK correctness.

## Out of scope follow-ups (Phase 2+)

- TCP + heartbeat
- Zoom (`0x05`/`0x0F`), gimbal (`0x07`/`0x0E`), thermal, laser, AI
- Media gallery via Web Server API
- S.BUS / UniGCS parity features

## References

- `docs/manuals/unipod-mt11/UniPod-MT11-SDK-Handbook.md`
- `docs/manuals/unipod-mt11/db/sdk.db`
- `src/FlightMap/Widgets/PhotoVideoControl.qml`
- `src/Camera/SimulatedCameraControl.*`
- `src/Camera/MavlinkCameraControlInterface.h`
- `src/VideoManager/VideoManager.*` (ethernet gate, `videoSourceUnipodMT11`)
