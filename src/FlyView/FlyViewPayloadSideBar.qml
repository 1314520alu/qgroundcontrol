import QtQuick

import QGroundControl
import QGroundControl.Controls

// UniGCS-style frosted vertical strip. Uses Column (not ColumnLayout) so
// icon+label height is never compressed into overlapping rows.
Item {
    id: root

    default property alias content: contentCol.data

    property real barPadding: ScreenTools.defaultFontPixelWidth * 0.4
    property real barSpacing: ScreenTools.defaultFontPixelHeight * 0.35
    property real circleSize: 0

    implicitWidth: Math.max(contentCol.implicitWidth, circleSize) + barPadding * 2
    implicitHeight: contentCol.implicitHeight + barPadding * 2

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultFontPixelWidth * 1.1
        color: Qt.rgba(0, 0, 0, 0.55)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.18)
    }

    Column {
        id: contentCol
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: root.barPadding
        spacing: root.barSpacing
        width: Math.max(implicitWidth, root.circleSize)
    }
}
