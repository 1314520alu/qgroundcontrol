import QtQuick

import QGroundControl
import QGroundControl.Controls

Column {
    id:      control
    spacing: ScreenTools.defaultFontPixelWidth * 0.5

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCPalette { id: qgcPal }

    function _factText(fact) {
        if (!fact) {
            return qsTr("--")
        }
        var valueText = fact.valueString ? fact.valueString : fact.value
        if (valueText === undefined || valueText === "") {
            return qsTr("--")
        }
        return valueText + (fact.units ? " " + fact.units : "")
    }

    function _batteryText() {
        if (!_activeVehicle || _activeVehicle.batteries.count === 0) {
            return qsTr("--")
        }
        var battery = _activeVehicle.batteries.get(0)
        if (!battery || !battery.percentRemaining) {
            return qsTr("--")
        }
        var raw = battery.percentRemaining.rawValue
        if (isNaN(raw)) {
            return qsTr("--")
        }
        return Math.round(raw) + qsTr("%")
    }

    function _satText() {
        if (!_activeVehicle || !_activeVehicle.gps || !_activeVehicle.gps.count) {
            return qsTr("--")
        }
        return _activeVehicle.gps.count.valueString
    }

    HudTelemetryCard {
        label: qsTr("相对高度")
        value: _activeVehicle ? control._factText(_activeVehicle.altitudeRelative) : qsTr("--")
    }
    HudTelemetryCard {
        label: qsTr("海拔")
        value: _activeVehicle ? control._factText(_activeVehicle.altitudeAMSL) : qsTr("--")
    }
    HudTelemetryCard {
        label: qsTr("地速")
        value: _activeVehicle ? control._factText(_activeVehicle.groundSpeed) : qsTr("--")
    }
    HudTelemetryCard {
        label: qsTr("距离")
        value: _activeVehicle ? control._factText(_activeVehicle.distanceToHome) : qsTr("--")
    }
    HudTelemetryCard {
        label: qsTr("电量")
        value: control._batteryText()
    }
    HudTelemetryCard {
        label: qsTr("卫星")
        value: control._satText()
    }

    component HudTelemetryCard: Rectangle {
        property string label
        property string value

        width:  ScreenTools.defaultFontPixelWidth * 14
        height: ScreenTools.defaultFontPixelHeight * 3.2
        radius: ScreenTools.defaultBorderRadius
        color:  qgcPal.window
        border.color: qgcPal.groupBorder
        border.width: 1
        opacity: 0.92

        Column {
            anchors.centerIn: parent
            spacing:          0

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text:               label
                font.pointSize:     ScreenTools.smallFontPointSize
                color:              qgcPal.text
                opacity:            0.7
            }
            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text:               value
                font.bold:          true
            }
        }
    }
}
