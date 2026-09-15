# SIYI gimbal pitch tape (DJI-style)

**Date:** 2026-09-15  
**Status:** Draft for user review  
**Parent:** UniGCS overlay (`docs/superpowers/specs/2026-08-21-unigcs-payload-overlay-design.md`), SIYI zoom HUD (Fly View payload overlay)  
**Cameras:** SIYI ZR10 / A8 Mini / UniPod MT11 via `UnipodMt11Client` (`192.168.144.25:37260`)  
**Primary targets:** Landscape remotes (Skydroid G20, SIYI MK32/MK15); short height is the constraint  
**Chosen look:** Layout A — vertical scrolling tape inside the zoom/photo column (right side of video)

## Problem

When the payload gimbal pitches (RC gimbal keys or overlay 回中/向下), Fly View has no on-video pitch cue. DJI FPV shows a short vertical **scrolling tape** with a fixed center readout (e.g. `-27°`) that appears while the gimbal moves and then lingers. Operators on this fork expect that pattern. Zoom already has a C++-owned HUD; pitch does not.

## Goals

1. Show a DJI-style **pitch-only** tape on the video while SIYI-family gimbal pitch is changing, then hide after a linger.
2. Drive the number from live SDK attitude (**CMD `0x0D`**), not from stick speed guesses and not from MAVLink `Gimbal.absolutePitch`.
3. Work for **RC gimbal keys** (SBUS to the gimbal; GCS never sees the stick) and for overlay **0x08** quick actions that change pitch.
4. Reuse the zoom-HUD ownership pattern so QML does not re-read a stale property cache.

## Non-goals

- Yaw (or roll) tape / compass bar.
- CMD **`0x25`** 10 Hz attitude push (ZR10 support unverified; revisit later).
- MAVLink toolbar gimbal, `GimbalController`, or non-UDP cameras (Topotek TQ10N stays without this tape).
- Restoring the overlay PTZ D-pad (removed by UniGCS gimbal expand).
- Fake degrees from CMD `0x07` speed.
- Changing zoom HUD behavior (`0x18` hold poll, C++-owned zoom HUD).

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| When | While pitch is moving, then linger **5 s** (same as zoom HUD) |
| Cameras | Payload UDP only: ZR10 / A8 Mini / UniPod |
| Axes | Pitch only |
| Style | DJI scrolling tape + fixed center readout |
| Position | A — right of video, **left of** the zoom/photo column |
| Data | Poll CMD **`0x0D`** (ACK values `/10` = degrees) |
| First sample | Prime only — no flash on connect |
| Display | Integer degrees with `°` (e.g. `-27°`) |
| Show threshold | After prime, show when **`qRound(pitch)`** changes |
| Linger | `5000` ms after last show, single-shot timer restarted on each show |

## Protocol

CMD **`0x0D` Request gimbal attitude**. Request payload empty. Example from `docs/manuals/unipod-mt11/db/sdk.json`:

`55 66 01 00 00 00 00 0D E8 05`

ACK payload (SIYI / UniPod; little-endian `int16`; unit **0.1°** / **0.1°/s**):

| Offset | Field | Use |
| --- | --- | --- |
| 0 | yaw | Ignore for HUD |
| 2 | pitch | Tape + readout (`/ 10.0`) |
| 4 | roll | Ignore |
| 6 | yaw_velocity | Ignore |
| 8 | pitch_velocity | Ignore |
| 10 | roll_velocity | Ignore |

Parse **pitch if payload is at least 4 bytes** (yaw+pitch). Prefer 12-byte ACKs; do not fail if trailing velocity fields are missing. Sign: **positive pitch = look up**, **negative = look down**, **0° = horizon**. Mechanical range across these products is about **+30° … −90°**; clamp drawing to that, still show the raw rounded value if a sample is slightly outside.

Do **not** enable `0x25` in this change.

## Architecture

```text
UnipodMt11Protocol
  buildGimbalAttitudeRequest(seq)     CMD 0x0D, empty payload
  parseGimbalAttitudeAck(payload, …)  pitch (required), yaw/roll optional

UnipodMt11Client
  existing 150 ms / 80 ms poll timer (same as 0x18)
  each tick: requestCurrentZoom() AND requestGimbalAttitude()
  80 ms when zoom hold OR overlay PTZ pitch hold (0x07 up/down)
  150 ms otherwise (covers RC gimbal keys)
  pitchDegrees property (NaN until first good ACK)

QGCCameraManager
  siyiPitchDegrees / siyiPitchHudVisible / siyiPitchHudText
  first finite sample primes; later integer-degree changes → showSiyiPitchHud()
  hide timer 5000 ms; QML copies from signal arguments (not cached getters)

FlyViewPayloadOverlay
  GimbalPitchTape (new QML)
  anchors: right of video, left of rightBar (zoom/photo)
  visible from C++ HUD flag; pitch from C++ degrees
```

**Poll policy (explicit):** while the SIYI UDP client is running, send **`0x0D` on every zoom-poll tick**, including idle. RC gimbal never goes through GCS; idle `0x0D` is the only way to see it. Doubling the existing 150 ms packet (`0x18` + `0x0D`) is accepted. Do not wait for the 1 s system-info timer.

**PTZ hold:** `ptzStart` with pitch up/down (if anything still sends `0x07`) shortens the interval to 80 ms, same as zoom hold. Overlay 0x08 one-shots do not need 80 ms; 150 ms plus the pitch jump is enough.

**QML cache:** same workaround as zoom. `Connections` on `siyiPitchHudVisibleChanged(visible)` / `siyiPitchHudTextChanged(text)` / `siyiPitchDegreesChanged(pitch)` must assign from **signal arguments**. Do not re-read the properties inside the handler.

**Disconnect:** client stop or camera switch resets pitch to NaN, hides the tape, and clears the primed flag so the next connect does not flash.

## UI

Vertical **scrolling** scale: ticks move; the center window stays put.

```text
                    ┌───┐
                 10 ┤   │
                    │   │
                  0 ┤───│  ← longer tick = horizon
                    │-27│  ← fixed center window
                    │   │
                -40 ┤   │
                    └───┘
              left of rightBar
```

- **Scroll:** looking up (pitch increases) moves ticks **down**, like DJI. Current value is always in the center window.
- **Window:** rounded dark fill (`rgba(0,0,0,0.45)`), white integer + `°` with **no space** (`tr("%1°").arg(qRound(pitch))`, e.g. `-27°`), thin light border — same family as the zoom HUD, not a new accent color.
- **QML file:** new `src/FlyView/GimbalPitchTape.qml`, add to `FlyView/CMakeLists.txt` `QML_FILES`. Overlay hosts it; do not inline a Canvas in `FlyViewPayloadOverlay.qml`.
- **Ticks:** major every **10°**, labeled; minor every **5°**, unlabeled. `0°` tick is longer/brighter. No yaw marks.
- **Visible span:** about **±25°** around current pitch (tape height maps to ~50° of scale). At +30° or −90° the tape **stops** at the end; it does not wrap.
- **Placement:** `anchors.right: rightBar.left` with a small gap; vertically centered in the overlay, **above `bottomReserve`**. Must not cover leftBar, rightBar, zoom HUD (top center), or laser HUD. Short landscape: width ≤ ~5 character widths; height ≤ ~42% of overlay (or ~12 line heights), whichever is smaller. Glove-safe: the tape is display-only (not a touch target).
- **When hidden:** occupy no hit-testing and no layout that shoves the photo/zoom column.

Zoom HUD and pitch tape may be visible together (RC zoom + gimbal). They do not share a slot.

## Error handling

- Bad CRC / unparsable `0x0D`: drop the datagram (existing frame parser). Keep last good pitch; do not hide the tape solely because one poll failed.
- No ACK after connect: pitch stays NaN, tape stays hidden.
- Ethernet loss: existing client `stop()` hides everything via reset.

## Testing

- **Protocol:** `buildGimbalAttitudeRequest(0)` matches the handbook hex above. `parseGimbalAttitudeAck` on a 12-byte payload with pitch raw `-270` → `-27.0°`; reject payloads shorter than 4 bytes; accept 6-byte (yaw+pitch+roll) without velocities.
- **Manager:** first pitch sample does not set `siyiPitchHudVisible`; changing rounded degrees does; hide timer expires at 5 s; zoom HUD tests still pass.
- **Client (unit where practical):** poll path requests `0x0D`; `0x0D` ACK updates `pitchDegrees`.
- **Device:** Android landscape remote, ZR10 (and A8 Mini / MT11 if handy). RC pitch: tape appears, center matches motion, hides ~5 s after stop. Overlay 回中 / 向下: tape appears. Connect: no flash. Zoom HUD still works while holding +/-.

## Success criteria

1. Pitch tape appears on SIYI UDP cameras while pitch changes (RC or 0x08), with a DJI-style scrolling scale and integer center readout.
2. No tape on connect, on yaw-only motion (including 偏航回中 if rounded pitch is unchanged), or on Topotek / MAVLink-only gimbals.
3. Placement is inside the right payload column on short landscape and does not block zoom/photo.
4. Zoom `0x18` hold polling and C++-owned zoom HUD are unchanged.
