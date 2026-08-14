import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Status pill for Summary card headers — uses QGC status colors on button chrome.
Rectangle {
    id: root

    property string text: qsTr("Ready")
    property bool ok: true
    property bool warn: false

    readonly property color _accent: warn ? QGroundControl.globalPalette.colorOrange
                                          : (ok ? QGroundControl.globalPalette.colorGreen
                                                : QGroundControl.globalPalette.colorRed)

    implicitWidth: label.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.6
    implicitHeight: ScreenTools.defaultFontPixelHeight * 1.35
    radius: ScreenTools.defaultFontPixelHeight * 0.25
    color: QGroundControl.globalPalette.button
    border.width: 1
    border.color: _accent

    QGCLabel {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root._accent
        font.pointSize: ScreenTools.defaultFontPointSize * 0.8
        font.bold: true
    }
}
