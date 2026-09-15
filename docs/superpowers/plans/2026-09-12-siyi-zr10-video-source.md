# SIYI ZR10 Video Source Implementation Plan

> **For agentic workers:** Execute inline (user chose 执行). Use TDD for catalog/control tests. Do not commit unless asked.

**Goal:** Add SIYI ZR10 as a video-source preset with the same RTSP and gimbal/photo/zoom SDK control as A8 Mini.

**Architecture:** Reuse `SiyiA8MiniCameraControl` + `UnipodMt11Client`. New enum/catalog entry; selection checks treat ZR10 like A8 Mini; `modelName()` follows the current video source.

**Tech Stack:** Qt6, Fact `videoSource`, PayloadCapabilityCatalog JSON, existing SIYI UDP SDK.

## Global Constraints

- RTSP is `rtsp://192.168.144.25:8554/main.264` (reuse `siyiA8MiniRtspUrl`).
- No auto hardware-ID select.
- Overlay phase-1 same as A8 Mini: gimbal, exposure_auto, photo, video, zoom.
- zh_CN: 思翼 ZR10.
- Branch: `mac-and-siyi` (do not commit unless asked).

---

### Task 1: Catalog + tests

**Files:**

- Modify: `src/Camera/PayloadCapabilities.json`
- Modify: `docs/manuals/payload-capabilities/capabilities.json`
- Modify: `test/Camera/CameraPayloadCapabilitiesTest.h`
- Modify: `test/Camera/CameraPayloadCapabilitiesTest.cc`
- Modify: `test/Camera/CameraGimbalQuickActionsTest.h`
- Modify: `test/Camera/CameraGimbalQuickActionsTest.cc`

- [ ] **Step 1:** Add failing catalog + A8 gimbal tests.
- [ ] **Step 2:** Run `ctest -R 'CameraPayloadCapabilitiesTest|CameraGimbalQuickActionsTest'` — catalog ZR10 fails.
- [ ] **Step 3:** Add ZR10 camera object (same overlay_phase1 as A8 Mini; `hw_id` `0x6B`).
- [ ] **Step 4:** Re-run tests — catalog passes; gimbal A8 flags pass.

---

### Task 2: Video source wiring + control reuse

**Files:**

- Modify: `src/Settings/VideoSettings.h`, `src/Settings/VideoSettings.cc`
- Modify: `src/VideoManager/VideoManager.cc`
- Modify: `src/AppSettings/pages/Video.SettingsUI.json`
- Modify: `src/Camera/SiyiA8MiniCameraControl.h`, `src/Camera/SiyiA8MiniCameraControl.cc`
- Modify: `src/Camera/QGCCameraManager.cc`
- Modify: `src/Camera/UnipodMt11Client.cc`
- Modify: `src/FlyView/FlyViewPayloadOverlay.qml`
- Modify: `src/FlightMap/Widgets/PhotoVideoControl.qml`
- Modify: `translations/qgc_source_zh_CN.ts`

- [ ] **Step 1:** Add `videoSourceSiyiZr10` / Q_PROPERTY; enum list; `streamConfigured`; `usesSiyiRadioEthernet`.
- [ ] **Step 2:** VideoManager hasVideo lists + URI branch using `siyiA8MiniRtspUrl`.
- [ ] **Step 3:** `modelName()` / `_isSelectedVideoSource()` / overlay lookup / photo messages follow selected source; emit `infoChanged` on source change.
- [ ] **Step 4:** QGCCameraManager `isSiyiZr10VideoSource` / gimbal helper; `_syncPayloadCameraList` refreshes label; Unipod client `_canStart` includes ZR10.
- [ ] **Step 5:** QML modelName checks + zh_CN strings.
- [ ] **Step 6:** Rebuild camera tests + Android-debug if device work is next.

---

### Task 3: Verify

- [ ] Desktop `ctest -R 'CameraPayload|CameraGimbal'`
- [ ] `cmake --build build/Android-debug --target apk` if installing
