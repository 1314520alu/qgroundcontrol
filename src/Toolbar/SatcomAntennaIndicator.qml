import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

//-------------------------------------------------------------------------
//-- Weitong satcom antenna telemetry (CC frame) in the Fly View toolbar.
Item {
    id:             control
    objectName:     "toolbar_satcomAntennaIndicator"
    width:          indicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var  controller:    QGroundControl.satcomAntennaController
    property bool showIndicator: controller && controller.enabled

    property bool _connected:       controller && controller.connected
    property bool _telemetryValid:  controller && controller.telemetryValid
    property int  _antennaState:    controller ? controller.antennaState : 0
    property int  _networkState:    controller ? controller.networkState : 0

    readonly property bool _tracking: _antennaState === 2
    readonly property bool _searching: _antennaState === 1
    readonly property bool _online: _networkState === 1

    QGCPalette { id: qgcPal }

    function _statusColor() {
        if (!_connected) {
            return qgcPal.warningText
        }
        if (!_telemetryValid) {
            return qgcPal.colorYellow
        }
        if (_tracking && _online) {
            return qgcPal.colorGreen
        }
        if (_searching || !_online) {
            return qgcPal.colorYellow
        }
        return qgcPal.text
    }

    Row {
        id:                     indicatorRow
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        spacing:                ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage {
            id:                     satcomIcon
            width:                  height
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            source:                 "/qmlimages/TrackingIcon.svg"
            fillMode:               Image.PreserveAspectFit
            sourceSize.height:      height
            color:                  control._statusColor()
            opacity:                control._connected ? 1 : 0.55
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0
            visible:                control._connected

            Row {
                spacing: ScreenTools.defaultFontPixelWidth * 0.6

                QGCLabel {
                    text:           control._telemetryValid ? control.controller.antennaStateText : qsTr("--")
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          control._statusColor()
                }

                QGCLabel {
                    text:           control._telemetryValid ? control.controller.networkStateText : qsTr("--")
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          control._telemetryValid && control._online ? qgcPal.colorGreen : qgcPal.warningText
                }
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth * 0.6

                QGCLabel {
                    text:           control._telemetryValid ? control.controller.signalStrength.toFixed(1) : qsTr("--")
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          qgcPal.text
                }

                QGCLabel {
                    text: control._telemetryValid
                          ? qsTr("%1°/%2°")
                                .arg(control.controller.azimuthDeg.toFixed(1))
                                .arg(control.controller.elevationDeg.toFixed(1))
                          : qsTr("--")
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          qgcPal.text
                }
            }
        }

        QGCLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible:                !control._connected
            text:                   control.controller ? control.controller.connectionStatusText : ""
            font.pointSize:         ScreenTools.smallFontPointSize
            color:                  control._statusColor()
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(satcomInfoPage, control)
    }

    Component {
        id: satcomInfoPage

        ToolIndicatorPage {
            showExpand: false

            contentComponent: SettingsGroupLayout {
                heading: qsTr("Satcom Antenna")

                LabelledLabel {
                    label:      qsTr("Connection")
                    labelText:  control.controller ? control.controller.connectionStatusText : qsTr("--")
                }

                LabelledLabel {
                    label:      qsTr("State")
                    labelText:  control._telemetryValid ? control.controller.antennaStateText : qsTr("--")
                }

                LabelledLabel {
                    label:      qsTr("Network")
                    labelText:  control._telemetryValid ? control.controller.networkStateText : qsTr("--")
                }

                LabelledLabel {
                    label:      qsTr("Signal")
                    labelText:  control._telemetryValid ? control.controller.signalStrength.toFixed(1) : qsTr("--")
                }

                LabelledLabel {
                    label:      qsTr("Azimuth")
                    labelText:  control._telemetryValid ? qsTr("%1°").arg(control.controller.azimuthDeg.toFixed(1)) : qsTr("--")
                }

                LabelledLabel {
                    label:      qsTr("Elevation")
                    labelText:  control._telemetryValid ? qsTr("%1°").arg(control.controller.elevationDeg.toFixed(1)) : qsTr("--")
                }
            }
        }
    }
}
