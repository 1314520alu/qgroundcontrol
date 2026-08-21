import QtQuick
import QtQuick.Layouts

import QGroundControl.Controls

/// Hold +/- zoom or focus buttons for Topotek TQ10N (ZMC in/out, release stop).
ColumnLayout {
    id: root

    property var camera
    property bool useFocus: false
    property real buttonSize: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 3.2)

    spacing: ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    TopotekZoomHoldButton {
        Layout.alignment: Qt.AlignHCenter
        label: "+"
        enabled: root.camera
        onPressed: {
            if (!root.camera) { return }
            if (root.useFocus) { root.camera.startFocus(1) } else { root.camera.startZoom(1) }
        }
        onReleased: {
            if (!root.camera) { return }
            if (root.useFocus) { root.camera.stopFocus() } else { root.camera.stopZoom() }
        }
    }

    QGCLabel {
        Layout.alignment: Qt.AlignHCenter
        text: root.useFocus ? qsTr("Focus") : qsTr("Zoom")
        font.pointSize: ScreenTools.smallFontPointSize
    }

    TopotekZoomHoldButton {
        Layout.alignment: Qt.AlignHCenter
        label: "\u2212"
        enabled: root.camera
        onPressed: {
            if (!root.camera) { return }
            if (root.useFocus) { root.camera.startFocus(-1) } else { root.camera.startZoom(-1) }
        }
        onReleased: {
            if (!root.camera) { return }
            if (root.useFocus) { root.camera.stopFocus() } else { root.camera.stopZoom() }
        }
    }
}
