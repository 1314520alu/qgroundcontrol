# UniGCS-style Fly View Payload Overlay Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a UniGCS-like left/right video overlay on Fly View that shows payload buttons only when the active camera advertises each capability.

**Architecture:** New QML overlay in `FlyViewWidgetLayer` (not inside the PIP video item). Capability flags live on `MavlinkCameraControlInterface` with default `false`. Compact `PhotoVideoControl` remains for map-main / PIP. Phase 1 wires TQ10N (gimbal/photo/record/zoom) and MT11 (photo/record/media gallery).

**Tech Stack:** Qt 6 QML, `MavlinkCameraControlInterface`, existing Topotek gimbal/zoom QML, UniPod media gallery, CTest Unit tests.

**Spec:** `docs/superpowers/specs/2026-08-21-unigcs-payload-overlay-design.md`

## Global Constraints

- Landscape remotes first (Skydroid G20 / SIYI UniRC); short height is the bottleneck; glove touch ≥ `ScreenTools.minTouchPixels`.
- Never dereference `activeVehicle()` / `Vehicle*` / `_camera` without a null check.
- No `Q_ASSERT` in production code.
- Capability false → button **hidden**, not greyed out.
- No new payload protocols in Phase 1 (lens/laser/AI/follow stay hidden).
- Histogram / pencil icons are out of scope.
- Overlay only when video is the **main** Fly View item; PIP keeps the compact strip.
- Conventional Commits: `feat(FlyView): …` / `feat(Camera): …`.

---

### Task 1: Camera capability flags + unit tests

**Files:**

- Modify: `src/Camera/MavlinkCameraControlInterface.h`
- Modify: `src/Camera/TopotekTq10CameraControl.h` (remove duplicate `Q_PROPERTY`, keep override)
- Create: `test/Camera/CameraPayloadCapabilitiesTest.h`
- Create: `test/Camera/CameraPayloadCapabilitiesTest.cc`
- Modify: `test/Camera/CMakeLists.txt`

**Interfaces:**

- Consumes: existing `MavlinkCameraControlInterface`, `TopotekTq10CameraControl(Vehicle*, TopotekTq10Client*, QObject*)`, `SimulatedCameraControl(Vehicle*, QObject*)`
- Produces: `virtual bool hasGimbalPad() const`, `hasLensSwitch()`, `hasLaserRange()`, `hasAiRecognition()`, `hasFollowFlight()`, `hasMediaLibrary()` — all default `false`; `Q_PROPERTY` + `NOTIFY infoChanged`

- [ ] **Step 1: Write the failing test**

`test/Camera/CameraPayloadCapabilitiesTest.h`:

```cpp
#pragma once

#include "UnitTest.h"

class CameraPayloadCapabilitiesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSimulatedDefaultsFalse();
    void testTopotekHasGimbalPad();
};
```

`test/Camera/CameraPayloadCapabilitiesTest.cc`:

```cpp
#include "CameraPayloadCapabilitiesTest.h"

#include "SimulatedCameraControl.h"
#include "TopotekTq10CameraControl.h"
#include "TopotekTq10Client.h"

void CameraPayloadCapabilitiesTest::testSimulatedDefaultsFalse()
{
    SimulatedCameraControl cam(nullptr, this);
    QCOMPARE(cam.hasGimbalPad(), false);
    QCOMPARE(cam.hasLensSwitch(), false);
    QCOMPARE(cam.hasLaserRange(), false);
    QCOMPARE(cam.hasAiRecognition(), false);
    QCOMPARE(cam.hasFollowFlight(), false);
    QCOMPARE(cam.hasMediaLibrary(), false);
}

void CameraPayloadCapabilitiesTest::testTopotekHasGimbalPad()
{
    TopotekTq10Client client(this);
    TopotekTq10CameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.hasGimbalPad(), true);
    QCOMPARE(cam.hasLensSwitch(), false);
    QCOMPARE(cam.hasLaserRange(), false);
    QCOMPARE(cam.hasMediaLibrary(), false);
}
```

Add to `test/Camera/CMakeLists.txt` `target_sources` and:

```cmake
add_qgc_test(CameraPayloadCapabilitiesTest LABELS Unit Camera)
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cd ~/Projects/qgroundcontrol && source ./env-qgc.sh
cmake --build build/macOS --target CameraPayloadCapabilitiesTest -j8
ctest --test-dir build/macOS -R CameraPayloadCapabilitiesTest --output-on-failure
```

Expected: compile fail (`hasGimbalPad` not on interface / Simulated) or FAIL `false != true` once declared only on Topotek extra property depending on moc.

- [ ] **Step 3: Add interface defaults and move Topotek override**

In `MavlinkCameraControlInterface.h`, after the `hasTracking` `Q_PROPERTY` block add:

```cpp
Q_PROPERTY(bool hasGimbalPad      READ hasGimbalPad      NOTIFY infoChanged)
Q_PROPERTY(bool hasLensSwitch     READ hasLensSwitch     NOTIFY infoChanged)
Q_PROPERTY(bool hasLaserRange     READ hasLaserRange     NOTIFY infoChanged)
Q_PROPERTY(bool hasAiRecognition  READ hasAiRecognition  NOTIFY infoChanged)
Q_PROPERTY(bool hasFollowFlight   READ hasFollowFlight   NOTIFY infoChanged)
Q_PROPERTY(bool hasMediaLibrary   READ hasMediaLibrary   NOTIFY infoChanged)
```

In the public virtuals section (near `hasTracking()`), add non-pure defaults:

```cpp
virtual bool hasGimbalPad() const { return false; }
virtual bool hasLensSwitch() const { return false; }
virtual bool hasLaserRange() const { return false; }
virtual bool hasAiRecognition() const { return false; }
virtual bool hasFollowFlight() const { return false; }
virtual bool hasMediaLibrary() const { return false; }
```

In `TopotekTq10CameraControl.h`:

- Delete `Q_PROPERTY(bool hasGimbalPad READ hasGimbalPad CONSTANT)`
- Change `bool hasGimbalPad() const { return true; }` to `bool hasGimbalPad() const override { return true; }`

Do **not** add stubs on `SimulatedCameraControl` or `VehicleCameraControl` — they inherit `false`.

- [ ] **Step 4: Run the tests and make sure they pass**

```bash
cmake --build build/macOS -j8
ctest --test-dir build/macOS -R CameraPayloadCapabilitiesTest --output-on-failure
```

Expected: PASS both slots.

- [ ] **Step 5: Commit**

```bash
git add src/Camera/MavlinkCameraControlInterface.h src/Camera/TopotekTq10CameraControl.h \
  test/Camera/CameraPayloadCapabilitiesTest.h test/Camera/CameraPayloadCapabilitiesTest.cc \
  test/Camera/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(Camera): add payload overlay capability flags

Default-false gimbal/lens/laser/AI/follow/media flags on the camera
interface so Fly View can hide UniGCS-style buttons per camera.
EOF
)"
```

---

### Task 2: UniPod `hasMediaLibrary` follows media client ready

**Files:**

- Modify: `src/Camera/UnipodMt11CameraControl.h`
- Modify: `src/Camera/UnipodMt11CameraControl.cc`
- Modify: `src/Camera/QGCCameraManager.cc` (after constructing both objects, call `setMediaClient`)
- Modify: `test/Camera/CameraPayloadCapabilitiesTest.h`
- Modify: `test/Camera/CameraPayloadCapabilitiesTest.cc`

**Interfaces:**

- Consumes: `UnipodMt11MediaClient::isReady()`, `setReady(bool)`, `readyChanged()`
- Produces: `void UnipodMt11CameraControl::setMediaClient(UnipodMt11MediaClient *client)`; `bool hasMediaLibrary() const override` → `_mediaClient && _mediaClient->isReady()`

- [ ] **Step 1: Write the failing test**

Add slot `testUnipodMediaLibraryFollowsReady` to the test class:

```cpp
#include "UnipodMt11CameraControl.h"
#include "UnipodMt11Client.h"
#include "UnipodMt11MediaClient.h"

void CameraPayloadCapabilitiesTest::testUnipodMediaLibraryFollowsReady()
{
    UnipodMt11Client client(this);
    UnipodMt11MediaClient media(this);
    UnipodMt11CameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.hasMediaLibrary(), false);

    cam.setMediaClient(&media);
    QCOMPARE(cam.hasMediaLibrary(), false);

    QSignalSpy spy(&cam, &MavlinkCameraControlInterface::infoChanged);
    media.setReady(true);
    QCOMPARE(cam.hasMediaLibrary(), true);
    QVERIFY(spy.count() >= 1);

    media.setReady(false);
    QCOMPARE(cam.hasMediaLibrary(), false);
}
```

Need `#include <QtTest/QSignalSpy>` and `MavlinkCameraControlInterface.h`.

- [ ] **Step 2: Run test to verify it fails**

```bash
ctest --test-dir build/macOS -R CameraPayloadCapabilitiesTest --output-on-failure
```

Expected: compile fail (`setMediaClient` missing) or FAIL.

- [ ] **Step 3: Implement**

`UnipodMt11CameraControl.h` — forward-declare `class UnipodMt11MediaClient;`, add:

```cpp
void setMediaClient(UnipodMt11MediaClient *client);
bool hasMediaLibrary() const override;
```

Private: `UnipodMt11MediaClient *_mediaClient = nullptr;`

`UnipodMt11CameraControl.cc`:

```cpp
#include "UnipodMt11MediaClient.h"

void UnipodMt11CameraControl::setMediaClient(UnipodMt11MediaClient *client)
{
    if (_mediaClient == client) {
        return;
    }
    if (_mediaClient) {
        disconnect(_mediaClient, nullptr, this, nullptr);
    }
    _mediaClient = client;
    if (_mediaClient) {
        connect(_mediaClient, &UnipodMt11MediaClient::readyChanged, this, &UnipodMt11CameraControl::infoChanged);
    }
    emit infoChanged();
}

bool UnipodMt11CameraControl::hasMediaLibrary() const
{
    return _mediaClient && _mediaClient->isReady();
}
```

In `QGCCameraManager` constructor, after `_unipodMediaClient` and `_unipodCameraControl` are created:

```cpp
_unipodCameraControl->setMediaClient(_unipodMediaClient);
```

- [ ] **Step 4: Run tests**

```bash
cmake --build build/macOS -j8
ctest --test-dir build/macOS -R "CameraPayloadCapabilitiesTest|UnipodMt11MediaClientTest" --output-on-failure
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/Camera/UnipodMt11CameraControl.h src/Camera/UnipodMt11CameraControl.cc \
  src/Camera/QGCCameraManager.cc test/Camera/CameraPayloadCapabilitiesTest.h \
  test/Camera/CameraPayloadCapabilitiesTest.cc
git commit -m "$(cat <<'EOF'
feat(Camera): gate UniPod media library on HTTP client ready

Expose hasMediaLibrary so Fly View 保存 only appears when the TF
gallery client is actually usable.
EOF
)"
```

---

### Task 3: Icon button + side bar QML

**Files:**

- Create: `src/FlyView/FlyViewPayloadIconButton.qml`
- Create: `src/FlyView/FlyViewPayloadSideBar.qml`
- Modify: `src/FlyView/CMakeLists.txt` (`QML_FILES`)

**Interfaces:**

- Consumes: `ScreenTools.minTouchPixels`, `QGCPalette`, `QGCColoredImage`, `QGCLabel`
- Produces: `FlyViewPayloadIconButton` properties `iconSource`, `label`, `selected`; signals `clicked`; `FlyViewPayloadSideBar` as `ColumnLayout` with default children

- [ ] **Step 1: Add `FlyViewPayloadIconButton.qml`**

```qml
import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property url iconSource: ""
    property string label: ""
    property bool selected: false

    signal clicked()

    width: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 3.4)
    height: width + labelItem.height + ScreenTools.defaultFontPixelHeight * 0.15

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Rectangle {
        id: circle
        width: root.width
        height: width
        radius: width * 0.5
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
        border.width: 1
        border.color: (pressArea.pressed || root.selected)
                      ? qgcPal.buttonHighlight
                      : Qt.rgba(qgcPal.buttonBorder.r, qgcPal.buttonBorder.g, qgcPal.buttonBorder.b, 0.45)

        QGCColoredImage {
            anchors.centerIn: parent
            width: parent.width * 0.5
            height: width
            source: root.iconSource
            sourceSize.height: height
            color: qgcPal.text
            fillMode: Image.PreserveAspectFit
            visible: root.iconSource !== ""
        }
    }

    QGCLabel {
        id: labelItem
        anchors.horizontalCenter: circle.horizontalCenter
        anchors.top: circle.bottom
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.08
        text: root.label
        font.pointSize: ScreenTools.smallFontPointSize
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
```

- [ ] **Step 2: Add `FlyViewPayloadSideBar.qml`**

```qml
import QtQuick
import QtQuick.Layouts

import QGroundControl

ColumnLayout {
    id: root
    spacing: ScreenTools.defaultFontPixelWidth / 2
}
```

- [ ] **Step 3: Register in `src/FlyView/CMakeLists.txt`**

Insert both files in `QML_FILES` next to `FlyViewLocalVideoControls.qml`.

- [ ] **Step 4: Incremental build**

```bash
cd ~/Projects/qgroundcontrol && source ./env-qgc.sh
cmake --build build/macOS -j8
```

Expected: QML module recompile succeeds. (qmllint optional: `just lint` if it covers these files.)

- [ ] **Step 5: Commit**

```bash
git add src/FlyView/FlyViewPayloadIconButton.qml src/FlyView/FlyViewPayloadSideBar.qml src/FlyView/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(FlyView): add UniGCS-style payload overlay icon controls

Circular translucent icon+label and a vertical side bar for landscape
video chrome.
EOF
)"
```

---

### Task 4: Overlay host + hide compact strips when video is main

**Files:**

- Create: `src/FlyView/FlyViewPayloadOverlay.qml`
- Modify: `src/FlyView/CMakeLists.txt`
- Modify: `src/FlyView/FlyViewWidgetLayer.qml`
- Modify: `src/FlyView/FlyViewTopRightColumnLayout.qml`
- Modify: `src/FlyView/FlyViewTopRightPanel.qml`
- Modify: `src/FlightMap/Widgets/PhotoVideoControl.qml`
- Modify: `src/FlyView/FlyViewLocalVideoControls.qml`

**Interfaces:**

- Consumes: `mapControl.pipState` (`fullState` = map is main), `QGroundControl.videoManager.hasVideo`, camera flags from Task 1–2, `toolStrip.width`
- Produces: `bool overlayActive` — true only when overlay is actually showing at least one button; parents pass `hideWhenPayloadOverlay: payloadOverlay.overlayActive`

Camera `_camera` binding **must** match `PhotoVideoControl.qml` (prefer UniPod MT11, then Topotek TQ10N, then any capturing camera). Copy that `property var _camera` block verbatim.

- [ ] **Step 1: Add hide properties on compact strips**

`PhotoVideoControl.qml` near other properties:

```qml
property bool hideWhenPayloadOverlay: false
```

Change `visible:` to start with `!hideWhenPayloadOverlay &&`.

`FlyViewLocalVideoControls.qml`:

```qml
property bool hideWhenPayloadOverlay: false
```

AND into `_show`: `&& !hideWhenPayloadOverlay`.

- [ ] **Step 2: Write `FlyViewPayloadOverlay.qml` (visibility + columns, expand can be empty host)**

Key properties:

```qml
property var mapControl
property real toolStripWidth: 0
property real bottomReserve: ScreenTools.defaultFontPixelHeight * 8

readonly property bool _videoIsMain: mapControl && mapControl.pipState
    && mapControl.pipState.state !== mapControl.pipState.fullState
readonly property bool _hasLeft: _camera && _camera.hasGimbalPad
    || (_camera && _camera.hasLensSwitch)
    || (_camera && _camera.hasLaserRange)
    || (_camera && _camera.hasAiRecognition)
    || (_camera && _camera.hasFollowFlight)
readonly property bool _hasRight: _camera && (
    (_camera.exposureMode != null)
    || _camera.capturesPhotos
    || _camera.capturesVideo
    || _camera.hasZoom
    || _camera.hasFocus
    || _camera.hasMediaLibrary)
readonly property bool overlayActive: visible && (_hasLeft || _hasRight)

visible: QGroundControl.videoManager.hasVideo && _videoIsMain && (_hasLeft || _hasRight)
         && !QGroundControl.videoManager.fullScreen

enum Expand { None, Gimbal, Lens, Range, Recognize, Follow, Auto, Zoom, Focus }
property int expand: FlyViewPayloadOverlay.None
```

Fix operator precedence in `_hasLeft`: wrap `_camera && (_camera.hasGimbalPad || …)`.

On `_cameraChanged`: `expand = FlyViewPayloadOverlay.None`.

Layout:

- Left `FlyViewPayloadSideBar`: `anchors.left: parent.left`; `anchors.leftMargin: toolStripWidth + ScreenTools.defaultFontPixelWidth * 0.75`; `anchors.top: parent.top`; `anchors.topMargin: ScreenTools.defaultFontPixelHeight`; `visible: _hasLeft`
- Right bar: `anchors.right: parent.right`; same top; `visible: _hasRight`
- Collapse `MouseArea`: `anchors.fill: parent`; `enabled: expand !== FlyViewPayloadOverlay.None`; `z: 0`; `onClicked: expand = FlyViewPayloadOverlay.None`
- Side bars and expand host `z: 1`

Left buttons (each `visible` on its flag; `selected: expand === …`; `onClicked` toggles that expand or `None`):

| label `qsTr` | icon | flag |
| --- | --- | --- |
| 云台 | `/InstrumentValueIcons/gimbal-2.svg` | `hasGimbalPad` |
| 镜头 | `/InstrumentValueIcons/camera.svg` | `hasLensSwitch` |
| 测距 | `/InstrumentValueIcons/radar.svg` | `hasLaserRange` |
| 开启识别 | `/qmlimages/TrackingIcon.svg` | `hasAiRecognition` |
| 跟随飞行 | `/InstrumentValueIcons/drone.svg` | `hasFollowFlight` |

Right buttons:

| label | icon | flag | action |
| --- | --- | --- | --- |
| AUTO | (no icon; button `label` only is OK, or `/InstrumentValueIcons/brightness-down.svg`) | `exposureMode != null` | expand Auto |
| 拍照 | `/qmlimages/camera_photo.svg` | `capturesPhotos` | `_camera.takePhoto()` — **do not** change expand |
| 录像 | `/qmlimages/camera_video.svg` | `capturesVideo` | `_camera.toggleVideoRecording()` |
| 变倍 | `/InstrumentValueIcons/zoom-in.svg` | `hasZoom` | expand Zoom |
| 变焦 | `/InstrumentValueIcons/camera.svg` | `hasFocus` | expand Focus |
| 保存 | `/res/SaveToDisk.svg` | `hasMediaLibrary` | open gallery (Task 5 if not yet; stub `console.log` forbidden — wire factory in Task 5) |

Photo/record: keep immediate; optional inner white/red circle can wait — icon+label is enough for Phase 1.

- [ ] **Step 3: Host overlay in `FlyViewWidgetLayer.qml`**

Insert **before** `FlyViewToolStrip` (toolstrip paints on top):

```qml
FlyViewPayloadOverlay {
    id:                     payloadOverlay
    anchors.fill:           parent
    mapControl:             _root.mapControl
    toolStripWidth:         toolStrip.visible ? toolStrip.width : 0
    z:                      QGroundControl.zOrderWidgets
}
```

Pass hide flag:

```qml
FlyViewTopRightColumnLayout {
    hideWhenPayloadOverlay: payloadOverlay.overlayActive
    ...
}
```

Add `property bool hideWhenPayloadOverlay: false` on `FlyViewTopRightColumnLayout` and:

```qml
PhotoVideoControl { hideWhenPayloadOverlay: hideWhenPayloadOverlay }
FlyViewLocalVideoControls {
    hideWhenPhotoVideoVisible: photoVideoControlLoader.visible
    hideWhenPayloadOverlay: hideWhenPayloadOverlay
}
```

`FlyViewTopRightPanel.qml` `PhotoVideoControl`: same property, bind from widget layer `topRightPanel.hideWhenPayloadOverlay: payloadOverlay.overlayActive` (add the property on the panel root).

- [ ] **Step 4: Build**

```bash
cmake --build build/macOS -j8
```

Expected: success. Desktop smoke: video source Topotek, swap map/video PIP — overlay only when video is main; compact strip when map is main.

- [ ] **Step 5: Commit**

```bash
git add src/FlyView/FlyViewPayloadOverlay.qml src/FlyView/FlyViewWidgetLayer.qml \
  src/FlyView/FlyViewTopRightColumnLayout.qml src/FlyView/FlyViewTopRightPanel.qml \
  src/FlyView/FlyViewLocalVideoControls.qml src/FlyView/CMakeLists.txt \
  src/FlightMap/Widgets/PhotoVideoControl.qml
git commit -m "$(cat <<'EOF'
feat(FlyView): overlay UniGCS payload chrome on full-window video

Show capability-gated left/right icon columns and hide the compact
photo strip while video is the main Fly View item.
EOF
)"
```

---

### Task 5: Expand panels + media gallery 保存

**Files:**

- Modify: `src/FlyView/FlyViewPayloadOverlay.qml`
- Modify: `src/FlyView/TopotekZoomHoldButtons.qml`

**Interfaces:**

- Consumes: `TopotekGimbalPad`, `TopotekZoomHoldButtons`, `QGCPopupDialogFactory`, `UnipodMt11MediaGallery`, `QGCCameraManager.unipodMediaClient`
- Produces: exclusive expand host; 保存 opens existing gallery; zoom buttons also drive focus via `useFocus`

- [ ] **Step 1: Parameterize zoom hold for focus**

`TopotekZoomHoldButtons.qml` add:

```qml
property bool useFocus: false
```

Pressed/released:

```qml
onPressed: {
    if (!root.camera) { return }
    if (root.useFocus) { root.camera.startFocus(1) } else { root.camera.startZoom(1) }
}
onReleased: {
    if (!root.camera) { return }
    if (root.useFocus) { root.camera.stopFocus() } else { root.camera.stopZoom() }
}
```

(minus button uses `-1`.) Label: `root.useFocus ? qsTr("Focus") : qsTr("Zoom")`.

- [ ] **Step 2: Expand host in overlay**

Place an `Item` between left bar and right bar (`anchors.left: leftBar.right`, `anchors.right: rightBar.left`, `anchors.top: leftBar.top`, `z: 1`):

- `expand === Gimbal` && `hasGimbalPad`: `TopotekGimbalPad { camera: _camera }`
- `expand === Zoom`: `TopotekZoomHoldButtons { camera: _camera; useFocus: false }`
- `expand === Focus`: `TopotekZoomHoldButtons { camera: _camera; useFocus: true }`
- `expand === Auto` && `_camera.exposureMode`: `FactComboBox { fact: _camera.exposureMode; indexModel: false; sizeToContents: true }` (import `QGroundControl.FactControls`)
- Lens/Range/Recognize/Follow: no extra UI in Phase 1 (buttons hidden). If a flag is later true with no panel, tapping still toggles expand to an empty host — acceptable.

Toggle helper:

```qml
function _toggleExpand(value) {
    expand = (expand === value) ? FlyViewPayloadOverlay.None : value
}
```

- [ ] **Step 3: 保存 → media gallery**

Copy the `QGCPopupDialogFactory` + `UnipodMt11MediaGallery` pattern from `PhotoVideoControl.qml` (`mediaGalleryFactory.open({ mediaClient: _unipodMediaClient })`).

```qml
property var _cameraManager: _activeVehicle ? _activeVehicle.cameraManager : null
property var _unipodMediaClient: _cameraManager ? _cameraManager.unipodMediaClient : null
```

保存 `onClicked`: if `_unipodMediaClient` then `mediaGalleryFactory.open({ mediaClient: _unipodMediaClient })`.

- [ ] **Step 4: Build**

```bash
cmake --build build/macOS -j8
```

Expected: success. TQ10N: tap 云台 → D-pad; tap 变倍 → +/-; tap again collapses. MT11: 保存 opens gallery when client ready.

- [ ] **Step 5: Commit**

```bash
git add src/FlyView/FlyViewPayloadOverlay.qml src/FlyView/TopotekZoomHoldButtons.qml
git commit -m "$(cat <<'EOF'
feat(FlyView): expand gimbal/zoom/focus and open UniPod gallery

Tap overlay icons to expand existing pads; 保存 reuses the MT11
media library dialog.
EOF
)"
```

---

### Task 6: zh_CN strings + Android verify

**Files:**

- Modify: `translations/qgc_source_zh_CN.ts` (or run the repo translation update if one exists)
- Manual: UniRC `192.168.0.102:5555` (or current wireless ADB)

**Interfaces:**

- Consumes: `qsTr` source strings from overlay
- Produces: Chinese labels matching UniGCS: 云台、镜头、测距、开启识别、跟随飞行、拍照、录像、变倍、变焦、保存; AUTO can stay `AUTO`

- [ ] **Step 1: Ensure every user-visible overlay string uses `qsTr`**

AUTO: `qsTr("AUTO")`. Zoom expand label already `qsTr("Zoom")` / `qsTr("Focus")` — add zh if missing: 变倍 / 变焦 for the **icon** labels (not the +/- caption). Icon 变倍 = `qsTr("Zoom")` with zh `变倍`; 变焦 = `qsTr("Focus")` with zh `变焦`.

- [ ] **Step 2: Add zh_CN entries**

Search `translations/qgc_source_zh_CN.ts` for `<source>Zoom</source>`. If FlyViewPayloadOverlay context is missing after lupdate, add a context:

```xml
<context>
    <name>FlyViewPayloadOverlay</name>
    <message>
        <source>Gimbal</source>
        <translation>云台</translation>
    </message>
    ...
</context>
```

Use English `qsTr("Gimbal")` etc. in QML (not raw Chinese) so lupdate works. Mapping:

| qsTr | zh |
| --- | --- |
| Gimbal | 云台 |
| Lens | 镜头 |
| Range | 测距 |
| Recognize | 开启识别 |
| Follow | 跟随飞行 |
| Photo | 拍照 |
| Video | 录像 |
| Zoom | 变倍 |
| Focus | 变焦 |
| Save | 保存 |
| AUTO | AUTO |

- [ ] **Step 3: Android incremental build + install**

```bash
cd ~/Projects/qgroundcontrol && source ./env-qgc.sh
cmake --build build/Android-debug -j8
SERIAL=$(adb devices | awk '/device$/{print $1; exit}')
adb -s "$SERIAL" install -r -d build/Android-debug/android-build/QGroundControl.apk
adb -s "$SERIAL" shell am start -n org.mavlink.qgroundcontrol/.QGCActivity
```

- [ ] **Step 4: HIL checklist (short landscape)**

1. Video main + TQ10N: left 云台, right 拍照/录像/变倍; no 测距/识别/跟随/保存/AUTO/变焦.
2. Tap 云台 → D-pad; tap video empty → collapse; PTZ still works when collapsed (click-through).
3. Swap to map main: overlay gone, compact PhotoVideoControl (with TQ10N pad) back.
4. UniPod source + media ready: 保存 opens gallery; not ready: 保存 hidden.
5. Gloves: icons not smaller than other Fly buttons.

- [ ] **Step 5: Commit**

```bash
git add translations/qgc_source_zh_CN.ts src/FlyView/FlyViewPayloadOverlay.qml
git commit -m "$(cat <<'EOF'
feat(FlyView): localize payload overlay labels for zh_CN

Match UniGCS left/right captions on SIYI/Skydroid remotes.
EOF
)"
```

---

## Spec coverage

| Spec requirement | Task |
| --- | --- |
| Capability flags default false | 1 |
| TQ10N `hasGimbalPad` | 1 |
| MT11 `hasMediaLibrary` when ready | 2 |
| Icon+label chrome | 3 |
| Overlay on widget layer; video main only | 4 |
| Hide compact strip when overlay active | 4 |
| Keep compact strip for PIP / map main | 4 |
| Click-through when expand none | 4 |
| Tap expand gimbal/zoom/focus/AUTO | 5 |
| 保存 → gallery | 5 |
| Parameterize zoom for focus | 5 |
| zh_CN | 6 |
| No histogram/pencil; no new protocols | (omitted by design) |
| `just build` / UniRC HIL | 6 |

## Type names (locked)

- Expand enum: `FlyViewPayloadOverlay.None|Gimbal|Lens|Range|Recognize|Follow|Auto|Zoom|Focus`
- Hide property: `hideWhenPayloadOverlay`
- Overlay output: `overlayActive`
- Zoom property: `useFocus`
- UniPod: `setMediaClient(UnipodMt11MediaClient *)`
