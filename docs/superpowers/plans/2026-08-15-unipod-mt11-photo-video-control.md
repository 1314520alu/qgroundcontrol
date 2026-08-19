# UniPod MT11 Photo / Video Control — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** When video source is UniPod MT11 and radio ethernet is ready, Fly view red-circle photo/record buttons trigger onboard TF capture via UDP SDK (`0x0C`), with recording state from `0x0A`/`0x0B`.

**Architecture:** Add `UnipodMt11Protocol` (frame/CRC), `UnipodMt11Client` (`QUdpSocket` to `192.168.144.25:37260`), and `UnipodMt11CameraControl` (`MavlinkCameraControlInterface`). When `VideoSettings` source is UniPod MT11, `QGCCameraManager` prefers this control so `PhotoVideoControl.qml` needs no UI rewrite.

**Tech Stack:** Qt 6 (`QUdpSocket`, `QTimer`), existing Camera module + Fact/VideoSettings, GTest via `add_qgc_test`, command authority `docs/manuals/unipod-mt11/db/sdk.db`

**Spec:** `docs/superpowers/specs/2026-08-15-unipod-mt11-photo-video-control-design.md`

## Global Constraints

- Transport: UDP only, host `192.168.144.25`, port `37260`
- Phase 1 commands only: `0x0C` (photo=0, record toggle=2), `0x0A` poll, `0x0B` push parse
- Onboard TF recording — never call `VideoManager::startRecording()` for MT11 strip
- `hasModes() == false` Phase 1
- Prefer UniPod camera control only while `videoSource == UniPod MT11`
- Do not start client without MT11 video source + `192.168.144.x` ready
- Match CODING_STYLE.md; no `Q_ASSERT` in production paths
- Conventional Commits if committing; **commit only when the user asks**
- Landscape remotes first (no new stacked UI)

## File map

| File | Role |
| --- | --- |
| `src/Camera/UnipodMt11Protocol.h/.cc` | Pack/unpack frames + CRC16-CCITT (`0x1021`, init 0) |
| `src/Camera/UnipodMt11Client.h/.cc` | UDP I/O, poll `0x0A`, emit record/photo feedback |
| `src/Camera/UnipodMt11CameraControl.h/.cc` | `MavlinkCameraControlInterface` adapter |
| `src/Camera/QGCCameraManager.h/.cc` | Own/prefer UniPod control when MT11 source active |
| `src/Camera/CMakeLists.txt` | Register new sources |
| `test/Camera/UnipodMt11ProtocolTest.cc/.h` | Unit tests for CRC + frames |
| `test/Camera/CMakeLists.txt` | Register unit test |

---

### Task 1: Protocol pack/unpack + CRC unit tests

**Files:**
- Create: `src/Camera/UnipodMt11Protocol.h`
- Create: `src/Camera/UnipodMt11Protocol.cc`
- Create: `test/Camera/UnipodMt11ProtocolTest.h`
- Create: `test/Camera/UnipodMt11ProtocolTest.cc`
- Modify: `src/Camera/CMakeLists.txt`
- Modify: `test/Camera/CMakeLists.txt`

**Interfaces:**
- Consumes: none
- Produces:
  - `namespace UnipodMt11Protocol` with:
    - `static constexpr quint16 kDefaultPort = 37260;`
    - `static constexpr const char* kDefaultHost = "192.168.144.25";`
    - `quint16 crc16(const QByteArray &data);` — CRC over bytes given (init 0)
    - `QByteArray buildFrame(quint8 ctrl, quint16 seq, quint8 cmdId, const QByteArray &payload);`
    - `bool parseFrame(const QByteArray &datagram, quint8 *ctrlOut, quint16 *seqOut, quint8 *cmdOut, QByteArray *payloadOut);`
    - `QByteArray buildPhotoCommand(quint16 seq);` — `0x0C` + `0x00`
    - `QByteArray buildRecordToggleCommand(quint16 seq);` — `0x0C` + `0x02`
    - `QByteArray buildSystemInfoRequest(quint16 seq);` — `0x0A` empty payload, `ctrl` need_ack=1
    - `struct SystemInfoAck { quint8 recordSta = 0; /* other fields optional Phase 1 */ };`
    - `bool parseSystemInfoAck(const QByteArray &payload, SystemInfoAck *out);`
    - `enum class FuncFeedback : quint8 { PhotoOk=0, PhotoFailNoCard=1, ..., RecordStart=5, RecordEnd=6 };` (values per SDK)
    - `bool parseFuncFeedback(const QByteArray &payload, quint8 *infoTypeOut);`

- [ ] **Step 1: Write failing unit test**

```cpp
// test/Camera/UnipodMt11ProtocolTest.cc
#include "UnipodMt11ProtocolTest.h"
#include "UnipodMt11Protocol.h"

void UnipodMt11ProtocolTest::testPhotoFrameMatchesHandbook()
{
    const QByteArray frame = UnipodMt11Protocol::buildPhotoCommand(0);
    QCOMPARE(frame.toHex(' ').toUpper(), QByteArray("55 66 01 01 00 00 00 0C 00 34 CE"));
}

void UnipodMt11ProtocolTest::testRecordFrameMatchesHandbook()
{
    const QByteArray frame = UnipodMt11Protocol::buildRecordToggleCommand(0);
    QCOMPARE(frame.toHex(' ').toUpper(), QByteArray("55 66 01 01 00 00 00 0C 02 76 EE"));
}

void UnipodMt11ProtocolTest::testSystemInfoRequestMatchesHandbook()
{
    const QByteArray frame = UnipodMt11Protocol::buildSystemInfoRequest(0);
    QCOMPARE(frame.toHex(' ').toUpper(), QByteArray("55 66 01 00 00 00 00 0A 0F 75"));
}

void UnipodMt11ProtocolTest::testParseRoundTrip()
{
    const QByteArray frame = UnipodMt11Protocol::buildPhotoCommand(7);
    quint8 ctrl = 0, cmd = 0;
    quint16 seq = 0;
    QByteArray payload;
    QVERIFY(UnipodMt11Protocol::parseFrame(frame, &ctrl, &seq, &cmd, &payload));
    QCOMPARE(cmd, quint8(0x0C));
    QCOMPARE(seq, quint16(7));
    QCOMPARE(payload, QByteArray(1, char(0x00)));
}
```

Header: `QGCTestCase` subclass with `Q_OBJECT` and slots matching other Camera tests (`VehicleCameraControlTest` pattern).

- [ ] **Step 2: Run test — expect FAIL (missing symbols)**

```bash
just build
ctest -R UnipodMt11ProtocolTest --output-on-failure
```

Expected: link/compile failure or FAIL until implementation exists.

- [ ] **Step 3: Implement `UnipodMt11Protocol`**

CRC: table-driven or bit algorithm matching SDK (`G(X)=X^16+X^12+X^5+1`, init 0). Verify photo/record/system examples from handbook before finishing.

Frame layout (LE): `STX 0x6655` → `CTRL` → `Data_len` u16 → `SEQ` u16 → `CMD_ID` → `DATA` → `CRC16` u16 over all preceding bytes.

`CTRL` for requests that need ACK (`0x0A`): `0x01` (need_ack). For `0x0C` examples use `0x01` as in handbook hex (need_ack bit set even though no ACK defined — match handbook bytes exactly).

`parseSystemInfoAck`: payload layout from SDK handbook / `sdk.db` `command_fields` for `0x0A` ACK — at minimum read `record_sta` at the documented index (field 4 / 0-based offset after reserved/hdr — use `sdk.db` + handbook; assert in a test with a crafted payload).

- [ ] **Step 4: Register sources and test in CMake**

Add the three `.cc` files under Camera (protocol only this task) and:

```cmake
add_qgc_test(UnipodMt11ProtocolTest LABELS Unit Camera)
```

- [ ] **Step 5: Run tests — expect PASS**

```bash
just build
ctest -R UnipodMt11ProtocolTest --output-on-failure
```

Expected: all four cases PASS.

- [ ] **Step 6: Commit only if user asks**

```bash
git add src/Camera/UnipodMt11Protocol.* test/Camera/UnipodMt11ProtocolTest.* src/Camera/CMakeLists.txt test/Camera/CMakeLists.txt
# git commit only when requested
```

---

### Task 2: `UnipodMt11Client` UDP + status signals

**Files:**
- Create: `src/Camera/UnipodMt11Client.h`
- Create: `src/Camera/UnipodMt11Client.cc`
- Modify: `src/Camera/CMakeLists.txt`

**Interfaces:**
- Consumes: `UnipodMt11Protocol::*`, `ScreenToolsController::siyiRadioEthernetAddress()` (or existing VideoManager ethernet-ready helper if exposed — prefer reuse of the same readiness used by UniPod RTSP gate), `VideoSettings::videoSourceUnipodMT11`
- Produces: `class UnipodMt11Client : public QObject` with:
  - `explicit UnipodMt11Client(QObject *parent = nullptr);`
  - `bool isReady() const;` — socket open + host reachable path (ethernet present)
  - `Q_INVOKABLE void start();` / `void stop();`
  - `void takePhoto();`
  - `void toggleRecording();`
  - `quint8 recordSta() const;` — last known `0..3`
  - Signals: `readyChanged()`, `recordStaChanged(quint8)`, `funcFeedback(quint8 infoType)`, `sendFailed(const QString &reason)`

- [ ] **Step 1: Implement client skeleton**

- Own `QUdpSocket`, `QTimer` poll 1000 ms for `0x0A`.
- `start()`: if already running return; bind any local port; start poll timer; bump seq on each send.
- `stop()`: stop timer; close socket; clear ready.
- On `readyRead`: read datagrams; `parseFrame`; if `cmd==0x0A` parse ACK → update `recordSta`; if `cmd==0x0B` emit `funcFeedback`.
- `takePhoto` / `toggleRecording`: if `!isReady()` emit `sendFailed` and return; else `writeDatagram` to `kDefaultHost:kDefaultPort`.

- [ ] **Step 2: Wire readiness**

Start only when:
1. `VideoSettings::videoSource()` equals `videoSourceUnipodMT11`, and
2. SIYI/radio ethernet address is non-empty / `192.168.144.x` present (same condition VideoManager uses for UniPod gate success).

Expose a slot `void setActive(bool)` called from camera control / manager so the client does not independently subscribe to every settings change if manager owns lifecycle — **prefer manager-driven `start/stop`** to avoid duplicate timers.

- [ ] **Step 3: Logging**

`QGC_LOGGING_CATEGORY(UnipodMt11ClientLog, "Camera.UnipodMt11Client")` — log send hex at debug, failures at warning.

- [ ] **Step 4: Build**

```bash
just build
```

Expected: compiles; no new test required beyond protocol (UDP needs device / can add a later mock).

- [ ] **Step 5: Commit only if user asks**

---

### Task 3: `UnipodMt11CameraControl` adapter

**Files:**
- Create: `src/Camera/UnipodMt11CameraControl.h`
- Create: `src/Camera/UnipodMt11CameraControl.cc`
- Modify: `src/Camera/CMakeLists.txt`

**Interfaces:**
- Consumes: `UnipodMt11Client`, `VideoManager::instance()` for `hasVideo`/`decoding`, `Vehicle*`
- Produces: `class UnipodMt11CameraControl : public MavlinkCameraControlInterface` implementing the same stub pattern as `SimulatedCameraControl` for unused APIs, with real behavior for:
  - `modelName()` → `"UniPod MT11"`
  - `vendor()` → `"Reebot"`
  - `capturesPhotos()` / `capturesVideo()` → `client && client->isReady()`
  - `hasModes()` → `false`
  - `hasVideoStream()` → `VideoManager::instance()->decoding()`
  - `takePhoto()` → `client->takePhoto()`; brief local photo-in-progress then idle (or idle immediately)
  - `toggleVideoRecording()` / `start` / `stop` → `client->toggleRecording()` (stop also sends toggle — SDK is toggle-only)
  - `captureVideoState()` → Disabled if not ready / no card (`recordSta==2`); Capturing if `recordSta==1` or `3`; else Idle
  - `capturePhotosState()` → Disabled if not ready; else Idle (or CapturingSingle while local brief lock)
  - `recordTime()` / `recordTimeStr()` → local `QElapsedTimer` while Capturing (SDK has no record-time field in Phase 1)

- [ ] **Step 1: Copy stub surface from `SimulatedCameraControl`**

Reuse the same no-op overrides for zoom/focus/streams/thermal/params so the class links. Do **not** call `VideoManager::startRecording` / `triggerSimpleCamera`.

- [ ] **Step 2: Connect client signals**

```cpp
connect(_client, &UnipodMt11Client::recordStaChanged, this, [this](quint8) {
    emit captureVideoStateChanged();
    emit recordTimeChanged();
});
connect(_client, &UnipodMt11Client::readyChanged, this, &UnipodMt11CameraControl::infoChanged);
connect(_client, &UnipodMt11Client::funcFeedback, this, /* optional AppMessages for no-card / photo fail */);
```

On `recordSta==2`, `QGC::showAppMessage(tr("UniPod MT11: no storage card"));` (or equivalent existing toast helper).

- [ ] **Step 3: Build**

```bash
just build
```

Expected: PASS compile.

- [ ] **Step 4: Commit only if user asks**

---

### Task 4: `QGCCameraManager` inject / prefer UniPod control

**Files:**
- Modify: `src/Camera/QGCCameraManager.h`
- Modify: `src/Camera/QGCCameraManager.cc`

**Interfaces:**
- Consumes: `UnipodMt11CameraControl`, `UnipodMt11Client`, `SettingsManager::videoSettings()->videoSource()`, `VideoManager` ethernet-ready signals if available
- Produces: When MT11 source active and ready, `currentCameraInstance()` used by Fly UI is the UniPod control

- [ ] **Step 1: Own UniPod objects on the manager**

```cpp
// members
UnipodMt11Client *_unipodClient = nullptr;
UnipodMt11CameraControl *_unipodCameraControl = nullptr;
```

Construct as children of the manager (alongside `_simulatedCameraControl`). Do **not** add UniPod to `_cameras` until active (or add and remove — pick one strategy and stick to it).

**Recommended strategy (simple):**

1. Always construct client + control (inactive).
2. On video source / ethernet change:
   - If MT11 + ready: `_unipodClient->start()`; ensure control is in `_cameras` / `_cameraLabels`; `setCurrentCamera` to its index (or force `currentCameraInstance()` override when MT11 active).
   - Else: `_unipodClient->stop()`; remove UniPod from list if present; restore previous current index if needed.

**Avoid:** leaving Simulated DIGICAM as current while MT11 source is selected.

Override approach alternative (even simpler for Phase 1):

```cpp
MavlinkCameraControlInterface *QGCCameraManager::currentCameraInstance()
{
    if (_unipodCameraControl && _unipodCameraControl->capturesPhotos()) {
        return _unipodCameraControl;
    }
    // existing index-based logic
}
```

Use this if list inject fights with MAVLink cameras. Document choice in code comment. Prefer **override `currentCameraInstance()`** for Phase 1 minimal churn.

- [ ] **Step 2: Subscribe to video source Fact**

```cpp
connect(SettingsManager::instance()->videoSettings()->videoSource(), &Fact::rawValueChanged,
        this, &QGCCameraManager::_unipodVideoSourceChanged);
```

Also re-evaluate when ethernet address becomes available (connect to the same notifier VideoManager uses, or poll lightly via existing SIYI helper signal if one exists). Reuse `VideoManager` signals (`hasVideoChanged` is insufficient alone — need ethernet). If no signal, reconnect on `VideoManager`’s UniPod gate success path by adding a thin `ethernetReadyChanged` signal in Task 4 only if required — **YAGNI**: call `_syncUnipodCamera()` from a 1s timer only while source is MT11 and not ready, max 60s, then stop (mirrors gate timeout).

- [ ] **Step 3: Implement `_syncUnipodCamera()`**

- Start/stop client
- Emit `currentCameraChanged()` / `infoChanged` as needed so QML refreshes

- [ ] **Step 4: Build + smoke**

```bash
just build
ctest -R UnipodMt11ProtocolTest --output-on-failure
```

Expected: PASS.

- [ ] **Step 5: Commit only if user asks**

---

### Task 5: Manual HIL checklist + docs touch

**Files:**
- Modify: `docs/manuals/unipod-mt11/UniPod-MT11-SDK-Handbook.md` (short “QGC Phase 1” note under §6)
- Modify: `docs/superpowers/specs/2026-08-15-unipod-mt11-photo-video-control-design.md` — set Status to `Approved — implementing` then `Implemented` when done

- [ ] **Step 1: Update handbook §6**

Add bullets: Phase 1 Fly controls send `0x0C`; recording state from `0x0A`; requires UniPod video source + `192.168.144.x`.

- [ ] **Step 2: Device checklist (operator)**

On SIYI remote with MT11 + TF card:

1. Select video source UniPod MT11 → confirm RTSP video.
2. Open Fly photo/video strip → photo → file appears on TF (or UniGCS media).
3. Record → button shows recording; stop → file on TF; `record_sta` coherent.
4. Remove TF → record disabled / message.
5. Switch video source away → no further UDP; Simulated/MAVLink behavior restored.

- [ ] **Step 3: Final verify**

```bash
just build
ctest -R UnipodMt11ProtocolTest --output-on-failure
just lint   # if feasible for touched paths
```

- [ ] **Step 4: Commit only if user asks**

---

## Spec coverage check

| Spec requirement | Task |
| --- | --- |
| UDP `192.168.144.25:37260` | 1–2 |
| `0x0C` photo / record | 1–3 |
| `0x0A` / `0x0B` status | 2–3 |
| Reuse `PhotoVideoControl` | 3–4 |
| Prefer when MT11 source | 4 |
| No local GST as onboard | 3 |
| No TCP / zoom / AI | omitted |
| Unit CRC tests | 1 |
| HIL validation | 5 |

## Placeholder / consistency self-review

- No TBD steps; handbook example CRCs locked: photo `34 CE`, record `76 EE`, `0x0A` req `0F 75`.
- `currentCameraInstance` override vs list inject: Task 4 recommends override for Phase 1.
- Client lifecycle owned by manager via `_syncUnipodCamera()`.
