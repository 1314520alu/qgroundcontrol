import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Item {
    id: control

    implicitWidth:  height
    implicitHeight: ScreenTools.toolbarHeight
    width:          implicitWidth
    height:         implicitHeight

    signal clicked()

    QGCPalette { id: qgcPal }

    Rectangle {
        id:             avatarFrame
        anchors.centerIn: parent
        width:          Math.min(parent.width, parent.height) - ScreenTools.defaultFontPixelWidth * 0.5
        height:         width
        radius:         width / 2
        color:          qgcPal.windowShade
        border.color:   qgcPal.buttonHighlight
        border.width:   1
        clip:           true

        Image {
            anchors.fill:       parent
            source:             "/res/GoldenRetrieverAvatar.png"
            fillMode:           Image.PreserveAspectCrop
            mipmap:             true
            asynchronous:       true
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      control.clicked()
    }
}
