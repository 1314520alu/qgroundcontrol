import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property url iconSource: ""
    property string label: ""
    property bool selected: false

    signal clicked()

    width: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 3.4)
    height: width + labelItem.height + ScreenTools.defaultFontPixelHeight * 0.15

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Rectangle {
        id: circle
        width: root.width
        height: width
        radius: width * 0.5
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
        border.width: 1
        border.color: (pressArea.pressed || root.selected)
                      ? qgcPal.buttonHighlight
                      : Qt.rgba(qgcPal.buttonBorder.r, qgcPal.buttonBorder.g, qgcPal.buttonBorder.b, 0.45)

        QGCColoredImage {
            anchors.centerIn: parent
            width: parent.width * 0.5
            height: width
            source: root.iconSource
            sourceSize.height: height
            color: qgcPal.text
            fillMode: Image.PreserveAspectFit
            visible: root.iconSource !== ""
        }
    }

    QGCLabel {
        id: labelItem
        anchors.horizontalCenter: circle.horizontalCenter
        anchors.top: circle.bottom
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.08
        text: root.label
        font.pointSize: ScreenTools.smallFontPointSize
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
