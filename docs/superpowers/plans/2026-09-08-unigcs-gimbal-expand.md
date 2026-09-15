# UniGCS Gimbal Expand (Four Quick Actions) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the 云台 D-pad expand with UniGCS’s four one-shot actions (回中 / 向下 / 偏航回中 / 俯仰朝下) in a second column beside the left payload bar.

**Architecture:** Capability flags + `Q_INVOKABLE` methods on `MavlinkCameraControlInterface`; SIYI-family cameras send `0x08` center modes via `UnipodMt11Client`; QML column of `FlyViewPayloadIconButton` replaces `TopotekGimbalPad` in `FlyViewPayloadOverlay`.

**Tech Stack:** Qt 6 QML/C++, `UnipodMt11Protocol`, existing Fly View payload overlay, CTest Unit.

**Spec:** `docs/superpowers/specs/2026-09-08-unigcs-gimbal-expand-design.md`

## Global Constraints

- Landscape remotes first; short height; glove touch ≥ `ScreenTools.minTouchPixels`.
- Null-check `_camera` / `activeVehicle` before use.
- No `Q_ASSERT` in production code.
- Capability false → button **hidden**, not greyed.
- No D-pad in the 云台 expand path.
- SIYI center modes: 1=回中, 4=向下, 3=偏航回中, 2=俯仰朝下 (sdk `center_pos`).
- Do **not** create git commits unless the user explicitly asks.
- Conventional Commits if committing later: `feat(Camera): …` / `feat(FlyView): …`.

## File map

| File | Role |
| --- | --- |
| `src/Camera/MavlinkCameraControlInterface.h` | Default `hasGimbal*` + invokable stubs |
| `src/Camera/UnipodMt11Client.h/.cc` | `ptzCenter(mode)` (or named helpers) |
| `src/Camera/UnipodMt11CameraControl.h/.cc` | Override has* + methods |
| `src/Camera/SiyiA8MiniCameraControl.h/.cc` | Same as UniPod (shared client) |
| `src/Camera/TopotekTq10CameraControl.h/.cc` | Only recenter → `ptzHome()` |
| `src/FlyView/FlyViewPayloadOverlay.qml` | Second-column UI; remove pad expand |
| `test/Camera/UnipodMt11ProtocolTest.*` | Assert center modes 2–4 frames |
| `test/Camera/CameraPayloadCapabilitiesTest.*` or new gimbal-actions test | Capability defaults + SIYI/Topotek |
| `translations/qgc_source_zh_CN.ts` | 回中 / 向下 / 偏航回中 / 俯仰朝下 |

---

### Task 1: Protocol tests for center modes 2–4

**Files:**

- Modify: `test/Camera/UnipodMt11ProtocolTest.cc`
- Modify: `test/Camera/UnipodMt11ProtocolTest.h` (only if adding a new slot; else extend `testGimbalFramesMatchHandbook`)

**Interfaces:**

- Consumes: `UnipodMt11Protocol::buildCenterCommand(quint16 seq, quint8 mode)`
- Produces: regression coverage for modes 1–4 hex frames

- [ ] **Step 1: Extend the failing assertion set**

In `testGimbalFramesMatchHandbook`, after the existing mode-1 check, add:

```cpp
    QCOMPARE(UnipodMt11Protocol::buildCenterCommand(0, 2).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 08 02 B2 22"));
    QCOMPARE(UnipodMt11Protocol::buildCenterCommand(0, 3).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 08 03 93 32"));
    QCOMPARE(UnipodMt11Protocol::buildCenterCommand(0, 4).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 08 04 74 42"));
```

- [ ] **Step 2: Run the unit test**

```bash
source ./env-qgc.sh
ctest --test-dir build -R UnipodMt11ProtocolTest --output-on-failure
```

Expected: PASS (builders already accept any `mode`; this locks SDK mapping).

- [ ] **Step 3: Skip commit** unless the user asked to commit.

---

### Task 2: Client API — send center modes 2–4

**Files:**

- Modify: `src/Camera/UnipodMt11Client.h`
- Modify: `src/Camera/UnipodMt11Client.cc`

**Interfaces:**

- Consumes: `UnipodMt11Protocol::buildCenterCommand`
- Produces: `void ptzCenter(quint8 mode);` — mode 1 keeps current `ptzHome()` behavior (call through or duplicate send)

- [ ] **Step 1: Declare API**

In `UnipodMt11Client.h`, after `ptzHome();`:

```cpp
    /// CMD 0x08 center_pos: 1=full home, 2=center+down, 3=center, 4=look down.
    void ptzCenter(quint8 mode);
```

- [ ] **Step 2: Implement**

In `UnipodMt11Client.cc`:

```cpp
void UnipodMt11Client::ptzCenter(quint8 mode)
{
    if (!isReady()) {
        return;
    }
    if (mode < 1 || mode > 4) {
        return;
    }
    (void) _sendDatagram(UnipodMt11Protocol::buildCenterCommand(++_seq, mode));
}

void UnipodMt11Client::ptzHome()
{
    ptzCenter(1);
}
```

(Replace the body of existing `ptzHome()` with the `ptzCenter(1)` call.)

- [ ] **Step 3: Build camera target**

```bash
source ./env-qgc.sh
cmake --build build --config Debug --parallel 8 --target QGroundControl
```

Expected: compile succeeds.

- [ ] **Step 4: Skip commit** unless requested.

---

### Task 3: Interface + camera overrides

**Files:**

- Modify: `src/Camera/MavlinkCameraControlInterface.h`
- Modify: `src/Camera/UnipodMt11CameraControl.h` / `.cc`
- Modify: `src/Camera/SiyiA8MiniCameraControl.h` / `.cc`
- Modify: `src/Camera/TopotekTq10CameraControl.h` / `.cc`
- Create or extend: `test/Camera/CameraGimbalQuickActionsTest.h` / `.cc`
- Modify: `test/Camera/CMakeLists.txt` (if new test file)

**Interfaces:**

- Consumes: `UnipodMt11Client::ptzCenter`, `TopotekTq10Client::ptzHome`
- Produces (on `MavlinkCameraControlInterface`):

```cpp
Q_PROPERTY(bool hasGimbalRecenter READ hasGimbalRecenter NOTIFY infoChanged)
Q_PROPERTY(bool hasGimbalLookDown READ hasGimbalLookDown NOTIFY infoChanged)
Q_PROPERTY(bool hasGimbalYawRecenter READ hasGimbalYawRecenter NOTIFY infoChanged)
Q_PROPERTY(bool hasGimbalPitchDown READ hasGimbalPitchDown NOTIFY infoChanged)

virtual bool hasGimbalRecenter() const { return false; }
virtual bool hasGimbalLookDown() const { return false; }
virtual bool hasGimbalYawRecenter() const { return false; }
virtual bool hasGimbalPitchDown() const { return false; }

Q_INVOKABLE virtual void gimbalRecenter() {}
Q_INVOKABLE virtual void gimbalLookDown() {}
Q_INVOKABLE virtual void gimbalYawRecenter() {}
Q_INVOKABLE virtual void gimbalPitchDown() {}
```

- SIYI / UniPod (when `hasGimbalPad()`): all four `has*` true; methods call `_client->ptzCenter(1|4|3|2)`.
- Topotek: only `hasGimbalRecenter()` true → `ptzHome()`; others false.

- [ ] **Step 1: Write failing capability test**

```cpp
// test/Camera/CameraGimbalQuickActionsTest.cc
#include "CameraGimbalQuickActionsTest.h"
#include "SimulatedCameraControl.h"
#include "TopotekTq10CameraControl.h"
#include "TopotekTq10Client.h"
#include "UnipodMt11CameraControl.h"
#include "UnipodMt11Client.h"

void CameraGimbalQuickActionsTest::testSimulatedDefaultsFalse()
{
    SimulatedCameraControl cam(nullptr, this);
    QCOMPARE(cam.hasGimbalRecenter(), false);
    QCOMPARE(cam.hasGimbalLookDown(), false);
    QCOMPARE(cam.hasGimbalYawRecenter(), false);
    QCOMPARE(cam.hasGimbalPitchDown(), false);
}

void CameraGimbalQuickActionsTest::testTopotekOnlyRecenter()
{
    auto* client = new TopotekTq10Client(this);
    TopotekTq10CameraControl cam(nullptr, client, this);
    QCOMPARE(cam.hasGimbalRecenter(), true);
    QCOMPARE(cam.hasGimbalLookDown(), false);
    QCOMPARE(cam.hasGimbalYawRecenter(), false);
    QCOMPARE(cam.hasGimbalPitchDown(), false);
}

void CameraGimbalQuickActionsTest::testUnipodHasAllFour()
{
    auto* client = new UnipodMt11Client(this);
    UnipodMt11CameraControl cam(nullptr, client, this);
    QVERIFY(cam.hasGimbalPad());
    QCOMPARE(cam.hasGimbalRecenter(), true);
    QCOMPARE(cam.hasGimbalLookDown(), true);
    QCOMPARE(cam.hasGimbalYawRecenter(), true);
    QCOMPARE(cam.hasGimbalPitchDown(), true);
}
```

Mirror registration pattern from `CameraPayloadCapabilitiesTest` / `UT_REGISTER_TEST`.

- [ ] **Step 2: Run test — expect FAIL** (symbols / overrides missing)

```bash
ctest --test-dir build -R CameraGimbalQuickActionsTest --output-on-failure
```

- [ ] **Step 3: Add interface defaults + overrides**

`UnipodMt11CameraControl` / `SiyiA8MiniCameraControl` (same pattern):

```cpp
bool UnipodMt11CameraControl::hasGimbalRecenter() const { return hasGimbalPad(); }
bool UnipodMt11CameraControl::hasGimbalLookDown() const { return hasGimbalPad(); }
bool UnipodMt11CameraControl::hasGimbalYawRecenter() const { return hasGimbalPad(); }
bool UnipodMt11CameraControl::hasGimbalPitchDown() const { return hasGimbalPad(); }

void UnipodMt11CameraControl::gimbalRecenter()
{
    if (_client && hasGimbalRecenter()) { _client->ptzCenter(1); }
}
void UnipodMt11CameraControl::gimbalLookDown()
{
    if (_client && hasGimbalLookDown()) { _client->ptzCenter(4); }
}
void UnipodMt11CameraControl::gimbalYawRecenter()
{
    if (_client && hasGimbalYawRecenter()) { _client->ptzCenter(3); }
}
void UnipodMt11CameraControl::gimbalPitchDown()
{
    if (_client && hasGimbalPitchDown()) { _client->ptzCenter(2); }
}
```

`TopotekTq10CameraControl`:

```cpp
bool TopotekTq10CameraControl::hasGimbalRecenter() const { return hasGimbalPad(); }
void TopotekTq10CameraControl::gimbalRecenter()
{
    if (_client && hasGimbalRecenter()) { _client->ptzHome(); }
}
```

- [ ] **Step 4: Re-run unit tests — expect PASS**

```bash
ctest --test-dir build -R 'CameraGimbalQuickActionsTest|UnipodMt11ProtocolTest' --output-on-failure
```

- [ ] **Step 5: Skip commit** unless requested.

---

### Task 4: QML second column UI

**Files:**

- Modify: `src/FlyView/FlyViewPayloadOverlay.qml`
- Modify: `translations/qgc_source_zh_CN.ts` (add contexts for new `qsTr` strings if missing)

**Interfaces:**

- Consumes: `_camera.hasGimbalRecenter` / `LookDown` / `YawRecenter` / `PitchDown` and matching invokables
- Produces: UniGCS-style expand column; no `TopotekGimbalPad` in expand host

- [ ] **Step 1: Replace `TopotekGimbalPad` block**

Remove:

```qml
        TopotekGimbalPad {
            anchors.left: parent.left
            anchors.top: parent.top
            visible: root.expand === FlyViewPayloadOverlay.Expand.Gimbal
                     && root._camera && root._camera.hasGimbalPad
            camera: root._camera
        }
```

Insert:

```qml
        Column {
            id: gimbalQuickCol
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: root._barSpacing
            visible: root.expand === FlyViewPayloadOverlay.Expand.Gimbal
                     && root._camera && root._camera.hasGimbalPad

            FlyViewPayloadIconButton {
                circleSize: leftBar.circleSize
                visible: root._camera && root._camera.hasGimbalRecenter
                iconSource: "/InstrumentValueIcons/gimbal-2.svg"
                label: qsTr("Recenter")
                onClicked: if (root._camera) { root._camera.gimbalRecenter() }
            }
            FlyViewPayloadIconButton {
                circleSize: leftBar.circleSize
                visible: root._camera && root._camera.hasGimbalLookDown
                iconSource: "/InstrumentValueIcons/gimbal-2.svg"
                label: qsTr("Down")
                onClicked: if (root._camera) { root._camera.gimbalLookDown() }
            }
            FlyViewPayloadIconButton {
                circleSize: leftBar.circleSize
                visible: root._camera && root._camera.hasGimbalYawRecenter
                iconSource: "/InstrumentValueIcons/gimbal-2.svg"
                label: qsTr("Yaw Recenter")
                onClicked: if (root._camera) { root._camera.gimbalYawRecenter() }
            }
            FlyViewPayloadIconButton {
                circleSize: leftBar.circleSize
                visible: root._camera && root._camera.hasGimbalPitchDown
                iconSource: "/InstrumentValueIcons/gimbal-2.svg"
                label: qsTr("Pitch Down")
                onClicked: if (root._camera) { root._camera.gimbalPitchDown() }
            }
        }
```

If better distinct icons exist under `resources/InstrumentValueIcons`, swap `iconSource` without changing labels.

- [ ] **Step 2: zh_CN translations**

Ensure `qgc_source_zh_CN.ts` maps:

| source | translation |
| --- | --- |
| Recenter | 回中 |
| Down | 向下 |
| Yaw Recenter | 偏航回中 |
| Pitch Down | 俯仰朝下 |

- [ ] **Step 3: Build Android (or Mac Debug) and install if device present**

```bash
source ./env-qgc.sh
cmake --build build/Android-debug -j8
adb -s 192.168.0.102:5555 install -r -d build/Android-debug/android-build/QGroundControl.apk
```

- [ ] **Step 4: Manual check on remote**

1. Video main → tap 云台 → second column appears (SIYI: four buttons; Topotek: 回中 only).
2. Each button sends action; no D-pad visible.
3. Re-tap 云台 or tap video → collapse.
4. Column does not cover PIP/compass on short landscape.

- [ ] **Step 5: Skip commit** unless requested.

---

### Task 5: Spec coverage self-check

- [ ] Confirm each locked decision in the spec has a task:

| Spec item | Task |
| --- | --- |
| Four keys only / no D-pad | Task 4 |
| Second column beside left bar | Task 4 |
| Hide unsupported | Tasks 3–4 |
| SIYI modes 1/4/3/2 | Tasks 1–3 |
| Topotek recenter only | Task 3 |
| Collapse existing | Task 4 (unchanged MouseArea) |

- [ ] Run `just lint` or targeted qmllint if available after QML edits.

---

## Execution handoff

Plan complete and saved to `docs/superpowers/plans/2026-09-08-unigcs-gimbal-expand.md`.

Spec: `docs/superpowers/specs/2026-09-08-unigcs-gimbal-expand-design.md`

**Two execution options:**

1. **Subagent-Driven (recommended)** — fresh subagent per task, review between tasks  
2. **Inline Execution** — run tasks in this session with checkpoints  

Which approach?
