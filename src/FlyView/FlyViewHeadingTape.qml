import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id:     control
    height: ScreenTools.defaultFontPixelHeight * 2.2
    width:  ScreenTools.defaultFontPixelWidth * 36

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real _heading:      _activeVehicle ? _activeVehicle.heading.rawValue : 0

    QGCPalette { id: qgcPal }

    function _normalize(degrees) {
        var a = degrees % 360
        if (a < 0) {
            a += 360
        }
        return a
    }

    Rectangle {
        id:             tapeBackground
        anchors.fill:   parent
        color:          qgcPal.window
        radius:         ScreenTools.defaultBorderRadius
        border.color:   qgcPal.buttonHighlight
        border.width:   1
        clip:           true
        opacity:        0.92

        Item {
            id:             tapeStrip
            anchors.fill:   parent

            Repeater {
                model: 720

                QGCLabel {
                    property int _startAngle: modelData + 180 + control._heading
                    property int _angle:      control._normalize(_startAngle)

                    anchors.verticalCenter: parent.verticalCenter
                    x:              (_angle % 45 === 0) ? ((modelData * (tapeBackground.width / 360)) - (width * 0.5)) : 0
                    visible:        _angle % 45 === 0
                    color:          qgcPal.text
                    font.pointSize: ScreenTools.smallFontPointSize
                    text: {
                        switch (_angle) {
                        case 0:     return qsTr("N")
                        case 45:    return qsTr("NE")
                        case 90:    return qsTr("E")
                        case 135:   return qsTr("SE")
                        case 180:   return qsTr("S")
                        case 225:   return qsTr("SW")
                        case 270:   return qsTr("W")
                        case 315:   return qsTr("NW")
                        }
                        return ""
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter:   parent.verticalCenter
        width:          headingLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
        height:         parent.height - 4
        radius:         ScreenTools.defaultBorderRadius
        color:          qgcPal.buttonHighlight

        QGCLabel {
            id:                     headingLabel
            anchors.centerIn:       parent
            color:                  qgcPal.buttonHighlightText
            font.bold:              true
            text:                   ("000" + Math.round(control._normalize(control._heading))).slice(-3)
        }
    }
}
