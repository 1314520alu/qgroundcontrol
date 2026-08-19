import QtQuick

import QGroundControl.Controls

Rectangle {
    id: root

    property string label: "+"

    signal pressed()
    signal released()

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
        font.pointSize: ScreenTools.defaultFontPointSize * 1.15
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        enabled: root.enabled
        onPressed: root.pressed()
        onReleased: root.released()
    }
}
