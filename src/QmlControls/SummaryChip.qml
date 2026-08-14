import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Compact chip used by Vehicle Summary cards — colors follow QGC palette.
Rectangle {
    id: root

    property string text: ""
    property color textColor: QGroundControl.globalPalette.text
    // Prefer panel-shade language over translucent black overlays
    property color fillColor: QGroundControl.globalPalette.button
    property real horizontalPadding: ScreenTools.defaultFontPixelWidth * 0.65
    property real verticalPadding: ScreenTools.defaultFontPixelHeight * 0.15

    implicitWidth: label.implicitWidth + horizontalPadding * 2
    implicitHeight: label.implicitHeight + verticalPadding * 2
    radius: ScreenTools.defaultFontPixelHeight * 0.25
    color: fillColor
    border.width: 1
    border.color: QGroundControl.globalPalette.buttonBorder

    QGCLabel {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.textColor
        font.pointSize: ScreenTools.defaultFontPointSize * 0.78
    }
}
