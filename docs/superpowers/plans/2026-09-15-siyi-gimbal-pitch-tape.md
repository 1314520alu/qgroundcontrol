# SIYI Gimbal Pitch Tape Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show a DJI-style scrolling pitch tape on Fly View for SIYI ZR10 / A8 Mini / UniPod while gimbal pitch changes, driven by SDK CMD `0x0D`.

**Architecture:** Parse gimbal attitude in `UnipodMt11Protocol`, poll `0x0D` on the existing SIYI UDP zoom timer in `UnipodMt11Client`, own HUD visibility in `QGCCameraManager` (same as zoom), draw the tape in new `GimbalPitchTape.qml` hosted by `FlyViewPayloadOverlay`.

**Tech Stack:** Qt6 C++20, QML, existing SIYI UDP client (`192.168.144.25:37260`), QTest / CTest.

**Spec:** `docs/superpowers/specs/2026-09-15-siyi-gimbal-pitch-tape-design.md`

## Global Constraints

- Pitch only. No yaw/roll tape. No CMD `0x25`. No MAVLink `GimbalController` / toolbar gimbal. No Topotek tape.
- Do not change zoom HUD: `0x18` still polls every tick; C++ still owns `siyiZoomHud*`.
- Poll `0x0D` on the same 150 ms / 80 ms timer as `0x18` (80 ms when zoom hold **or** overlay PTZ pitch hold).
- First finite pitch sample primes only (no HUD flash). Later show when `qRound(pitch)` changes. Linger 5000 ms.
- Display `tr("%1°").arg(qRound(pitch))` with no space (`-27°`). Sign: positive = look up, 0° = horizon.
- Tape sits left of `rightBar`, above `bottomReserve`. Display-only (not a touch target).
- QML HUD copies **signal arguments**, never re-reads C++ properties inside `Connections` handlers.
- 0° is a valid pitch — unprimed state must be **NaN**, not `0`.
- Landscape remotes first (Skydroid G20 / SIYI MK32). `CODING_STYLE.md`: 4 spaces, no `Q_ASSERT`, no `QTest::qWait`.
- Do **not** commit unless the user explicitly asks. Skip each Commit step until then.
- Do **not** mix XF200 tethered-power work into this change.

## File map

| File | Role |
| --- | --- |
| `src/Camera/UnipodMt11Protocol.h/.cc` | `0x0D` request + ACK parse |
| `test/Camera/UnipodMt11ProtocolTest.h/.cc` | Protocol unit tests |
| `src/Camera/UnipodMt11Client.h/.cc` | Poll `0x0D`, `pitchDegrees` |
| `src/Camera/QGCCameraManager.h/.cc` | C++-owned pitch HUD |
| `test/Camera/QGCCameraManagerTest.h/.cc` | Prime / show / hide / zoom still works |
| `src/FlyView/GimbalPitchTape.qml` | Scrolling tape UI |
| `src/FlyView/CMakeLists.txt` | Register QML |
| `src/FlyView/FlyViewPayloadOverlay.qml` | Host tape + signal copy |

---

### Task 1: CMD `0x0D` protocol

**Files:**
- Modify: `src/Camera/UnipodMt11Protocol.h`
- Modify: `src/Camera/UnipodMt11Protocol.cc`
- Modify: `test/Camera/UnipodMt11ProtocolTest.h`
- Modify: `test/Camera/UnipodMt11ProtocolTest.cc`

**Interfaces:**
- Consumes: existing `buildFrame`, little-endian `int16` `/ 10.0` like other SIYI ACKs
- Produces:
  - `QByteArray buildGimbalAttitudeRequest(quint16 seq);`
  - `bool parseGimbalAttitudeAck(const QByteArray& payload, double* pitchDegOut);`

- [ ] **Step 1: Write the failing tests**

In `test/Camera/UnipodMt11ProtocolTest.h`, add slots next to `testGimbalFramesMatchHandbook`:

```cpp
    void testGimbalAttitudeRequestMatchesHandbook();
    void testParseGimbalAttitudeAck();
```

In `test/Camera/UnipodMt11ProtocolTest.cc`, after `testGimbalFramesMatchHandbook`:

```cpp
void UnipodMt11ProtocolTest::testGimbalAttitudeRequestMatchesHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildGimbalAttitudeRequest(0).toHex(' ').toUpper(),
             QByteArray("55 66 01 00 00 00 00 0D E8 05"));
}

void UnipodMt11ProtocolTest::testParseGimbalAttitudeAck()
{
    double pitch = 0.0;
    // yaw=0, pitch=-270 (−27.0°), roll=0, three velocities=0
    const QByteArray twelve = QByteArray::fromHex("0000f4fe0000000000000000");
    QVERIFY(UnipodMt11Protocol::parseGimbalAttitudeAck(twelve, &pitch));
    QCOMPARE(pitch, -27.0);

    const QByteArray four = QByteArray::fromHex("0000f4fe");
    QVERIFY(UnipodMt11Protocol::parseGimbalAttitudeAck(four, &pitch));
    QCOMPARE(pitch, -27.0);

    const QByteArray six = QByteArray::fromHex("0000f4fe0000");
    QVERIFY(UnipodMt11Protocol::parseGimbalAttitudeAck(six, &pitch));
    QCOMPARE(pitch, -27.0);

    QVERIFY(!UnipodMt11Protocol::parseGimbalAttitudeAck(QByteArray::fromHex("0000f4"), &pitch));
    QVERIFY(!UnipodMt11Protocol::parseGimbalAttitudeAck(QByteArray(), &pitch));
    QVERIFY(!UnipodMt11Protocol::parseGimbalAttitudeAck(twelve, nullptr));
}
```

- [ ] **Step 2: Run tests — expect FAIL**

```bash
cmake --build build --target QGroundControl --parallel
ctest --preset default --output-on-failure -R UnipodMt11ProtocolTest
```

Expected: compile error `buildGimbalAttitudeRequest` / `parseGimbalAttitudeAck` not declared.

- [ ] **Step 3: Implement protocol**

In `src/Camera/UnipodMt11Protocol.h`, after `buildCurrentZoomRequest` / `parseCurrentZoomAck`:

```cpp
/// Request gimbal attitude (CMD 0x0D). Empty payload.
QByteArray buildGimbalAttitudeRequest(quint16 seq);
/// Parse CMD 0x0D ACK: little-endian int16 at offset 2, / 10.0 = pitch degrees.
/// Needs at least 4 bytes (yaw + pitch). Velocities optional.
bool parseGimbalAttitudeAck(const QByteArray& payload, double* pitchDegOut);
```

In `src/Camera/UnipodMt11Protocol.cc`, next to other builders (after `buildCurrentZoomRequest`):

```cpp
QByteArray buildGimbalAttitudeRequest(quint16 seq)
{
    return buildFrame(0x01, seq, 0x0D, QByteArray());
}

bool parseGimbalAttitudeAck(const QByteArray& payload, double* pitchDegOut)
{
    if (!pitchDegOut || payload.size() < 4) {
        return false;
    }

    qint16 pitchRaw = 0;
    memcpy(&pitchRaw, payload.constData() + 2, sizeof(pitchRaw));
    *pitchDegOut = static_cast<double>(qFromLittleEndian(pitchRaw)) / 10.0;
    return true;
}
```

`#include <QtCore/QtEndian>` and `<cstring>` are already in this file.

- [ ] **Step 4: Re-run tests — expect PASS**

```bash
cmake --build build --target QGroundControl --parallel
ctest --preset default --output-on-failure -R UnipodMt11ProtocolTest
```

Expected: `UnipodMt11ProtocolTest` 100% passed. Existing zoom/gimbal frame tests still pass.

- [ ] **Step 5: Commit** (skip unless the user asked)

```bash
git add src/Camera/UnipodMt11Protocol.h src/Camera/UnipodMt11Protocol.cc \
        test/Camera/UnipodMt11ProtocolTest.h test/Camera/UnipodMt11ProtocolTest.cc
git commit -m "$(cat <<'EOF'
feat(Camera): parse SIYI CMD 0x0D gimbal pitch

EOF
)"
```

---

### Task 2: UDP client poll + `pitchDegrees`

**Files:**
- Modify: `src/Camera/UnipodMt11Client.h`
- Modify: `src/Camera/UnipodMt11Client.cc`

**Interfaces:**
- Consumes: `buildGimbalAttitudeRequest`, `parseGimbalAttitudeAck`
- Produces:
  - `double pitchDegrees() const` — NaN until first good ACK
  - `void requestGimbalAttitude()`
  - `Q_PROPERTY(double pitchDegrees READ pitchDegrees NOTIFY pitchDegreesChanged)`
  - `void pitchDegreesChanged()`
  - Friend test helper `void _setPitchDegrees(double pitchDegrees)` (already friend `QGCCameraManagerTest`)

- [ ] **Step 1: Extend the client header**

In `src/Camera/UnipodMt11Client.h`:

- Add property under `zoomLevel`:

```cpp
    Q_PROPERTY(double pitchDegrees READ pitchDegrees NOTIFY pitchDegreesChanged)
```

- Add after `requestCurrentZoom()`:

```cpp
    void requestGimbalAttitude();
```

- Add after `zoomLevel()`:

```cpp
    /// Last gimbal pitch in degrees; NaN if unknown.
    double pitchDegrees() const { return _pitchDegrees; }
```

- Add signal `void pitchDegreesChanged();` next to `zoomLevelChanged()`.

- In private methods, add:

```cpp
    void _setPitchDegrees(double pitchDegrees);
    void _updateSdkPollInterval();
```

Keep `_setPitchDegrees` private — `QGCCameraManagerTest` is already a friend.

- Add members:

```cpp
    bool _ptzPitchHold = false;
    double _pitchDegrees = qQNaN();
```

(`QtNumeric` is already included.)

- [ ] **Step 2: Implement poll, parse, interval, reset**

`requestGimbalAttitude()` — same shape as `requestCurrentZoom()`:

```cpp
void UnipodMt11Client::requestGimbalAttitude()
{
    if (!isReady()) {
        return;
    }

    (void) _sendDatagram(UnipodMt11Protocol::buildGimbalAttitudeRequest(++_seq));
}
```

In `start()`, after `requestCurrentZoom();` add `requestGimbalAttitude();`.

In `stop()`, after `_setRecordSta(0);` add `_ptzPitchHold = false;` and `_setPitchDegrees(qQNaN());`.

Replace interval writes in `startZoom` / `_stopZoomHold` with `_updateSdkPollInterval()`:

```cpp
void UnipodMt11Client::_updateSdkPollInterval()
{
    if (!_zoomHoldTimer) {
        return;
    }
    const bool hold = _zoomHoldDirection != 0 || _ptzPitchHold;
    _zoomHoldTimer->setInterval(hold ? UnipodMt11Protocol::kCurrentZoomHoldPollIntervalMs
                                     : UnipodMt11Protocol::kCurrentZoomPollIntervalMs);
}
```

`startZoom`: after setting `_zoomHoldDirection`, call `_updateSdkPollInterval()` instead of `setInterval(kCurrentZoomHoldPollIntervalMs)`.

`_stopZoomHold`: do **not** blindly force 150 ms (PTZ pitch may still be held). Clear zoom hold flags, then `_updateSdkPollInterval()`.

`ptzStart`: after a successful speed command, set `_ptzPitchHold = (pitch != 0);` then `_updateSdkPollInterval();`. Do not treat yaw-only (left/right) as a pitch hold.

`ptzStop`: `_ptzPitchHold = false;` then `_updateSdkPollInterval();` then send `0x07` stop as today.

`_onZoomHoldTimeout`: keep the existing `0x18` request, then **always** `requestGimbalAttitude();`.

`_onReadyRead`: add `cmd == 0x0D` next to `0x18`:

```cpp
            } else if (cmd == 0x0D) {
                double pitch = 0.0;
                if (UnipodMt11Protocol::parseGimbalAttitudeAck(payload, &pitch)) {
                    _setPitchDegrees(pitch);
                }
```

`_setPitchDegrees`:

```cpp
void UnipodMt11Client::_setPitchDegrees(double pitchDegrees)
{
    if (qIsNaN(pitchDegrees)) {
        if (qIsNaN(_pitchDegrees)) {
            return;
        }
        _pitchDegrees = qQNaN();
        emit pitchDegreesChanged();
        return;
    }
    if (!qIsNaN(_pitchDegrees) && qAbs(_pitchDegrees - pitchDegrees) < 0.05) {
        return;
    }
    _pitchDegrees = pitchDegrees;
    emit pitchDegreesChanged();
}
```

- [ ] **Step 3: Build**

```bash
cmake --build build --target QGroundControl --parallel
```

Expected: compile succeeds. No new unit test in this task (no UDP mock). Task 3 covers HUD via `_setPitchDegrees`.

- [ ] **Step 4: Commit** (skip unless the user asked)

```bash
git add src/Camera/UnipodMt11Client.h src/Camera/UnipodMt11Client.cc
git commit -m "$(cat <<'EOF'
feat(Camera): poll SIYI gimbal pitch on the zoom UDP timer

EOF
)"
```

---

### Task 3: C++-owned pitch HUD

**Files:**
- Modify: `src/Camera/QGCCameraManager.h`
- Modify: `src/Camera/QGCCameraManager.cc`
- Modify: `test/Camera/QGCCameraManagerTest.h`
- Modify: `test/Camera/QGCCameraManagerTest.cc`

**Interfaces:**
- Consumes: `UnipodMt11Client::pitchDegrees()` / `pitchDegreesChanged` / `_setPitchDegrees`
- Produces:
  - `Q_PROPERTY(qreal siyiPitchDegrees READ siyiPitchDegrees NOTIFY siyiPitchDegreesChanged)`
  - `Q_PROPERTY(bool siyiPitchHudVisible READ siyiPitchHudVisible NOTIFY siyiPitchHudVisibleChanged)`
  - `Q_PROPERTY(QString siyiPitchHudText READ siyiPitchHudText NOTIFY siyiPitchHudTextChanged)`
  - `Q_INVOKABLE void showSiyiPitchHud();`
  - Signals pass the **new value** as the first argument (QML cache workaround)

- [ ] **Step 1: Write the failing manager test**

In `QGCCameraManagerTest.h` add:

```cpp
    void _testSiyiPitchHudFollowsUdpClient();
```

In `QGCCameraManagerTest.cc` after `_testSiyiZoomLevelFollowsUdpClient`, include `<QtCore/QtNumeric>` if needed:

```cpp
void QGCCameraManagerTest::_testSiyiPitchHudFollowsUdpClient()
{
    QGCCameraManager* cameraManager = vehicle()->cameraManager();
    QVERIFY(cameraManager);
    UnipodMt11Client* client = cameraManager->unipodClient();
    QVERIFY(client);

    QVERIFY(qIsNaN(cameraManager->siyiPitchDegrees()));
    QVERIFY(!cameraManager->siyiPitchHudVisible());
    QVERIFY(cameraManager->siyiPitchHudText().isEmpty());

    client->_setPitchDegrees(-27.0);
    QCOMPARE(cameraManager->siyiPitchDegrees(), -27.0);
    QVERIFY(!cameraManager->siyiPitchHudVisible());
    QCOMPARE(cameraManager->siyiPitchHudText(), QStringLiteral("-27°"));

    client->_setPitchDegrees(-27.4);
    QVERIFY(!cameraManager->siyiPitchHudVisible());

    client->_setPitchDegrees(-26.0);
    QVERIFY(cameraManager->siyiPitchHudVisible());
    QCOMPARE(cameraManager->siyiPitchHudText(), QStringLiteral("-26°"));

    cameraManager->_hideSiyiPitchHud();
    QVERIFY(!cameraManager->siyiPitchHudVisible());

    cameraManager->showSiyiPitchHud();
    QVERIFY(cameraManager->siyiPitchHudVisible());

    client->_setPitchDegrees(qQNaN());
    QVERIFY(qIsNaN(cameraManager->siyiPitchDegrees()));
    QVERIFY(!cameraManager->siyiPitchHudVisible());
}
```

Keep `_testSiyiZoomLevelFollowsUdpClient` unchanged.

- [ ] **Step 2: Run test — expect FAIL**

```bash
cmake --build build --target QGroundControl --parallel
ctest --preset default --output-on-failure -R QGCCameraManagerTest
```

Expected: compile error (`siyiPitchDegrees` / `_hideSiyiPitchHud` missing).

If the **link** fails on unrelated `Xf200TetheredPowerVisual` symbols, do not “fix” XF200 here. Finish protocol tests; report the link error. HUD logic can still be implemented to the code below.

- [ ] **Step 3: Implement manager HUD**

`QGCCameraManager.h`:

- Next to zoom HUD `Q_PROPERTY`s:

```cpp
    Q_PROPERTY(qreal siyiPitchDegrees READ siyiPitchDegrees NOTIFY siyiPitchDegreesChanged)
    Q_PROPERTY(bool siyiPitchHudVisible READ siyiPitchHudVisible NOTIFY siyiPitchHudVisibleChanged)
    Q_PROPERTY(QString siyiPitchHudText READ siyiPitchHudText NOTIFY siyiPitchHudTextChanged)
```

- Next to `showSiyiZoomHud()`:

```cpp
    qreal siyiPitchDegrees() const { return _siyiPitchDegrees; }
    bool siyiPitchHudVisible() const { return _siyiPitchHudVisible; }
    QString siyiPitchHudText() const { return _siyiPitchHudText; }
    Q_INVOKABLE void showSiyiPitchHud();
```

- Signals (must pass the value):

```cpp
    void siyiPitchDegreesChanged(qreal siyiPitchDegrees);
    void siyiPitchHudVisibleChanged(bool siyiPitchHudVisible);
    void siyiPitchHudTextChanged(const QString& siyiPitchHudText);
```

- Private slots next to zoom HUD:

```cpp
    void _onUnipodPitchDegreesChanged();
    void _hideSiyiPitchHud();
```

- Private helpers / members:

```cpp
    void _setSiyiPitchHudVisible(bool visible);
    void _updateSiyiPitchHudText();
    qreal _siyiPitchDegrees = qQNaN();
    bool _siyiPitchHudVisible = false;
    QString _siyiPitchHudText;
    QTimer _siyiPitchHudHideTimer;
```

Need `#include <QtCore/QtNumeric>` in the header **or** initialize `_siyiPitchDegrees` in the `.cc` constructor. Prefer constructor init in `.cc` and `qreal _siyiPitchDegrees;` with assignment `= qQNaN()` — `QGCCameraManager.h` does not currently include QtNumeric. Add `#include <QtCore/QtNumeric>` to the header.

`QGCCameraManager.cc` near `kSiyiZoomHudHideMs`:

```cpp
constexpr int kSiyiPitchHudHideMs = 5000;
```

Constructor, after the zoom hide timer setup:

```cpp
    _siyiPitchHudHideTimer.setSingleShot(true);
    _siyiPitchHudHideTimer.setInterval(kSiyiPitchHudHideMs);
    (void) connect(&_siyiPitchHudHideTimer, &QTimer::timeout, this, &QGCCameraManager::_hideSiyiPitchHud);

    (void) connect(_unipodClient, &UnipodMt11Client::pitchDegreesChanged, this,
                   &QGCCameraManager::_onUnipodPitchDegreesChanged);
```

Implementation (place next to `_onUnipodZoomLevelChanged`):

```cpp
void QGCCameraManager::_onUnipodPitchDegreesChanged()
{
    if (!_unipodClient) {
        return;
    }

    const double pitch = _unipodClient->pitchDegrees();
    if (qIsNaN(pitch)) {
        if (!qIsNaN(_siyiPitchDegrees)) {
            _siyiPitchDegrees = qQNaN();
            emit siyiPitchDegreesChanged(_siyiPitchDegrees);
        }
        _siyiPitchHudHideTimer.stop();
        _setSiyiPitchHudVisible(false);
        _updateSiyiPitchHudText();
        return;
    }

    const bool primed = !qIsNaN(_siyiPitchDegrees);
    const int oldRounded = primed ? qRound(_siyiPitchDegrees) : 0;
    const int newRounded = qRound(pitch);
    if (primed && qFuzzyCompare(_siyiPitchDegrees, static_cast<qreal>(pitch))) {
        return;
    }

    _siyiPitchDegrees = static_cast<qreal>(pitch);
    emit siyiPitchDegreesChanged(_siyiPitchDegrees);
    _updateSiyiPitchHudText();
    if (primed && newRounded != oldRounded) {
        showSiyiPitchHud();
    }
}

void QGCCameraManager::showSiyiPitchHud()
{
    if (qIsNaN(_siyiPitchDegrees)) {
        return;
    }
    _updateSiyiPitchHudText();
    _setSiyiPitchHudVisible(true);
    _siyiPitchHudHideTimer.start();
}

void QGCCameraManager::_hideSiyiPitchHud()
{
    _setSiyiPitchHudVisible(false);
}

void QGCCameraManager::_setSiyiPitchHudVisible(bool visible)
{
    if (_siyiPitchHudVisible == visible) {
        return;
    }
    _siyiPitchHudVisible = visible;
    emit siyiPitchHudVisibleChanged(_siyiPitchHudVisible);
}

void QGCCameraManager::_updateSiyiPitchHudText()
{
    const QString text = qIsNaN(_siyiPitchDegrees) ? QString() : tr("%1°").arg(qRound(_siyiPitchDegrees));
    if (_siyiPitchHudText == text) {
        return;
    }
    _siyiPitchHudText = text;
    emit siyiPitchHudTextChanged(_siyiPitchHudText);
}
```

Do **not** use `qFuzzyCompare` to skip the integer check — the early `qFuzzyCompare` return is only to ignore sub-0.05 noise already filtered in the client. If both fire, HUD still keys off `qRound`.

- [ ] **Step 4: Re-run tests — expect PASS**

```bash
cmake --build build --target QGroundControl --parallel
ctest --preset default --output-on-failure -R 'QGCCameraManagerTest|UnipodMt11ProtocolTest'
```

Expected: `_testSiyiPitchHudFollowsUdpClient` and `_testSiyiZoomLevelFollowsUdpClient` both pass.

- [ ] **Step 5: Commit** (skip unless the user asked)

```bash
git add src/Camera/QGCCameraManager.h src/Camera/QGCCameraManager.cc \
        test/Camera/QGCCameraManagerTest.h test/Camera/QGCCameraManagerTest.cc
git commit -m "$(cat <<'EOF'
feat(Camera): show SIYI pitch HUD when rounded degrees change

EOF
)"
```

---

### Task 4: Scrolling tape QML

**Files:**
- Create: `src/FlyView/GimbalPitchTape.qml`
- Modify: `src/FlyView/CMakeLists.txt`
- Modify: `src/FlyView/FlyViewPayloadOverlay.qml`

**Interfaces:**
- Consumes: `siyiPitchHudVisibleChanged(visible)`, `siyiPitchHudTextChanged(text)`, `siyiPitchDegreesChanged(pitch)`
- Produces: `GimbalPitchTape` with `tapeVisible`, `pitchDegrees`, `hudText`

- [ ] **Step 1: Register the QML file**

In `src/FlyView/CMakeLists.txt` `QML_FILES`, insert alphabetically after `FlyViewWidgetLayer.qml`:

```
              GimbalPitchTape.qml
```

- [ ] **Step 2: Create `src/FlyView/GimbalPitchTape.qml`**

```qml
import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property bool tapeVisible: false
    property real pitchDegrees: 0
    property string hudText: ""

    visible: tapeVisible
    enabled: false
    z: 3
    width: ScreenTools.defaultFontPixelWidth * 5
    height: Math.min(parent ? parent.height * 0.42 : 0, ScreenTools.defaultFontPixelHeight * 12)

    readonly property real _minPitch: -90
    readonly property real _maxPitch: 30
    readonly property real _spanDeg: 50
    readonly property real _pixelsPerDegree: height / _spanDeg
    readonly property real _drawPitch: {
        if (pitchDegrees !== pitchDegrees) {
            return 0
        }
        return Math.max(_minPitch, Math.min(_maxPitch, pitchDegrees))
    }
    readonly property int _tickCount: ((_maxPitch - _minPitch) / 5) + 1

    clip: true

    Repeater {
        model: root._tickCount

        Item {
            width: root.width
            height: 1
            readonly property real tickValue: root._minPitch + index * 5
            readonly property bool major: (tickValue % 10) === 0
            readonly property bool horizon: tickValue === 0
            y: (root.height / 2) + (root._drawPitch - tickValue) * root._pixelsPerDegree

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.2
                width: horizon ? parent.width * 0.85 : (major ? parent.width * 0.55 : parent.width * 0.35)
                height: horizon ? 2 : 1
                color: Qt.rgba(1, 1, 1, horizon ? 0.9 : (major ? 0.65 : 0.35))
            }

            QGCLabel {
                visible: major
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                text: tickValue > 0 ? ("+" + tickValue) : ("" + tickValue)
                color: Qt.rgba(1, 1, 1, horizon ? 0.95 : 0.7)
                font.pointSize: ScreenTools.smallFontPointSize
            }
        }
    }

    Rectangle {
        id: window
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: hudLabel.width + ScreenTools.defaultFontPixelWidth * 1.2
        height: hudLabel.height + ScreenTools.defaultFontPixelHeight * 0.35
        radius: ScreenTools.defaultFontPixelWidth * 0.4
        color: Qt.rgba(0, 0, 0, 0.45)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.35)

        QGCLabel {
            id: hudLabel
            anchors.centerIn: parent
            text: root.hudText
            color: "white"
            font.pointSize: ScreenTools.mediumFontPointSize
            font.bold: true
        }
    }
}
```

Tick `y`: looking up (pitch increases) moves ticks **down**, matching the spec.

- [ ] **Step 3: Host in `FlyViewPayloadOverlay.qml`**

Next to zoom HUD locals (~line 49):

```qml
    property bool _pitchHudVisible: false
    property string _pitchHudText: ""
    property real _pitchDegrees: Number.NaN
```

In `_syncZoomHudFromManager`, also sync pitch (or add `_syncPitchHudFromManager` and call both from `on_CameraManagerChanged`):

```qml
    function _syncPitchHudFromManager() {
        if (!_cameraManager) {
            _pitchHudVisible = false
            _pitchHudText = ""
            _pitchDegrees = Number.NaN
            return
        }
        _pitchHudVisible = _cameraManager.siyiPitchHudVisible
        _pitchHudText = _cameraManager.siyiPitchHudText
        _pitchDegrees = _cameraManager.siyiPitchDegrees
    }
```

Call `_syncPitchHudFromManager()` from `on_CameraManagerChanged` (keep the zoom sync).

After `zoomHud` (before or after the zoom `Connections`), add the tape. Place it as a **sibling of `rightBar`** so `anchors.right: rightBar.left` works. `rightBar` is declared later in the file — Qt allows forward id references.

```qml
    GimbalPitchTape {
        id: pitchTape
        anchors.right: rightBar.left
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.4
        y: {
            var maxY = parent.height - root.bottomReserve - height
            var center = (parent.height - height) / 2
            return Math.max(0, Math.min(center, maxY))
        }
        tapeVisible: root._pitchHudVisible && rightBar.visible
        pitchDegrees: root._pitchDegrees
        hudText: root._pitchHudText
    }
```

Extend the existing zoom `Connections` (same `target: root._cameraManager`):

```qml
        function onSiyiPitchHudVisibleChanged(visible) {
            root._pitchHudVisible = visible
        }
        function onSiyiPitchHudTextChanged(text) {
            root._pitchHudText = text
        }
        function onSiyiPitchDegreesChanged(pitch) {
            root._pitchDegrees = pitch
        }
```

Do **not** read `_cameraManager.siyiPitchHudText` inside those handlers.

- [ ] **Step 4: Build**

```bash
cmake --build build --target QGroundControl --parallel
```

Expected: QML module compiles. If `qmllint` is run via `just lint`, `GimbalPitchTape.qml` must be clean.

- [ ] **Step 5: Commit** (skip unless the user asked)

```bash
git add src/FlyView/GimbalPitchTape.qml src/FlyView/CMakeLists.txt src/FlyView/FlyViewPayloadOverlay.qml
git commit -m "$(cat <<'EOF'
feat(FlyView): add DJI-style SIYI gimbal pitch tape

EOF
)"
```

---

### Task 5: Verify

**Files:** none unless a test or layout bug shows up.

- [ ] **Step 1: Unit / integration**

```bash
cmake --build build --target QGroundControl --parallel
ctest --preset default --output-on-failure -R 'UnipodMt11ProtocolTest|QGCCameraManagerTest'
```

Expected: all listed tests pass, including zoom HUD.

- [ ] **Step 2: Device (ZR10 on landscape remote)**

1. Connect SIYI ZR10 video source. Confirm **no** pitch tape flash on connect.
2. RC gimbal pitch: tape appears left of zoom/photo, center integer follows motion, hides ~5 s after stop.
3. Overlay 回中 / 向下: tape appears. 偏航回中: tape stays hidden if rounded pitch does not change.
4. Hold screen zoom +/- : zoom HUD still updates; pitch tape independent.
5. Short landscape: tape does not cover zoom/photo buttons or sit in `bottomReserve`.

- [ ] **Step 3: Commit** (skip unless the user asked) — only if Step 1/2 caused extra fixes.

---

## Self-review (spec coverage)

| Spec item | Task |
| --- | --- |
| CMD `0x0D` request hex + parse `/10` | Task 1 |
| Idle 150 ms + hold 80 ms poll; RC path | Task 2 |
| Prime / `qRound` / 5 s linger / NaN reset | Task 3 |
| DJI tape, integer `°`, right of video | Task 4 |
| No `0x25`, no yaw, no MAVLink, no zoom regression | Tasks 2–5 (non-goals + zoom test kept) |
| QML signal-argument copy | Task 4 |
| Device landscape | Task 5 |
