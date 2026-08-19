import QtQuick
import QtQuick.Layouts
import QtMultimedia

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

/// Local VideoManager record / snapshot controls for manual streams (UniPod/RTSP/UDP).
/// Visual language matches PhotoVideoControl so Fly View stays consistent on short landscape remotes.
Rectangle {
    id: root

    property var _videoManager: QGroundControl.videoManager
    property var _videoSettings: QGroundControl.settingsManager.videoSettings
    property var _appSettings: QGroundControl.settingsManager.appSettings
    property var _activeVehicle: globals.activeVehicle
    /// Set by parent when PhotoVideoControl is already showing the same UI.
    property bool hideWhenPhotoVideoVisible: false
    property bool _isUnipodSource: _videoSettings && (_videoSettings.videoSource.rawValue === _videoSettings.unipodMT11VideoSource)
    property bool _isTopotekSource: _videoSettings && (_videoSettings.videoSource.rawValue === _videoSettings.topotekTq10NVideoSource)
    property var _cameraManager: _activeVehicle ? _activeVehicle.cameraManager : null
    property var _unipodCam: {
        if (!_isUnipodSource || !_cameraManager) {
            return null
        }
        const cam = _cameraManager.currentCameraInstance
        return (cam && cam.modelName === "UniPod MT11") ? cam : null
    }
    property var _topotekCam: {
        if (!_isTopotekSource || !_cameraManager) {
            return null
        }
        const cam = _cameraManager.currentCameraInstance
        return (cam && cam.modelName === "Topotek TQ10N") ? cam : null
    }
    property var _onboardCam: _unipodCam || _topotekCam
    property var _unipodMediaClient: _cameraManager ? _cameraManager.unipodMediaClient : null
    property bool _showMediaLibrary: _isUnipodSource && _unipodMediaClient
    property bool _mediaLibraryReady: _unipodMediaClient && _unipodMediaClient.ready
    // UniPod normally uses PhotoVideoControl (onboard) once a vehicle/cameraManager exists.
    // When that strip is not loaded (typically disconnected), keep this fallback visible so
    // Fly View does not lose photo/record controls entirely.
    // Prefer PhotoVideoControl when its Loader reports visible.
    property bool _show: _videoSettings && _videoSettings.showRecControl.rawValue
                         && (_videoManager.hasVideo || _isUnipodSource || _isTopotekSource)
                         && !hideWhenPhotoVideoVisible
    property bool _recording: _onboardCam
                              ? (_onboardCam.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateCapturing)
                              : _videoManager.recording
    property bool _audioMuted: _appSettings && _appSettings.audioMuted.rawValue
    property real _audioVolume: _appSettings ? _appSettings.audioVolume.rawValue : 50
    property real _margins: ScreenTools.defaultFontPixelHeight / 2
    property real _smallMargins: ScreenTools.defaultFontPixelWidth / 2
    property real _buttonSize: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 6) * 2 / 3
    property int _recordElapsedMs: 0
    property bool _photoPressed: false

    visible: _show
    width: mainLayout.width + (_smallMargins * 2)
    height: mainLayout.height + (_smallMargins * 2)
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.55)
    radius: _margins
    border.width: _recording ? 2 : 1
    border.color: _recording
                 ? Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.55 + 0.35 * recPulse.value)
                 : Qt.rgba(qgcPal.buttonBorder.r, qgcPal.buttonBorder.g, qgcPal.buttonBorder.b, 0.35)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function _formatRecordTime(ms) {
        const totalSec = Math.floor(ms / 1000)
        const h = Math.floor(totalSec / 3600)
        const m = Math.floor((totalSec % 3600) / 60)
        const s = totalSec % 60
        function pad(v) { return v < 10 ? "0" + v : "" + v }
        return pad(h) + ":" + pad(m) + ":" + pad(s)
    }

    function _playSound(effect) {
        if (_audioMuted || _audioVolume <= 0 || !effect) {
            return
        }
        effect.volume = Math.max(0.05, Math.min(1.0, _audioVolume / 100.0))
        effect.play()
    }

    function _toggleRecording() {
        if (_onboardCam) {
            if (_recording) {
                _playSound(recordStopSound)
            } else {
                _playSound(recordStartSound)
            }
            _onboardCam.toggleVideoRecording()
            return
        }
        if (_recording) {
            _playSound(recordStopSound)
            _videoManager.stopRecording()
        } else {
            _playSound(recordStartSound)
            _videoManager.startRecording()
        }
    }

    function _takeSnapshot() {
        _playSound(shutterSound)
        photoFlashAnim.restart()
        if (_onboardCam) {
            _onboardCam.takePhoto()
            return
        }
        _videoManager.grabImage()
    }

    on_RecordingChanged: {
        if (_recording) {
            _recordElapsedMs = 0
            recordElapsedTimer.restart()
            recPulse.start()
        } else {
            recordElapsedTimer.stop()
            _recordElapsedMs = 0
            recPulse.stop()
            recPulse.value = 0
        }
    }

    Timer {
        id: recordElapsedTimer
        interval: 250
        repeat: true
        onTriggered: root._recordElapsedMs += interval
    }

    NumberAnimation {
        id: recPulse
        target: recPulse
        property: "value"
        from: 0
        to: 1
        duration: 700
        loops: Animation.Infinite
        running: false
        property real value: 0
        easing.type: Easing.InOutSine
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

    DeadMouseArea { anchors.fill: parent }

    // Snapshot flash overlay
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "white"
        opacity: photoFlashAnim.running ? photoFlashAnim.opacityValue : 0
        z: 10
        visible: opacity > 0.01
    }

    SequentialAnimation {
        id: photoFlashAnim
        property real opacityValue: 0
        NumberAnimation {
            target: photoFlashAnim
            property: "opacityValue"
            from: 0.65
            to: 0
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.margins: _smallMargins
        anchors.top: parent.top
        anchors.left: parent.left
        spacing: _smallMargins

        // Record
        Item {
            id: videoCaptureButton
            Layout.alignment: Qt.AlignHCenter
            width: root._buttonSize
            height: width
            scale: videoPressArea.pressed ? 0.88 : 1.0

            Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }

            Rectangle {
                anchors.fill: parent
                radius: width * 0.5
                color: videoCaptureButtonPalette.button
                border.width: 1
                border.color: videoCaptureButtonPalette.buttonBorder

                QGCPalette { id: videoCaptureButtonPalette; colorGroupEnabled: true }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false
                    width: parent.width * 0.75
                    height: width
                    radius: width * 0.5
                    color: videoCaptureButtonPalette.buttonBorder
                }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false
                    width: parent.width * (root._recording ? 0.42 : 0.75)
                    height: width
                    radius: root._recording ? ScreenTools.defaultFontPixelWidth * 0.45 : width * 0.5
                    color: videoCaptureButtonPalette.videoCaptureButtonColor
                    border.width: 1
                    border.color: videoCaptureButtonPalette.buttonBorder
                    opacity: root._recording ? (0.75 + 0.25 * recPulse.value) : 1.0

                    Behavior on width { NumberAnimation { duration: 120 } }
                    Behavior on height { NumberAnimation { duration: 120 } }
                    Behavior on radius { NumberAnimation { duration: 120 } }
                }
            }

            MouseArea {
                id: videoPressArea
                anchors.fill: parent
                onClicked: root._toggleRecording()
            }
        }

        QGCLabel {
            Layout.alignment: Qt.AlignHCenter
            text: root._recording ? qsTr("REC") : qsTr("Video")
            font.pointSize: ScreenTools.smallFontPointSize
            color: root._recording ? qgcPal.colorRed : qgcPal.text
            visible: root._recording || !root._showMediaLibrary
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: videoRecordTime.width + (root._smallMargins * 2)
            Layout.preferredHeight: videoRecordTime.height
            radius: root._smallMargins
            color: root._recording ? videoCaptureButtonPalette.videoCaptureButtonColor : "transparent"
            opacity: root._recording ? (0.85 + 0.15 * recPulse.value) : 1.0
            // Hide idle 00:00:00 on UniPod — short landscape needs vertical room
            visible: root._recording || !root._showMediaLibrary

            QGCLabel {
                id: videoRecordTime
                anchors.left: parent.left
                anchors.leftMargin: root._smallMargins
                anchors.verticalCenter: parent.verticalCenter
                text: root._formatRecordTime(root._recordElapsedMs)
                font.family: ScreenTools.fixedFontFamily
                font.pointSize: ScreenTools.smallFontPointSize
            }
        }

        Item {
            Layout.preferredHeight: root._smallMargins / 2
            Layout.fillWidth: true
            visible: !root._showMediaLibrary
        }

        // Snapshot
        Item {
            id: photoCaptureButton
            Layout.alignment: Qt.AlignHCenter
            width: root._buttonSize
            height: width
            scale: photoPressArea.pressed ? 0.88 : 1.0

            Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }

            Rectangle {
                anchors.fill: parent
                radius: width * 0.5
                color: photoCaptureButtonPalette.button
                border.width: 1
                border.color: photoCaptureButtonPalette.buttonBorder

                QGCPalette { id: photoCaptureButtonPalette; colorGroupEnabled: true }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false
                    width: parent.width * 0.75
                    height: width
                    radius: width * 0.5
                    color: photoCaptureButtonPalette.buttonBorder
                }

                Rectangle {
                    id: photoInner
                    anchors.centerIn: parent
                    anchors.alignWhenCentered: false
                    width: parent.width * (photoFlashAnim.running ? 0.52 : 0.75)
                    height: width
                    radius: width * 0.5
                    color: photoCaptureButtonPalette.photoCaptureButtonColor
                    border.width: 1
                    border.color: photoCaptureButtonPalette.buttonBorder

                    Behavior on width { NumberAnimation { duration: 90 } }
                    Behavior on height { NumberAnimation { duration: 90 } }
                }
            }

            MouseArea {
                id: photoPressArea
                anchors.fill: parent
                onClicked: root._takeSnapshot()
            }
        }

        QGCLabel {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Photo")
            font.pointSize: ScreenTools.smallFontPointSize
            visible: !root._showMediaLibrary
        }

        // Compact browse icon — same column width as capture buttons
        QGCColoredImage {
            Layout.alignment: Qt.AlignHCenter
            visible: root._showMediaLibrary
            enabled: root._mediaLibraryReady
            opacity: root._mediaLibraryReady ? 1.0 : 0.45
            source: "/res/SaveToDisk.svg"
            mipmap: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
            Layout.preferredWidth: Layout.preferredHeight
            sourceSize.height: Layout.preferredHeight
            color: qgcPal.text
            fillMode: Image.PreserveAspectFit

            QGCMouseArea {
                fillItem: parent
                enabled: root._mediaLibraryReady
                onClicked: mediaGalleryFactory.open({ mediaClient: root._unipodMediaClient })
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
