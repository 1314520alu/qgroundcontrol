import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.FlyView

Item {
    id: root

    property var mapControl
    property real toolStripWidth: 0
    property real bottomReserve: ScreenTools.defaultFontPixelHeight * 8

    enum Expand { None, Gimbal, Lens, Range, Recognize, Follow, Auto, Zoom, Focus }

    property int expand: FlyViewPayloadOverlay.Expand.None

    property var _activeVehicle: globals.activeVehicle
    property var _cameraManager: _activeVehicle ? _activeVehicle.cameraManager : null
    property var _unipodMediaClient: _cameraManager ? _cameraManager.unipodMediaClient : null
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
    readonly property bool _hasLeft: _camera && (_camera.hasGimbalPad
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
    readonly property bool overlayActive: visible && (_hasLeft || _hasRight)

    visible: QGroundControl.videoManager.hasVideo && _videoIsMain && (_hasLeft || _hasRight)
             && !QGroundControl.videoManager.fullScreen
             && _showRecControl
             && !_compactOnlyExtras

    on_CameraChanged: expand = FlyViewPayloadOverlay.Expand.None

    function _toggleExpand(value) {
        expand = (expand === value) ? FlyViewPayloadOverlay.Expand.None : value
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.expand !== FlyViewPayloadOverlay.Expand.None
        z: 0
        onClicked: root.expand = FlyViewPayloadOverlay.Expand.None
    }

    FlyViewPayloadSideBar {
        id: leftBar
        anchors.left: parent.left
        anchors.leftMargin: root.toolStripWidth + ScreenTools.defaultFontPixelWidth * 0.75
        anchors.top: parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight
        visible: root._hasLeft
        z: 1

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasGimbalPad
            iconSource: "/InstrumentValueIcons/gimbal-2.svg"
            label: qsTr("Gimbal")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Gimbal
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Gimbal)
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasLensSwitch
            iconSource: "/InstrumentValueIcons/camera.svg"
            label: qsTr("Lens")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Lens
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Lens)
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasLaserRange
            iconSource: "/InstrumentValueIcons/radar.svg"
            label: qsTr("Range")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Range
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Range)
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasAiRecognition
            iconSource: "/qmlimages/TrackingIcon.svg"
            label: qsTr("Recognize")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Recognize
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Recognize)
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasFollowFlight
            iconSource: "/InstrumentValueIcons/drone.svg"
            label: qsTr("Follow")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Follow
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Follow)
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

        TopotekGimbalPad {
            anchors.left: parent.left
            anchors.top: parent.top
            visible: root.expand === FlyViewPayloadOverlay.Expand.Gimbal
                     && root._camera && root._camera.hasGimbalPad
            camera: root._camera
        }

        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: ScreenTools.defaultFontPixelWidth / 2
            visible: root.expand === FlyViewPayloadOverlay.Expand.Lens
                     && root._camera && root._camera.hasLensSwitch

            FlyViewPayloadIconButton {
                iconSource: "/InstrumentValueIcons/camera.svg"
                label: qsTr("Zoom / IR")
                onClicked: {
                    if (root._camera) { root._camera.setVideoLayout(0, 2) }
                    root.expand = FlyViewPayloadOverlay.Expand.None
                }
            }
            FlyViewPayloadIconButton {
                iconSource: "/InstrumentValueIcons/camera.svg"
                label: qsTr("IR / Zoom")
                onClicked: {
                    if (root._camera) { root._camera.setVideoLayout(2, 0) }
                    root.expand = FlyViewPayloadOverlay.Expand.None
                }
            }
            FlyViewPayloadIconButton {
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
                iconSource: "/InstrumentValueIcons/radar.svg"
                label: root._camera && root._camera.laserEnabled ? qsTr("Laser On") : qsTr("Laser Off")
                selected: root._camera && root._camera.laserEnabled
                onClicked: {
                    if (root._camera) {
                        root._camera.setLaserEnabled(!root._camera.laserEnabled)
                    }
                }
            }
            FlyViewPayloadIconButton {
                iconSource: "/InstrumentValueIcons/radar.svg"
                label: {
                    if (!root._camera) { return qsTr("Range") }
                    var d = root._camera.laserDistanceMeters
                    if (d !== d) { return qsTr("Range") } // NaN
                    return Number(d).toFixed(1) + " m"
                }
                onClicked: {
                    if (root._camera) { root._camera.requestLaserDistance() }
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
                iconSource: "/qmlimages/TrackingIcon.svg"
                label: root._camera && root._camera.aiRecognitionEnabled ? qsTr("AI On") : qsTr("AI Off")
                selected: root._camera && root._camera.aiRecognitionEnabled
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
            anchors.top: parent.top
            visible: root.expand === FlyViewPayloadOverlay.Expand.Zoom
            camera: root._camera
            useFocus: false
        }

        TopotekZoomHoldButtons {
            anchors.right: parent.right
            anchors.top: parent.top
            visible: root.expand === FlyViewPayloadOverlay.Expand.Focus
            camera: root._camera
            useFocus: true
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

    FlyViewPayloadSideBar {
        id: rightBar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight
        visible: root._hasRight
        z: 1

        FlyViewPayloadIconButton {
            visible: root._camera && (root._camera.hasExposureAuto || root._camera.exposureMode != null)
            iconSource: "/InstrumentValueIcons/brightness-down.svg"
            label: qsTr("AUTO")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Auto
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Auto)
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.capturesPhotos
            iconSource: "/qmlimages/camera_photo.svg"
            label: qsTr("Photo")
            selected: false
            enabled: root._camera && root._camera.capturePhotosState !== MavlinkCameraControlInterface.CapturePhotosStateDisabled
            onClicked: {
                if (root._camera) {
                    root._camera.takePhoto()
                }
            }
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.capturesVideo
            iconSource: "/qmlimages/camera_video.svg"
            label: (root._camera && root._camera.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateCapturing)
                   ? root._camera.recordTimeStr : qsTr("Video")
            selected: root._camera && root._camera.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateCapturing
            enabled: root._camera && root._camera.captureVideoState !== MavlinkCameraControlInterface.CaptureVideoStateDisabled
            onClicked: {
                if (root._camera) {
                    root._camera.toggleVideoRecording()
                }
            }
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasZoom
            iconSource: "/InstrumentValueIcons/zoom-in.svg"
            label: qsTr("Zoom")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Zoom
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Zoom)
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasFocus
            iconSource: "/InstrumentValueIcons/camera.svg"
            label: qsTr("Focus")
            selected: root.expand === FlyViewPayloadOverlay.Expand.Focus
            onClicked: root._toggleExpand(FlyViewPayloadOverlay.Expand.Focus)
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.hasMediaLibrary
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
