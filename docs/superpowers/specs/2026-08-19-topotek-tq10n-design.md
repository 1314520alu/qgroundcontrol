# Topotek TQ10N — QGC Integration Design

**Date:** 2026-08-19  
**Status:** Implemented  
**Scope:** Video preset + UDP control + Fly View split UI (Phase-1)

## Goals

Integrate Topotek **TQ10N** as a first-class video source on SIYI/Skydroid landscape remotes:

- RTSP `rtsp://192.168.144.108:554/stream=0`
- Independent UDP `#TP` stack (not UniPod `55 66`)
- Photo, record, hold zoom, PTZ D-pad + home
- **No** SMB media gallery

## Architecture

Parallel to UniPod MT11:

```
VideoSettings (Topotek TQ10N)
    → VideoManager.ensureSiyiRadioEthernet() + RTSP
    → QGCCameraManager._syncTopotekCamera()
        → TopotekTq10Client (UDP :9004 → :9003)
        → TopotekTq10CameraControl (UI adapter)
    → PhotoVideoControl split strip (gimbal left, capture right)
    → FlyViewLocalVideoControls fallback when no vehicle
```

## UI (landscape-first)

| Zone | Component | Interaction |
|------|-----------|-------------|
| Left | `TopotekGimbalPad` | Hold ↑↓←→ → PTZ; tap center → home |
| Right | Existing double-ring photo/video | Tap toggle |
| Right | `TopotekZoomHoldButtons` | Hold ± → ZMC; release → stop |

Hide vertical `QGCSlider` when `hasGimbalPad` — TQ10N uses hold buttons.

## Video source

- Enum: `VideoSettings::videoSourceTopotekTq10N` = `"Topotek TQ10N"`
- RTSP constant: `topotekTq10NRtspUrl`
- `usesSiyiRadioEthernet()` includes TQ10N

## Simulated camera exclusion

When Topotek source is active, skip `_ensureSimulatedCameraForLocalRecord()` — onboard TF capture must not fall back to local GST record.

## Tests

`TopotekTq10ProtocolTest` — CRC and hex strings vs Python reference.

## References

- [docs/manuals/topotek-tq10n/](../manuals/topotek-tq10n/)
- UniPod pattern: `UnipodMt11Client`, `QGCCameraManager._syncUnipodCamera()`
