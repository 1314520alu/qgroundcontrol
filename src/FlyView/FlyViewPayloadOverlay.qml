import QtQuick

import QGroundControl
import QGroundControl.Controls
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
    // Prefer a camera that can actually drive PhotoVideoControl. Manual streams (RTSP/UDP)
    // use SimulatedCameraControl for local VideoManager record; a MAVLink camera without
    // capture flags must not hide the UI. UniPod MT11 must never fall through to Simulated.
    property var _camera: {
        if (!_cameraManager) {
            return null
        }
        var current = _cameraManager.currentCameraInstance
        if (current && current.modelName === "UniPod MT11") {
            return current
        }
        if (current && current.modelName === "Topotek TQ10N") {
            return current
        }
        if (current && (current.capturesVideo || current.capturesPhotos || current.hasTracking || current.hasVideoStream)) {
            return current
        }
        var cams = _cameraManager.cameras
        if (cams) {
            for (var i = 0; i < cams.count; i++) {
                var c = cams.get(i)
                if (c && (c.capturesVideo || c.capturesPhotos || c.hasTracking || c.hasVideoStream)) {
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
        (_camera.exposureMode != null)
        || _camera.capturesPhotos
        || _camera.capturesVideo
        || _camera.hasZoom
        || _camera.hasFocus
        || _camera.hasMediaLibrary)
    readonly property bool overlayActive: visible && (_hasLeft || _hasRight)

    visible: QGroundControl.videoManager.hasVideo && _videoIsMain && (_hasLeft || _hasRight)
             && !QGroundControl.videoManager.fullScreen

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
    }

    FlyViewPayloadSideBar {
        id: rightBar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight
        visible: root._hasRight
        z: 1

        FlyViewPayloadIconButton {
            visible: root._camera && (root._camera.exposureMode != null)
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
            onClicked: {
                if (root._camera) {
                    root._camera.takePhoto()
                }
            }
        }

        FlyViewPayloadIconButton {
            visible: root._camera && root._camera.capturesVideo
            iconSource: "/qmlimages/camera_video.svg"
            label: qsTr("Video")
            selected: false
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
            }
        }
    }
}
