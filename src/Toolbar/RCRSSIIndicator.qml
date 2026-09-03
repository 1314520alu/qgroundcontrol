import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

//-------------------------------------------------------------------------
//-- GCS link quality (Mission Planner HUD "telemetry connection link quality")
Item {
    id:             control
    objectName:     "toolbar_rcRSSIIndicator"
    width:          rssiRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: _activeVehicle

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property real _linkQuality:     _activeVehicle ? _activeVehicle.gcsLinkQuality : 0
    property bool _rcRSSIAvailable: _activeVehicle && _activeVehicle.rcRSSI.rawValue > 0 && _activeVehicle.rcRSSI.rawValue <= 100
    // Match GPS satellite icon: square, full toolbar height.
    property real _iconSize:        height

    Component {
        id: rcRSSIInfoPage

        ToolIndicatorPage {
            showExpand: false

            contentComponent: SettingsGroupLayout {
                heading: qsTr("Link Quality")

                LabelledLabel {
                    label:      qsTr("GCS link quality")
                    labelText:  Math.round(control._linkQuality) + "%"
                }

                LabelledLabel {
                    label:      qsTr("Packets received")
                    labelText:  control._activeVehicle ? control._activeVehicle.mavlinkReceivedCount : qsTr("Not Connected")
                }

                LabelledLabel {
                    label:      qsTr("Packets lost")
                    labelText:  control._activeVehicle ? control._activeVehicle.mavlinkLossCount : qsTr("Not Connected")
                }

                LabelledLabel {
                    visible:    control._rcRSSIAvailable
                    label:      qsTr("RC RSSI")
                    labelText:  control._activeVehicle.rcRSSI.rawValue + "%"
                }
            }
        }
    }

    Row {
        id:                     rssiRow
        anchors.verticalCenter: parent.verticalCenter
        spacing:                ScreenTools.defaultFontPixelWidth / 4

        SignalStrength {
            anchors.verticalCenter: parent.verticalCenter
            size:                   control._iconSize
            percent:                control._linkQuality
        }

        QGCLabel {
            anchors.verticalCenter: parent.verticalCenter
            text:                   Math.round(control._linkQuality) + "%"
            font.pointSize:         ScreenTools.smallFontPointSize
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(rcRSSIInfoPage, control)
    }
}
