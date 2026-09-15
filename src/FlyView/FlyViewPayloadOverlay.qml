import QtQuick
import QtMultimedia

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.FlyView

Item {
    id: root

    property var mapControl
    property real toolStripWidth: 0
    property real bottomReserve: Math.max(parent.height * 0.26, ScreenTools.defaultFontPixelHeight * 9)

    // Hide takeoff / land / RTL from ToolStrip while video is the main view (keep them on map)
    readonly property bool hidePrimaryGuidedOnVideo: QGroundControl.videoManager.hasVideo && _videoIsMain

    enum Expand { None, Gimbal, Lens, Range, Recognize, Follow, Auto, Zoom, Focus }

    property int expand: FlyViewPayloadOverlay.Expand.None

    property var _activeVehicle: globals.activeVehicle
    property var _guidedController: globals.guidedControllerFlyView
    property var _cameraManager: _activeVehicle ? _activeVehicle.cameraManager : null
    property var _unipodMediaClient: _cameraManager ? _cameraManager.unipodMediaClient : null
    property var _appSettings: QGroundControl.settingsManager.appSettings
    property bool _audioMuted: _appSettings && _appSettings.audioMuted.rawValue
    property real _audioVolume: _appSettings ? _appSettings.audioVolume.rawValue : 50
    readonly property bool _recording: _camera
                                       && _camera.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateCapturing
    readonly property bool _laserOn: _camera && _camera.hasLaserRange && _camera.laserEnabled
    readonly property real _laserDistanceM: {
        if (!_camera || !_camera.hasLaserRange) {
            return Number.NaN
        }
        return _camera.laserDistanceMeters
    }
    readonly property bool _laserDistanceValid: _laserDistanceM === _laserDistanceM
    readonly property string _laserHudText: {
        if (!_laserOn) {
            return ""
        }
        if (!_laserDistanceValid) {
            return qsTr("--.- m")
        }
        return Number(_laserDistanceM).toFixed(1) + " m"
    }
    property bool _zoomHudVisible: false
    property string _zoomHudText: ""

    function _syncZoomHudFromManager() {
        if (!_cameraManager) {
            _zoomHudVisible = false
            _zoomHudText = ""
            return
        }
        _zoomHudVisible = _cameraManager.siyiZoomHudVisible
        _zoomHudText = _cameraManager.siyiZoomHudText
    }

    on_CameraManagerChanged: _syncZoomHudFromManager()
    // Prefer a camera that can actually drive PhotoVideoControl. Manual streams (RTSP/UDP)
    // use SimulatedCameraControl for local VideoManager record; a MAVLink camera without
    // capture flags must not hide the UI. UniPod MT11 must never fall through to Simulated.
    property var _camera: {
        if (!_cameraManager) {
            return null
        }
        var current = _cameraManager.currentCameraInstance
        if (current && (current.modelName === "UniPod MT11"
                        || current.modelName === "SIYI A8 Mini"
                        || current.modelName === "SIYI ZR10"
                        || current.modelName === "Topotek TQ10N")) {
            return current
        }
        if (current && (current.hasGimbalPad || current.capturesVideo || current.capturesPhotos
                        || current.hasMediaLibrary || current.hasTracking || current.hasVideoStream)) {
            return current
        }
        var cams = _cameraManager.cameras
        if (cams) {
            for (var i = 0; i < cams.count; i++) {
                var c = cams.get(i)
                if (c && (c.hasGimbalPad || c.capturesVideo || c.capturesPhotos
                          || c.hasMediaLibrary || c.hasTracking || c.hasVideoStream)) {
                    return c
                }
            }
        }
        return current
    }

    readonly property bool _videoIsMain: mapControl && mapControl.pipState
                                         && mapControl.pipState.state !== mapControl.pipState.fullState
    readonly property bool _hasLeftPayload: _camera && (_camera.hasGimbalPad
                                                        || _camera.hasLensSwitch
                                                        || _camera.hasLaserRange
                                                        || _camera.hasAiRecognition
                                                        || _camera.hasFollowFlight)
    readonly property bool _hasRight: _camera && (
        _camera.hasExposureAuto
        || (_camera.exposureMode != null)
        || _camera.capturesPhotos
        || _camera.capturesVideo
        || _camera.hasZoom
        || _camera.hasFocus
        || _camera.hasMediaLibrary)
    // Overlay does not host tracking-rect chrome or photo/video mode switch.
    readonly property bool _compactOnlyExtras: _camera && (
        (_camera.hasTracking && !_camera.hasAiRecognition) || _camera.hasModes)
    property var _videoSettings: QGroundControl.settingsManager.videoSettings
    property bool _showRecControl: _videoSettings ? _videoSettings.showRecControl.rawValue : true
    readonly property bool overlayActive: visible && (_hasLeftPayload || _hasRight)

    readonly property int _leftButtonCount: {
        var n = 0
        if (_camera && _camera.hasGimbalPad) { n++ }
        if (_camera && _camera.hasLensSwitch) { n++ }
        if (_camera && _camera.hasLaserRange) { n++ }
        if (_camera && _camera.hasAiRecognition) { n++ }
        if (_camera && _camera.hasFollowFlight) { n++ }
        return n
    }

    readonly property int _rightButtonCount: {
        var n = 0
        if (!_camera) { return 0 }
        if (_camera.hasExposureAuto || _camera.exposureMode != null) { n++ }
        if (_camera.capturesPhotos) { n++ }
        if (_camera.capturesVideo) { n++ }
        if (_camera.hasZoom) { n++ }
        if (_camera.hasFocus) { n++ }
        if (_camera.hasMediaLibrary) { n++ }
        return n
    }

    // Compact overlay circles: glove-sized, not stretched to fill the column.
    readonly property real _maxCircle: Math.max(ScreenTools.minTouchPixels * 1.35, ScreenTools.defaultFontPixelWidth * 5.0)
    readonly property real _minCircle: ScreenTools.minTouchPixels
    readonly property real _barMinWidth: ScreenTools.defaultFontPixelWidth * 9.0
    // Frosted strip only a little wider than the icon circle
    readonly property real _barPadding: ScreenTools.defaultFontPixelWidth * 0.35
    readonly property real _barSpacing: ScreenTools.defaultFontPixelHeight * 0.3
    readonly property real _labelSlot: ScreenTools.defaultFontPixelHeight * 1.05
    readonly property real _barRadius: ScreenTools.defaultFontPixelWidth * 1.6
    readonly property real _barEdgeMargin: ScreenTools.defaultFontPixelWidth * 0.55
    readonly property real _barTopMargin: ScreenTools.defaultFontPixelHeight * 0.45
    // Same icon size + frosted width on both strips. Floor of 4 keeps size stable when AUTO is hidden.
    readonly property real _barAvailableHeight: Math.max(0, height - _barTopMargin - bottomReserve)
    readonly property int _barFitCount: Math.max(4, Math.max(_leftButtonCount, _rightButtonCount))
    readonly property real _sharedCircleSize: _fitCircleSize(_barAvailableHeight, _barFitCount, 0)
    readonly property real _sharedBarWidth: _sharedCircleSize + _barPadding * 2

    function _fitCircleSize(availableHeight, buttonCount, sectionGap) {
        if (buttonCount <= 0 || availableHeight <= 0) {
            return _maxCircle
        }
        var gapExtra = sectionGap ? sectionGap : 0
        var gaps = Math.max(buttonCount - 1, 0) * _barSpacing + gapExtra
        var per = (availableHeight - _barPadding * 2 - gaps) / buttonCount - _labelSlot
        return Math.max(_minCircle, Math.min(_maxCircle, per))
    }

    visible: QGroundControl.videoManager.hasVideo && _videoIsMain
             && (_hasLeftPayload || _hasRight)
             && !QGroundControl.videoManager.fullScreen
             && _showRecControl
             && !_compactOnlyExtras

    on_CameraChanged: expand = FlyViewPayloadOverlay.Expand.None

    function _toggleExpand(value) {
        expand = (expand === value) ? FlyViewPayloadOverlay.Expand.None : value
    }

    function _playSound(effect) {
        if (_audioMuted || _audioVolume <= 0 || !effect) {
            return
        }
        effect.volume = Math.max(0.05, Math.min(1.0, _audioVolume / 100.0))
        effect.play()
    }

    function _takePhoto() {
        if (!_camera) {
            return
        }
        _playSound(shutterSound)
        photoButton.triggerPhotoFlash()
        _camera.takePhoto()
    }

    function _toggleRecording() {
        if (!_camera) {
            return
        }
        if (_recording) {
            _playSound(recordStopSound)
        } else {
            _playSound(recordStartSound)
        }
        _camera.toggleVideoRecording()
    }

    SoundEffect {
        id: shutterSound
        source: "qrc:/res/audio/camera_shutter.wav"
    }

    SoundEffect {
        id: recordStartSound
        source: "qrc:/res/audio/record_start.wav"
    }

    SoundEffect {
        id: recordStopSound
        source: "qrc:/res/audio/record_stop.wav"
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.expand !== FlyViewPayloadOverlay.Expand.None
        z: 0
        onClicked: root.expand = FlyViewPayloadOverlay.Expand.None
    }

    // Left UniGCS strip — hug visible buttons; icon size and width match the right strip.
    Item {
        id: leftBar
        anchors.left: parent.left
        anchors.leftMargin: root._barEdgeMargin
        anchors.top: parent.top
        anchors.topMargin: root._barTopMargin
        readonly property real circleSize: root._sharedCircleSize
        width: root._sharedBarWidth
        height: leftCol.implicitHeight + root._barPadding * 2
        visible: root._hasLeftPayload
        z: 1

        Rectangle {
            anchors.fill: parent
            radius: root._barRadius
            color: Qt.rgba(0, 0, 0, 0.55)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.18)
        }

        Column {
            id: leftCol
            anchors.top: parent.top
            anchors.topMargin: root._barPadding
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root._barSpacing
            width: leftBar.circleSize

            FlyViewPayloadIconButton {
                width: leftCol.width
                visible: root._camera && root._camera.hasGimbalPad
                circleSize: leftBar.circleSize
                iconSource: "/InstrumentValueIcons/gimbal-2.svg"
                label: qsTr("Gimbal")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Gimbal
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Gimbal)
            }

            FlyViewPayloadIconButton {
                width: leftCol.width
                visible: root._camera && root._camera.hasLensSwitch
                circleSize: leftBar.circleSize
                iconSource: "/InstrumentValueIcons/camera.svg"
                label: qsTr("Lens")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Lens
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Lens)
            }

            FlyViewPayloadIconButton {
                width: leftCol.width
                visible: root._camera && root._camera.hasLaserRange
                circleSize: leftBar.circleSize
                iconSource: "/InstrumentValueIcons/radar.svg"
                label: qsTr("Range")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Range
                accentBlue: root._laserOn
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Range)
            }

            FlyViewPayloadIconButton {
                width: leftCol.width
                visible: root._camera && root._camera.hasAiRecognition
                circleSize: leftBar.circleSize
                iconSource: "/qmlimages/TrackingIcon.svg"
                label: qsTr("Recognize")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Recognize
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Recognize)
            }

            FlyViewPayloadIconButton {
                width: leftCol.width
                visible: root._camera && root._camera.hasFollowFlight
                circleSize: leftBar.circleSize
                iconSource: "/InstrumentValueIcons/drone.svg"
                label: qsTr("Follow")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Follow
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Follow)
            }
        }
    }

    Item {
        id: expandHost
        anchors.left: leftBar.right
        anchors.right: rightBar.left
        anchors.top: leftBar.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomReserve
        z: 1

        Column {
            id: gimbalQuickCol
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: root._barSpacing
            visible: root.expand === FlyViewPayloadOverlay.Expand.Gimbal
                     && root._camera && root._camera.hasGimbalPad

            readonly property int visibleQuickCount: {
                var n = 0
                if (root._camera && root._camera.hasGimbalRecenter) { n++ }
                if (root._camera && root._camera.hasGimbalLookDown) { n++ }
                if (root._camera && root._camera.hasGimbalYawRecenter) { n++ }
                if (root._camera && root._camera.hasGimbalPitchDown) { n++ }
                return n
            }
            readonly property real circleSize: Math.min(
                root._sharedCircleSize,
                root._fitCircleSize(expandHost.height, visibleQuickCount, 0))

            FlyViewPayloadIconButton {
                circleSize: gimbalQuickCol.circleSize
                visible: root._camera && root._camera.hasGimbalRecenter
                iconSource: "/InstrumentValueIcons/home.svg"
                label: qsTr("Recenter")
                onClicked: if (root._camera) { root._camera.gimbalRecenter() }
            }
            FlyViewPayloadIconButton {
                circleSize: gimbalQuickCol.circleSize
                visible: root._camera && root._camera.hasGimbalLookDown
                iconSource: "/InstrumentValueIcons/arrow-thick-down.svg"
                label: qsTr("Down")
                onClicked: if (root._camera) { root._camera.gimbalLookDown() }
            }
            FlyViewPayloadIconButton {
                circleSize: gimbalQuickCol.circleSize
                visible: root._camera && root._camera.hasGimbalYawRecenter
                iconSource: "/InstrumentValueIcons/reload.svg"
                label: qsTr("Yaw Recenter")
                onClicked: if (root._camera) { root._camera.gimbalYawRecenter() }
            }
            FlyViewPayloadIconButton {
                circleSize: gimbalQuickCol.circleSize
                visible: root._camera && root._camera.hasGimbalPitchDown
                iconSource: "/InstrumentValueIcons/arrow-simple-down.svg"
                label: qsTr("Pitch Down")
                onClicked: if (root._camera) { root._camera.gimbalPitchDown() }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: ScreenTools.defaultFontPixelWidth / 2
            visible: root.expand === FlyViewPayloadOverlay.Expand.Lens
                     && root._camera && root._camera.hasLensSwitch

            FlyViewPayloadIconButton {
                circleSize: root._sharedCircleSize
                iconSource: "/InstrumentValueIcons/camera.svg"
                label: qsTr("Zoom / IR")
                onClicked: {
                    if (root._camera) { root._camera.setVideoLayout(0, 2) }
                    root.expand = FlyViewPayloadOverlay.Expand.None
                }
            }
            FlyViewPayloadIconButton {
                circleSize: root._sharedCircleSize
                iconSource: "/InstrumentValueIcons/camera.svg"
                label: qsTr("IR / Zoom")
                onClicked: {
                    if (root._camera) { root._camera.setVideoLayout(2, 0) }
                    root.expand = FlyViewPayloadOverlay.Expand.None
                }
            }
            FlyViewPayloadIconButton {
                circleSize: root._sharedCircleSize
                iconSource: "/InstrumentValueIcons/camera.svg"
                label: qsTr("PIP / IR")
                onClicked: {
                    if (root._camera) { root._camera.setVideoLayout(3, 2) }
                    root.expand = FlyViewPayloadOverlay.Expand.None
                }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: ScreenTools.defaultFontPixelWidth / 2
            visible: root.expand === FlyViewPayloadOverlay.Expand.Range
                     && root._camera && root._camera.hasLaserRange

            FlyViewPayloadIconButton {
                circleSize: root._sharedCircleSize
                iconSource: "/InstrumentValueIcons/radar.svg"
                label: root._laserOn ? qsTr("Laser On") : qsTr("Laser Off")
                selected: root._laserOn
                accentBlue: root._laserOn
                onClicked: {
                    if (root._camera) {
                        root._camera.setLaserEnabled(!root._camera.laserEnabled)
                    }
                }
            }
            FlyViewPayloadIconButton {
                circleSize: root._sharedCircleSize
                iconSource: "/InstrumentValueIcons/radar.svg"
                label: qsTr("Measure")
                accentBlue: root._laserOn && root._laserDistanceValid
                onClicked: {
                    if (root._camera) {
                        root._camera.requestLaserDistance()
                    }
                }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: ScreenTools.defaultFontPixelWidth / 2
            visible: root.expand === FlyViewPayloadOverlay.Expand.Recognize
                     && root._camera && root._camera.hasAiRecognition

            FlyViewPayloadIconButton {
                circleSize: root._sharedCircleSize
                iconSource: "/qmlimages/TrackingIcon.svg"
                label: root._camera && root._camera.aiRecognitionEnabled ? qsTr("AI On") : qsTr("AI Off")
                selected: !!(root._camera && root._camera.aiRecognitionEnabled)
                onClicked: {
                    if (root._camera) {
                        root._camera.setAiRecognitionEnabled(!root._camera.aiRecognitionEnabled)
                    }
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            width: placeholderFollow.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
            height: placeholderFollow.implicitHeight + ScreenTools.defaultFontPixelHeight
            color: Qt.rgba(0, 0, 0, 0.55)
            radius: ScreenTools.defaultFontPixelWidth / 2
            visible: root.expand === FlyViewPayloadOverlay.Expand.Follow
                     && root._camera && root._camera.hasFollowFlight

            QGCLabel {
                id: placeholderFollow
                anchors.centerIn: parent
                text: qsTr("Follow flight — protocol pending")
                wrapMode: Text.WordWrap
            }
        }

        TopotekZoomHoldButtons {
            anchors.right: parent.right
            visible: root.expand === FlyViewPayloadOverlay.Expand.Zoom
            y: {
                var _ = zoomToggleButton.y + zoomToggleButton.height + rightBar.y + rightCol.y + height
                return expandHost.mapFromItem(zoomToggleButton, 0, 0).y
                       + (zoomToggleButton.height - height) / 2
            }
            camera: root._camera
            useFocus: false
            circleSize: root._sharedCircleSize
            spacing: root._barSpacing
            onZoomInteracted: {
                if (root._cameraManager) {
                    root._cameraManager.showSiyiZoomHud()
                }
            }
        }

        TopotekZoomHoldButtons {
            anchors.right: parent.right
            visible: root.expand === FlyViewPayloadOverlay.Expand.Focus
            y: {
                var _ = focusToggleButton.y + focusToggleButton.height + rightBar.y + rightCol.y + height
                return expandHost.mapFromItem(focusToggleButton, 0, 0).y
                       + (focusToggleButton.height - height) / 2
            }
            camera: root._camera
            useFocus: true
            circleSize: root._sharedCircleSize
            spacing: root._barSpacing
        }

        FactComboBox {
            anchors.right: parent.right
            anchors.top: parent.top
            visible: root.expand === FlyViewPayloadOverlay.Expand.Auto
                     && root._camera && root._camera.exposureMode
            fact: root._camera ? root._camera.exposureMode : null
            indexModel: false
            sizeToContents: true
        }

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            width: placeholderAuto.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
            height: placeholderAuto.implicitHeight + ScreenTools.defaultFontPixelHeight
            color: Qt.rgba(0, 0, 0, 0.55)
            radius: ScreenTools.defaultFontPixelWidth / 2
            visible: root.expand === FlyViewPayloadOverlay.Expand.Auto
                     && root._camera && root._camera.hasExposureAuto
                     && !root._camera.exposureMode

            QGCLabel {
                id: placeholderAuto
                anchors.centerIn: parent
                text: qsTr("AUTO exposure — protocol pending")
                wrapMode: Text.WordWrap
            }
        }
    }

    // Large HUD readout on video while laser is on
    Item {
        id: laserHud
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.75
        width: laserHudRow.width
        height: laserHudRow.height
        visible: root._laserOn
        z: 3

        Rectangle {
            anchors.fill: laserHudRow
            anchors.margins: -ScreenTools.defaultFontPixelWidth * 0.6
            radius: ScreenTools.defaultFontPixelWidth * 0.5
            color: Qt.rgba(0, 0, 0, 0.45)
            border.width: 1
            border.color: Qt.rgba(laserHudPal.colorBlue.r, laserHudPal.colorBlue.g, laserHudPal.colorBlue.b, 0.75)
        }

        Row {
            id: laserHudRow
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            QGCColoredImage {
                anchors.verticalCenter: parent.verticalCenter
                width: ScreenTools.defaultFontPixelHeight * 1.4
                height: width
                source: "/InstrumentValueIcons/radar.svg"
                sourceSize.height: height
                color: laserHudPal.colorBlue
                fillMode: Image.PreserveAspectFit
            }

            QGCLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: root._laserHudText
                color: laserHudPal.colorBlue
                font.pointSize: ScreenTools.largeFontPointSize * 1.35
                font.bold: true
            }
        }

        QGCPalette { id: laserHudPal; colorGroupEnabled: true }
    }

    // Zoom multiple HUD — shown for 5s after zoom in/out updates
    Item {
        id: zoomHud
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: ScreenTools.toolbarHeight + ScreenTools.defaultFontPixelHeight * 0.4
                     + (laserHud.visible ? laserHud.height + ScreenTools.defaultFontPixelHeight * 0.4 : 0)
        width: zoomHudLabel.width
        height: zoomHudLabel.height
        visible: root._zoomHudVisible
        z: 3

        Rectangle {
            anchors.fill: zoomHudLabel
            anchors.margins: -ScreenTools.defaultFontPixelWidth * 0.75
            radius: ScreenTools.defaultFontPixelWidth * 0.5
            color: Qt.rgba(0, 0, 0, 0.45)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.35)
        }

        QGCLabel {
            id: zoomHudLabel
            text: root._zoomHudText
            color: "white"
            font.pointSize: ScreenTools.largeFontPointSize * 1.6
            font.bold: true
        }
    }

    Connections {
        target: root._cameraManager
        function onSiyiZoomHudVisibleChanged(visible) {
            root._zoomHudVisible = visible
        }
        function onSiyiZoomHudTextChanged(text) {
            root._zoomHudText = text
        }
    }

    Timer {
        interval: 500
        running: root.visible && root._laserOn
        repeat: true
        onTriggered: {
            if (root._camera) {
                root._camera.requestLaserDistance()
            }
        }
    }

    // Right UniGCS strip — hug visible buttons; same width and icon size as left.
    Item {
        id: rightBar
        anchors.right: parent.right
        anchors.rightMargin: root._barEdgeMargin
        anchors.top: parent.top
        anchors.topMargin: root._barTopMargin
        readonly property real circleSize: root._sharedCircleSize
        width: root._sharedBarWidth
        height: rightCol.implicitHeight + root._barPadding * 2
        visible: root._hasRight
        z: 1

        Rectangle {
            anchors.fill: parent
            radius: root._barRadius
            color: Qt.rgba(0, 0, 0, 0.55)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.18)
        }

        Column {
            id: rightCol
            anchors.top: parent.top
            anchors.topMargin: root._barPadding
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root._barSpacing
            width: rightBar.circleSize

            FlyViewPayloadIconButton {
                width: rightCol.width
                visible: root._camera && (root._camera.hasExposureAuto || root._camera.exposureMode != null)
                circleSize: rightBar.circleSize
                iconSource: "/InstrumentValueIcons/brightness-down.svg"
                label: qsTr("AUTO")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Auto
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Auto)
            }

            FlyViewPayloadIconButton {
                id: photoButton
                width: rightCol.width
                visible: root._camera && root._camera.capturesPhotos
                circleSize: rightBar.circleSize
                iconSource: "/qmlimages/camera_photo.svg"
                label: qsTr("Photo")
                selected: false
                enabled: root._camera && root._camera.capturePhotosState !== MavlinkCameraControlInterface.CapturePhotosStateDisabled
                onClicked: root._takePhoto()
            }

            FlyViewPayloadIconButton {
                width: rightCol.width
                visible: root._camera && root._camera.capturesVideo
                circleSize: rightBar.circleSize
                iconSource: "/qmlimages/camera_video.svg"
                label: root._recording ? root._camera.recordTimeStr : qsTr("Video")
                recording: root._recording
                selected: root._recording
                enabled: root._camera && root._camera.captureVideoState !== MavlinkCameraControlInterface.CaptureVideoStateDisabled
                onClicked: root._toggleRecording()
            }

            FlyViewPayloadIconButton {
                id: zoomToggleButton
                width: rightCol.width
                visible: root._camera && root._camera.hasZoom
                circleSize: rightBar.circleSize
                iconSource: "/InstrumentValueIcons/zoom-in.svg"
                label: qsTr("Zoom")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Zoom
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Zoom)
            }

            FlyViewPayloadIconButton {
                id: focusToggleButton
                width: rightCol.width
                visible: root._camera && root._camera.hasFocus
                circleSize: rightBar.circleSize
                iconSource: "/InstrumentValueIcons/camera.svg"
                label: qsTr("Focus")
                selected: root.expand === FlyViewPayloadOverlay.Expand.Focus
                onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Focus)
            }

            FlyViewPayloadIconButton {
                width: rightCol.width
                visible: root._camera && root._camera.hasMediaLibrary
                circleSize: rightBar.circleSize
                iconSource: "/res/SaveToDisk.svg"
                label: qsTr("Save")
                selected: false
                onClicked: {
                    if (root._unipodMediaClient) {
                        mediaGalleryFactory.open({ mediaClient: root._unipodMediaClient })
                    }
                }
            }
        }
    }

    QGCPopupDialogFactory {
        id: mediaGalleryFactory
        dialogComponent: mediaGalleryComponent
    }

    Component {
        id: mediaGalleryComponent
        UnipodMt11MediaGallery {
        }
    }
}
