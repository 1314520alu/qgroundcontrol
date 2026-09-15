import QtQuick
import QtMultimedia

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

/// Hold zoom/focus: press starts continuous in/out, release (or cancel) stops.
Column {
    id: root

    property var camera
    property bool useFocus: false
    property real circleSize: 0

    signal zoomInteracted()

    property var _appSettings: QGroundControl.settingsManager.appSettings
    property bool _audioMuted: _appSettings && _appSettings.audioMuted.rawValue
    property real _audioVolume: _appSettings ? _appSettings.audioVolume.rawValue : 50

    spacing: ScreenTools.defaultFontPixelHeight * 0.3

    function _playSound(effect) {
        if (_audioMuted || _audioVolume <= 0 || !effect) {
            return
        }
        effect.volume = Math.max(0.05, Math.min(1.0, _audioVolume / 100.0))
        effect.play()
    }

    function _start(direction) {
        if (!root.camera) {
            return
        }
        _playSound(direction > 0 ? zoomInSound : zoomOutSound)
        if (root.useFocus) {
            root.camera.startFocus(direction)
        } else {
            root.camera.startZoom(direction)
            root.zoomInteracted()
        }
    }

    function _stop() {
        if (!root.camera) {
            return
        }
        if (root.useFocus) {
            root.camera.stopFocus()
        } else {
            root.camera.stopZoom()
        }
    }

    SoundEffect {
        id: zoomInSound
        source: "qrc:/res/audio/zoom_in.wav"
    }

    SoundEffect {
        id: zoomOutSound
        source: "qrc:/res/audio/zoom_out.wav"
    }

    FlyViewPayloadIconButton {
        circleSize: root.circleSize
        enabled: root.camera
        iconSource: "/InstrumentValueIcons/zoom-in.svg"
        label: root.useFocus ? qsTr("Near") : qsTr("Zoom In")
        onPressed: root._start(1)
        onReleased: root._stop()
    }

    FlyViewPayloadIconButton {
        circleSize: root.circleSize
        enabled: root.camera
        iconSource: "/InstrumentValueIcons/zoom-out.svg"
        label: root.useFocus ? qsTr("Far") : qsTr("Zoom Out")
        onPressed: root._start(-1)
        onReleased: root._stop()
    }
}
