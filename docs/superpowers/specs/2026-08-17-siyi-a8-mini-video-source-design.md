# SIYI A8 Mini video source (design)

**Date:** 2026-08-17  
**Scope:** Video settings preset only (no SDK photo/record).  
**Approved:** Option A; RTSP `rtsp://192.168.144.25:8554/main.264` (pre-ZT30 / same as R1M).

## Behavior

1. Add **SIYI A8 Mini** to `videoSource` enum (alongside UniPod MT11 / SIYI R1M).
2. On select: set RTSP URI to `main.264`, sync editable `rtspUrl`, call `ensureSiyiRadioEthernet()`, wait for `192.168.144.x` before start (via existing `usesSiyiRadioEthernet`).
3. Settings UI: treat as preset RTSP (hide manual URL field; show auto-configured hint).

## Out of scope

SDK gimbal/photo control (`0x0C` etc.), auto-select on remote detect.
