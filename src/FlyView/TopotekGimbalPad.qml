import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// UniGCS-style flat D-pad for Topotek TQ10N PTZ speed control (hold direction, release stop).
Item {
    id: root

    property var camera
    property real cellSize: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 3.2)
    property real spacing: ScreenTools.defaultFontPixelWidth / 2

    implicitWidth: gridLayout.implicitWidth
    implicitHeight: gridLayout.implicitHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    GridLayout {
        id: gridLayout
        anchors.fill: parent
        columns: 3
        rows: 3
        columnSpacing: root.spacing
        rowSpacing: root.spacing

        Item { Layout.row: 0; Layout.column: 0; width: root.cellSize; height: root.cellSize }

        TopotekGimbalPadButton {
            Layout.row: 0
            Layout.column: 1
            label: "\u2191"
            enabled: root.camera
            onPressed: if (root.camera) { root.camera.ptzStart(1) }
            onReleased: if (root.camera) { root.camera.ptzStop() }
        }

        Item { Layout.row: 0; Layout.column: 2; width: root.cellSize; height: root.cellSize }

        TopotekGimbalPadButton {
            Layout.row: 1
            Layout.column: 0
            label: "\u2190"
            enabled: root.camera
            onPressed: if (root.camera) { root.camera.ptzStart(3) }
            onReleased: if (root.camera) { root.camera.ptzStop() }
        }

        TopotekGimbalPadButton {
            Layout.row: 1
            Layout.column: 1
            label: qsTr("Home")
            isHome: true
            enabled: root.camera
            onClicked: if (root.camera) { root.camera.ptzHome() }
        }

        TopotekGimbalPadButton {
            Layout.row: 1
            Layout.column: 2
            label: "\u2192"
            enabled: root.camera
            onPressed: if (root.camera) { root.camera.ptzStart(4) }
            onReleased: if (root.camera) { root.camera.ptzStop() }
        }

        Item { Layout.row: 2; Layout.column: 0; width: root.cellSize; height: root.cellSize }

        TopotekGimbalPadButton {
            Layout.row: 2
            Layout.column: 1
            label: "\u2193"
            enabled: root.camera
            onPressed: if (root.camera) { root.camera.ptzStart(2) }
            onReleased: if (root.camera) { root.camera.ptzStop() }
        }

        Item { Layout.row: 2; Layout.column: 2; width: root.cellSize; height: root.cellSize }
    }
}
