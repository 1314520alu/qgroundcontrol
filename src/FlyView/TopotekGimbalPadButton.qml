import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Single flat pad cell for TopotekGimbalPad.
Rectangle {
    id: root

    property string label: ""
    property bool isHome: false

    signal pressed()
    signal released()
    signal clicked()

    width: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 3.2)
    height: width
    radius: ScreenTools.defaultFontPixelWidth * 0.45
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, enabled ? 0.55 : 0.35)
    border.width: 1
    border.color: pressArea.pressed ? qgcPal.buttonHighlight : Qt.rgba(qgcPal.buttonBorder.r, qgcPal.buttonBorder.g, qgcPal.buttonBorder.b, 0.45)

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCLabel {
        anchors.centerIn: parent
        text: root.label
        font.pointSize: root.isHome ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        enabled: root.enabled
        onPressed: root.pressed()
        onReleased: root.released()
        onClicked: if (root.isHome) { root.clicked() }
    }
}
