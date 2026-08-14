# Android Emulator + ArduCopter SITL (DodecaHexa / UniRC 10 Pro)

**Date:** 2026-08-14  
**Status:** Approved  
**Goal:** One-command local workflow: ArduCopter SITL as **DodecaHexa X** (6 arms, 12 coaxial motors) + Android emulator sized like **SIYI UniRC 10 Pro**, running a locally built QGC APK connected to the SITL.

## Context

- Existing pieces: `tools/simulation/mock_vehicle.py`, `tools/simulation/run-arducopter-sitl.sh` (Docker, TCP 5760), `run-qgc.sh --android` (USB install), CI Android emulator boot test, `MotorLayoutBuilder` DodecaHexa X/Plus layouts.
- Target remote: UniRC 10 Pro — 10.1″, **1920×1200** landscape, Android 13, 2000 nit.
- Not in scope (v1): PX4 SITL, full Dockerized emulator on Mac, mock-only path (already covered), Skydroid-specific AVD (same 1920×1200 can reuse later).

## Requirements

| ID | Requirement |
|----|-------------|
| R1 | SITL presents DodecaHexa: `FRAME_CLASS=12`, `FRAME_TYPE=1` (X), 12 motors |
| R2 | Physics model `--model dodeca-hexa` (ArduPilot `SIM_Frame`) |
| R3 | Local Android debug APK install into emulator (build via `env-qgc.sh` / Android-debug) |
| R4 | AVD matches UniRC 10 Pro landscape 1920×1200, API 33 |
| R5 | Emulator QGC can open TCP link to host SITL (`adb reverse` preferred) |
| R6 | Single entry: `just android-sitl` or `./tools/simulation/run-android-sitl.sh` |
| R7 | Desktop QGC to same SITL still works (`tcp://localhost:5760`) for comparison |

## Architecture

```
Host Mac                              Android Emulator (unirc-10-pro)
┌──────────────────────────┐          ┌────────────────────────────────┐
│ Docker: arducopter-sitl  │◄────────►│ QGC (Android-debug APK)        │
│ model: dodeca-hexa       │ TCP 5760 │ 1920×1200 landscape, API 33    │
│ FRAME_CLASS=12 TYPE=1    │          │ Comm link: 127.0.0.1:5760      │
│ publish :5760            │          │ via adb reverse tcp:5760       │
└──────────────────────────┘          └────────────────────────────────┘
```

Orchestrator starts SITL → ensures AVD → starts emulator → installs APK → sets up reverse → prints next steps.

## Components

### 1. SITL launcher (extend existing)

**File:** `tools/simulation/run-arducopter-sitl.sh`

- Add `--frame <name>` (default for this workflow: `dodeca-hexa`; keep current `+` as default when script invoked alone for backward compatibility, **or** document breaking default change — prefer **non-breaking**: default stays `+`, orchestrator passes `--frame dodeca-hexa`).
- When `--frame dodeca-hexa`: `--model dodeca-hexa` and load `copter.parm` + DodecaHexa params.
- Repo fallback parm: `tools/simulation/params/copter-dodecahexa-x.parm` containing at least:

```text
FRAME_CLASS 12
FRAME_TYPE 1
```

(Official `copter-dodecahexa.parm` sets `FRAME_CLASS 12` only; we explicitly set X.)

- Keep `--with-latency`, container name, port `5760`.

### 2. UniRC AVD helper

**Files:**
- `tools/simulation/avd/create-unirc-10-pro-avd.sh`
- Optional skin/config under `tools/simulation/avd/unirc-10-pro/`

| Setting | Value |
|---------|--------|
| AVD name | `unirc-10-pro` |
| Resolution | 1920×1200, landscape |
| Density | ~224 dpi (10.1″) |
| System image | Android 13 / API 33, `google_apis` |
| ABI | auto: `arm64-v8a` on Apple Silicon; `x86_64` on Intel |
| Window | visible (not CI headless) |

Idempotent: if AVD exists, skip create unless `--force`.

### 3. Android install path

**Reuse / extend:** `run-qgc.sh`

- Add `--android-emulator` (or orchestrator calls `adb` directly): pick first `emulator-*` device, install `build/Android-debug/android-build/QGroundControl.apk`, launch activity.
- ABI note: default `QT_ANDROID_ABIS=arm64-v8a` matches Apple Silicon emulator; Intel Mac must build with `x86_64` (document in README).

### 4. Orchestrator

**File:** `tools/simulation/run-android-sitl.sh`  
**Just recipe:** `android-sitl`

Flags (suggested):
- `--build` — build Android APK before install
- `--no-emulator` — SITL only (desktop QGC)
- `--with-latency` — pass through to SITL script
- `--force-avd` — recreate AVD

Steps:
1. Start DodecaHexa SITL (Docker).
2. Create/start `unirc-10-pro` emulator.
3. Wait for `adb` device online.
4. `adb reverse tcp:5760 tcp:5760`.
5. Install + launch QGC (fail with clear “build APK first” if missing and no `--build`).
6. Print: open Comm Links → TCP `127.0.0.1:5760` (if not auto-connected).

### 5. Docs

Update `tools/simulation/README.md` with UniRC + DodecaHexa section, prerequisites (Docker, Android SDK, Qt Android kit per `env-qgc.sh`), and ABI note.

## Networking

| Method | QGC host:port | When |
|--------|---------------|------|
| **Preferred** `adb reverse tcp:5760 tcp:5760` | `127.0.0.1:5760` | Default |
| Fallback | `10.0.2.2:5760` | If reverse fails |

UDP 14550 mock path remains separate; this design is TCP SITL only.

## Error handling

- Docker missing / SITL container crash → exit non-zero with log hint (`docker logs arducopter-sitl`).
- No Android SDK / emulator binary → print setup pointers (`ANDROID_SDK_ROOT` from `env-qgc.sh`).
- Wrong ABI install failure → message to set `QT_ANDROID_ABIS` for host arch.
- Signature mismatch on install → uninstall `org.mavlink.qgroundcontrol` and retry (same as `run-qgc.sh --android`).

## Success criteria

1. `just android-sitl` (after one-time SDK/Docker/APK setup) brings up SITL + UniRC-sized emulator + QGC.
2. QGC connects to SITL; vehicle shows DodecaHexa / 12 motors.
3. Motor Setup shows coaxial DodecaHexa **X** diagram (`MotorLayoutBuilder`).
4. Same SITL reachable from desktop QGC at `tcp://localhost:5760`.

## Out of scope

- Auto-creating QGC Comm Link settings inside the APK (manual add OK for v1).
- PX4, Gazebo, or hardware-in-the-loop.
- CI job wiring (optional later; local-first).
- Changing production motor UI code (consume existing DodecaHexa support only).

## File map (expected)

| Path | Role |
|------|------|
| `tools/simulation/run-arducopter-sitl.sh` | `--frame dodeca-hexa` |
| `tools/simulation/params/copter-dodecahexa-x.parm` | FRAME_CLASS/TYPE fallback |
| `tools/simulation/avd/create-unirc-10-pro-avd.sh` | AVD create |
| `tools/simulation/run-android-sitl.sh` | Orchestrator |
| `run-qgc.sh` | `--android-emulator` |
| `justfile` | `android-sitl` |
| `tools/simulation/README.md` | Docs |

## Dependencies / prerequisites (operator)

- Docker
- Android SDK + platform-tools + emulator + API 33 system image
- Qt Android kit as in `env-qgc.sh`
- Built (or `--build`) Android-debug APK
