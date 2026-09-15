# SIYI ZR10 video source + gimbal control

**Date:** 2026-09-12  
**Status:** Approved (user: option B, approach 1, execute)  
**Parent:** A8 Mini video source (`docs/superpowers/specs/2026-08-17-siyi-a8-mini-video-source-design.md`) and UniGCS overlay.

## Problem

Operators with a SIYI ZR10 gimbal have no video-source preset. A8 Mini / R1M already cover the same pre-ZT30 RTSP path, but selecting those labels is wrong, and the payload overlay only binds gimbal/photo/zoom to A8 Mini.

## Behavior

1. Add **SIYI ZR10** to `videoSource` (Chinese: **思翼 ZR10**).
2. On select: RTSP `rtsp://192.168.144.25:8554/main.264`, sync `rtspUrl`, `ensureSiyiRadioEthernet()`, wait for `192.168.144.x` (same as A8 Mini).
3. Settings UI: treat as preset RTSP (hide manual URL).
4. Reuse `SiyiA8MiniCameraControl` + `UnipodMt11Client` (UDP `192.168.144.25:37260`). Do not add a second control class.
5. When ZR10 is selected, `modelName()` is `SIYI ZR10`; overlay buttons match A8 Mini phase-1: gimbal (four quick actions), zoom, photo, video, exposure. No lens / laser / media library.
6. Photo-fail messages use the selected model name (ZR10 vs A8 Mini).
7. Switching A8 Mini ↔ ZR10 updates the camera label; same control instance stays current.

## Out of scope

- Auto-select from SDK hardware ID (`0x6B` documented in catalog only).
- ZR10-only AI / laser / new SDK commands.
- Renaming `SiyiA8MiniCameraControl` to a generic SIYI class.
