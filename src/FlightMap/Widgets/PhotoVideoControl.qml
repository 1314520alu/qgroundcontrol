import QtQuick
import QtPositioning
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.FlyView

Rectangle {
    id: photoVideoControl
    width: mainLayout.width + (_smallMargins * 2)
    height: mainLayout.height + (_smallMargins * 2)
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
    radius: _margins
    visible: !hideWhenPayloadOverlay && _showPhotoVideoControls && _camera && (_camera.capturesVideo || _camera.capturesPhotos || _camera.hasTracking || _camera.hasVideoStream)

    property bool hideWhenPayloadOverlay: false
    property real _margins: ScreenTools.defaultFontPixelHeight / 2
    property real _smallMargins: ScreenTools.defaultFontPixelWidth / 2
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
    property var _videoSettings: QGroundControl.settingsManager.videoSettings
    property bool _showPhotoVideoControls: _videoSettings ? _videoSettings.showRecControl.rawValue : true
    property bool _cameraInPhotoMode: _camera && (_camera.cameraMode === MavlinkCameraControlInterface.CAM_MODE_PHOTO || _camera.cameraMode === MavlinkCameraControlInterface.CAM_MODE_SURVEY)
    property bool _cameraInVideoMode: !_cameraInPhotoMode
    property bool _videoCaptureIdle: _camera && _camera.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateIdle
    property bool _photoCaptureIdle: _camera && _camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateIdle
    property var _unipodMediaClient: _cameraManager ? _cameraManager.unipodMediaClient : null
    // Show whenever MT11 camera is active; enable when HTTP client is ready.
    // Keep the control above Video so short landscape remotes do not clip it.
    property bool _showMediaLibrary: _camera && _camera.modelName === "UniPod MT11" && _unipodMediaClient
    property bool _mediaLibraryReady: _unipodMediaClient && _unipodMediaClient.ready
    property bool _useTopotekSplitStrip: _camera && (_camera.hasGimbalPad || _camera.modelName === "Topotek TQ10N")

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    DeadMouseArea { anchors.fill: parent }

    RowLayout {
        id: mainLayout
        anchors.margins: _smallMargins
        anchors.top: parent.top
        anchors.left: parent.left
        spacing: _margins

        TopotekGimbalPad {
            visible: _useTopotekSplitStrip
            camera: _camera
        }

        ColumnLayout {
            Layout.fillHeight: true
            spacing: 0
            visible: _camera && _camera.hasZoom && !_useTopotekSplitStrip

            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Zoom")
                font.pointSize: ScreenTools.smallFontPointSize
            }

            QGCSlider {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                orientation: Qt.Vertical
                to: 100
                from: 0
                value: _camera ? _camera.zoomLevel : 0
                live: true
                onValueChanged: if (_camera) { _camera.zoomLevel = value }
            }
        }

        ColumnLayout {
            spacing: _margins

            // Camera name
            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: _camera ? _camera.modelName : ""
                visible: _cameraManager && _cameraManager.cameras.count > 1
            }

            // Photo/Video Mode Selector
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: ScreenTools.defaultFontPixelWidth * 10
                height: width / 2
                color: qgcPal.windowShadeLight
                radius: height * 0.5
                visible: _camera && _camera.hasModes

                //-- Video Mode
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.height
                    height: parent.height
                    color: _cameraInVideoMode ? qgcPal.window : qgcPal.windowShadeLight
                    radius: height * 0.5
                    anchors.left: parent.left
                    border.color: qgcPal.text
                    border.width: _cameraInPhotoMode ? 0 : 1

                    QGCColoredImage {
                        height: parent.height * 0.5
                        width: height
                        anchors.centerIn: parent
                        source: "/qmlimages/camera_video.svg"
                        fillMode: Image.PreserveAspectFit
                        sourceSize.height: height
                        color: _cameraInVideoMode ? qgcPal.colorGreen : qgcPal.text

                        MouseArea {
                            anchors.fill: parent
                            enabled: _cameraInPhotoMode ? _photoCaptureIdle : true
                            onClicked: _camera.setCameraModeVideo()
                        }
                    }
                }

                //-- Photo Mode
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.height
                    height: parent.height
                    color: _cameraInPhotoMode ? qgcPal.window : qgcPal.windowShadeLight
                    radius: height * 0.5
                    anchors.right: parent.right
                    border.color: qgcPal.text
                    border.width: _cameraInPhotoMode ? 1 : 0

                    QGCColoredImage {
                        height: parent.height * 0.5
                        width: height
                        anchors.centerIn: parent
                        source: "/qmlimages/camera_photo.svg"
                        fillMode: Image.PreserveAspectFit
                        sourceSize.height: height
                        color: _cameraInPhotoMode ? qgcPal.colorGreen : qgcPal.text

                        MouseArea {
                            anchors.fill: parent
                            enabled: _cameraInVideoMode ? _videoCaptureIdle : true
                            onClicked: _camera.setCameraModePhoto()
                        }
                    }
                }
            }

            // Start/Stop Video button
            Rectangle {
                id: videoCaptureButton
                Layout.alignment: Qt.AlignHCenter
                color: videoCaptureButtonPalette.button
                width: ScreenTools.defaultFontPixelWidth * (_showMediaLibrary ? 5 : 6)
                height: width
                radius: width * 0.5
                border.width: 1
                border.color: videoCaptureButtonPalette.buttonBorder
                visible: _camera && ((_camera.hasModes && _cameraInVideoMode) || (!_camera.hasModes && _camera.capturesVideo))
                enabled: _camera && _camera.captureVideoState !== MavlinkCameraControlInterface.CaptureVideoStateDisabled

                QGCPalette { id: videoCaptureButtonPalette; colorGroupEnabled: videoCaptureButton.enabled }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false // Prevents anchors.centerIn from snapping to integer coordinates, which can throw off centering.
                    color: videoCaptureButtonPalette.buttonBorder
                    width: parent.width * 0.75
                    height: width
                    radius: width * 0.5
                }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false // Prevents anchors.centerIn from snapping to integer coordinates, which can throw off centering.
                    width: parent.width * (_isCapturing ? 0.5 : 0.75)
                    height: width
                    radius: _isCapturing ? ScreenTools.defaultFontPixelWidth * 0.5 : width * 0.5
                    color: videoCaptureButtonPalette.videoCaptureButtonColor
                    border.width: 1
                    border.color: videoCaptureButtonPalette.buttonBorder

                    property bool _isCapturing: _camera && _camera.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateCapturing
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: if (_camera) { _camera.toggleVideoRecording() }
                }
            }

            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Video")
                font.pointSize: ScreenTools.smallFontPointSize
                visible: videoCaptureButton.visible && photoCaptureButton.visible && !_showMediaLibrary
            }

            // Record time
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                color: _videoCaptureIdle ? "transparent" : videoCaptureButtonPalette.videoCaptureButtonColor
                Layout.preferredWidth: videoRecordTime.width + (_smallMargins * 2)
                Layout.preferredHeight: videoRecordTime.height
                radius: _smallMargins
                // Hide idle timer on UniPod — short landscape needs room for Browse + settings
                visible: videoCaptureButton.visible && (!_showMediaLibrary || !_videoCaptureIdle)

                // Video record time
                QGCLabel {
                    id: videoRecordTime
                    anchors.leftMargin: _smallMargins
                    anchors.left: parent.left
                    anchors.top: parent.top
                    text: _videoCaptureIdle ? "00:00:00" : (_camera ? _camera.recordTimeStr : "00:00:00")
                }
            }

            Item {
                Layout.alignment: Qt.AlignHCenter
                width: 1
                height: 1
                visible: videoCaptureButton.visible && photoCaptureButton.visible && !_showMediaLibrary
            }

            // Take Photo button
            Rectangle {
                id: photoCaptureButton
                Layout.alignment: Qt.AlignHCenter
                color: photoCaptureButtonPalette.button
                width: ScreenTools.defaultFontPixelWidth * (_showMediaLibrary ? 5 : 6)
                height: width
                radius: width * 0.5
                border.width: 1
                border.color: photoCaptureButtonPalette.buttonBorder
                visible: _camera && ((_camera.hasModes && _cameraInPhotoMode) || (!_camera.hasModes && (_camera.hasVideoStream || _camera.capturesPhotos)))
                enabled: _camera && _camera.capturePhotosState !== MavlinkCameraControlInterface.CapturePhotosStateDisabled

                QGCPalette { id: photoCaptureButtonPalette; colorGroupEnabled: photoCaptureButton.enabled }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false // Prevents anchors.centerIn from snapping to integer coordinates, which can throw off centering.
                    color: photoCaptureButtonPalette.buttonBorder
                    width: parent.width * 0.75
                    height: width
                    radius: width * 0.5
                }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false // Prevents anchors.centerIn from snapping to integer coordinates, which can throw off centering.
                    width: parent.width * (_isCapturing ? 0.5 : 0.75)
                    height: width
                    radius: _isCapturing ? ScreenTools.defaultFontPixelWidth * 0.5 : width * 0.5
                    color: photoCaptureButtonPalette.photoCaptureButtonColor
                    border.width: 1
                    border.color: photoCaptureButtonPalette.buttonBorder

                    property bool _isCapturing: _camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateCapturingSinglePhoto ||
                                                    _camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateCapturingMultiplePhotos
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (_camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateCapturingMultiplePhotos) {
                            _camera.stopTakePhoto()
                        } else if (_camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateIdle) {
                            _camera.takePhoto()
                        }
                    }
                }
            }

            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Photo")
                font.pointSize: ScreenTools.smallFontPointSize
                visible: videoCaptureButton.visible && photoCaptureButton.visible && !_showMediaLibrary
            }

            TopotekZoomHoldButtons {
                Layout.alignment: Qt.AlignHCenter
                visible: _useTopotekSplitStrip
                camera: _camera
            }

            // Capture count (not useful for UniPod onboard TF — Browse is beside column)
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                color: _photoCaptureIdle ? "transparent" : photoCaptureButtonPalette.photoCaptureButtonColor
                Layout.preferredWidth: photoCaptureCount.width + (_smallMargins * 2)
                Layout.preferredHeight: photoCaptureCount.height
                radius: _smallMargins
                visible: photoCaptureButton.visible && !_showMediaLibrary

                QGCLabel {
                    id: photoCaptureCount
                    anchors.leftMargin: _smallMargins
                    anchors.left: parent.left
                    anchors.top: parent.top
                    text: _activeVehicle ? ('00000' + _activeVehicle.cameraTriggerPoints.count).slice(-5) : "00000"
                }
            }

            //-- Status Information
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 0
                visible: storageStatus.visible || batteryStatus.visible

                QGCLabel {
                    id: storageStatus
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Free: ") + _camera.storageFreeStr
                    font.pointSize: ScreenTools.defaultFontPointSize
                    visible: _camera.storageStatus === MavlinkCameraControlInterface.STORAGE_READY
                }

                QGCLabel {
                    id: batteryStatus
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Battery: ") + _camera.batteryRemainingStr
                    font.pointSize: ScreenTools.defaultFontPointSize
                    visible: _camera.batteryRemaining >= 0
                }
            }

            ColumnLayout {
                id: trackingControls
                Layout.alignment: Qt.AlignHCenter
                spacing: 0
                visible: _camera.hasTracking

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    color: _camera.trackingEnabled ? qgcPal.colorRed : qgcPal.windowShadeLight
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                    Layout.preferredHeight: Layout.preferredWidth
                    border.color: qgcPal.buttonText
                    border.width: 3

                    QGCColoredImage {
                        height: parent.height * 0.5
                        width: height
                        anchors.centerIn: parent
                        source: "/qmlimages/TrackingIcon.svg"
                        fillMode: Image.PreserveAspectFit
                        sourceSize.height: height
                        color: qgcPal.text

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                _camera.trackingEnabled = !_camera.trackingEnabled;
                                if (!_camera.trackingEnabled) {
                                    _camera.stopTracking()
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Camera Tracking")
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }

            // Browse above settings — same column width as capture buttons (no side-by-side overflow)
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: _smallMargins

                QGCColoredImage {
                    Layout.alignment: Qt.AlignHCenter
                    visible: _showMediaLibrary
                    enabled: _mediaLibraryReady
                    opacity: _mediaLibraryReady ? 1.0 : 0.45
                    source: "/res/SaveToDisk.svg"
                    mipmap: true
                    Layout.preferredHeight: Math.max(ScreenTools.minTouchPixels * 0.55, ScreenTools.defaultFontPixelHeight * 1.5)
                    Layout.preferredWidth: Layout.preferredHeight
                    sourceSize.height: Layout.preferredHeight
                    color: qgcPal.text
                    fillMode: Image.PreserveAspectFit

                    QGCMouseArea {
                        fillItem: parent
                        enabled: _mediaLibraryReady
                        onClicked: mediaGalleryFactory.open({ mediaClient: _unipodMediaClient })
                    }
                }

                QGCColoredImage {
                    Layout.alignment: Qt.AlignHCenter
                    source: "/res/gear-black.svg"
                    mipmap: true
                    Layout.preferredHeight: Math.max(ScreenTools.minTouchPixels * 0.55, ScreenTools.defaultFontPixelHeight * 1.5)
                    Layout.preferredWidth: Layout.preferredHeight
                    sourceSize.height: Layout.preferredHeight
                    color: qgcPal.text
                    fillMode: Image.PreserveAspectFit

                    QGCMouseArea {
                        fillItem: parent
                        onClicked: settingsDialogFactory.open()
                    }
                }
            }
        }

        QGCPopupDialogFactory {
            id: settingsDialogFactory

            dialogComponent: settingsDialogComponent
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

        Component {
            id: settingsDialogComponent

            QGCPopupDialog {
                title: qsTr("Settings")
                buttons: Dialog.Close

                property bool _multipleMavlinkCameras: _cameraManager.cameras.count > 1
                property bool _multipleMavlinkCameraStreams: _camera.streamLabels.length > 1
                property bool _cameraStorageSupported: _camera.storageStatus !== MavlinkCameraControlInterface.STORAGE_NOT_SUPPORTED
                property var _videoSettings: QGroundControl.settingsManager.videoSettings

                ColumnLayout {
                    spacing: _margins

                    GridLayout {
                        id: gridLayout
                        flow: GridLayout.TopToBottom
                        rows: dynamicRows + _camera.activeSettings.length

                        property int dynamicRows: 10

                        // First column
                        QGCLabel {
                            text: qsTr("Camera")
                            visible: _multipleMavlinkCameras
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Video Stream")
                            visible: _multipleMavlinkCameraStreams
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Thermal View Mode")
                            visible: _camera.thermalStreamInstance
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Blend Opacity")
                            visible: _camera.thermalStreamInstance && _camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        // Mavlink Camera Protocol active settings
                        Repeater {
                            model: _camera.activeSettings

                            QGCLabel {
                                text: _camera.getFact(modelData).shortDescription
                            }
                        }

                        QGCLabel {
                            text: qsTr("Photo Mode")
                            visible: _camera.capturesPhotos
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Photo Interval (seconds)")
                            visible: _camera.capturesPhotos && _camera.photoCaptureMode === MavlinkCameraControlInterface.PHOTO_CAPTURE_TIMELAPSE
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Video Grid Lines")
                            visible: _camera.hasVideoStream
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Video Screen Fit")
                            visible: _camera.hasVideoStream
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Reset Camera Defaults")
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text: qsTr("Storage")
                            visible: _cameraStorageSupported
                            onVisibleChanged: gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        // Second column
                        QGCComboBox {
                            Layout.fillWidth: true
                            sizeToContents: true
                            model: _cameraManager.cameraLabels
                            currentIndex: _cameraManager.currentCamera
                            visible: _multipleMavlinkCameras
                            onActivated: (index) => { _cameraManager.currentCamera = index }
                        }

                        QGCComboBox {
                            Layout.fillWidth: true
                            sizeToContents: true
                            model: _camera.streamLabels
                            currentIndex: _camera.currentStream
                            visible: _multipleMavlinkCameraStreams
                            onActivated: (index) => { _camera.currentStream = index }
                        }

                        QGCComboBox {
                            Layout.fillWidth: true
                            sizeToContents: true
                            model: [ qsTr("Off"), qsTr("Blend"), qsTr("Full"), qsTr("Picture In Picture") ]
                            currentIndex: _camera.thermalMode
                            visible: _camera.thermalStreamInstance
                            onActivated: (index) => { _camera.thermalMode = index }
                        }

                        QGCSlider {
                            Layout.fillWidth: true
                            to: 100
                            from: 0
                            value: _camera.thermalOpacity
                            live: true
                            visible: _camera.thermalStreamInstance && _camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND
                            onValueChanged: _camera.thermalOpacity = value
                        }

                        // Mavlink Camera Protocol active settings
                        Repeater {
                            model: _camera.activeSettings

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: ScreenTools.defaultFontPixelWidth

                                property var _fact: _camera.getFact(modelData)
                                property bool _isBool: _fact.typeIsBool
                                property bool _isCombo: !_isBool && _fact.enumStrings.length > 0
                                property bool _isSlider: _fact && !isNaN(_fact.increment)
                                property bool _isEdit: !_isBool && !_isSlider && _fact.enumStrings.length < 1
                                property bool _isReadOnly: _fact && _fact.readOnly

                                FactComboBox {
                                    Layout.fillWidth: true
                                    sizeToContents: true
                                    fact: parent._fact
                                    indexModel: false
                                    visible: parent._isCombo
                                    enabled: !parent._isReadOnly
                                }
                                FactTextField {
                                    Layout.fillWidth: true
                                    fact: parent._fact
                                    visible: parent._isEdit
                                    enabled: !parent._isReadOnly
                                }
                                QGCSlider {
                                    Layout.fillWidth: true
                                    to: parent._fact.max
                                    from: parent._fact.min
                                    stepSize: parent._fact.increment
                                    visible: parent._isSlider
                                    enabled: !parent._isReadOnly
                                    live: false
                                    property bool initialized: false

                                    onValueChanged: {
                                        if (!initialized) {
                                            return
                                        }
                                        parent._fact.value = value
                                    }

                                    Component.onCompleted: {
                                        value = parent._fact.value
                                        initialized = true
                                    }
                                }
                                QGCCheckBoxSlider {
                                    checked: parent._fact ? parent._fact.value : false
                                    visible: parent._isBool
                                    enabled: !parent._isReadOnly
                                    onClicked: parent._fact.value = checked ? 1 : 0
                                }
                            }
                        }

                        QGCComboBox {
                            Layout.fillWidth: true
                            sizeToContents: true
                            model: [ qsTr("Single"), qsTr("Time Lapse") ]
                            currentIndex: _camera.photoCaptureMode
                            visible: _camera.capturesPhotos
                            onActivated: (index) => { _camera.photoCaptureMode = index }
                        }

                        QGCSlider {
                            Layout.fillWidth: true
                            to: 60
                            from: 1
                            stepSize: 1
                            value: _camera.photoLapse
                            displayValue: true
                            live: true
                            visible: _camera.capturesPhotos && _camera.photoCaptureMode === MavlinkCameraControlInterface.PHOTO_CAPTURE_TIMELAPSE
                            onValueChanged: _camera.photoLapse = value
                        }

                        QGCCheckBoxSlider {
                            checked: _videoSettings.gridLines.rawValue
                            visible: _camera.hasVideoStream
                            onClicked: _videoSettings.gridLines.rawValue = checked ? 1 : 0
                        }

                        FactComboBox {
                            Layout.fillWidth: true
                            sizeToContents: true
                            fact: _videoSettings.videoFit
                            indexModel: false
                            visible: _camera.hasVideoStream
                        }

                        QGCButton {
                            Layout.fillWidth: true
                            text: qsTr("Reset")
                            onClicked: resetPrompt.open()
                            MessageDialog {
                                id: resetPrompt
                                title: qsTr("Reset Camera to Factory Settings")
                                text: qsTr("Confirm resetting all settings?")
                                buttons: MessageDialog.Yes | MessageDialog.No

                                onButtonClicked: function (button, role) {
                                    switch (button) {
                                    case MessageDialog.Yes:
                                        _camera.resetSettings()
                                        resetPrompt.close()
                                        break;
                                    case MessageDialog.No:
                                        resetPrompt.close()
                                        break;
                                    }
                                }
                            }
                        }

                        QGCButton {
                            Layout.fillWidth: true
                            text: qsTr("Format")
                            visible: _cameraStorageSupported
                            onClicked: formatPrompt.open()
                            MessageDialog {
                                id: formatPrompt
                                title: qsTr("Format Camera Storage")
                                text: qsTr("Confirm erasing all files?")
                                buttons: MessageDialog.Yes | MessageDialog.No

                                onButtonClicked: function (button, role) {
                                    switch (button) {
                                    case MessageDialog.Yes:
                                        _camera.formatCard()
                                        formatPrompt.close()
                                        break;
                                    case MessageDialog.No:
                                        formatPrompt.close()
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
