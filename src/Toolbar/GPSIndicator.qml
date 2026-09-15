import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Used as the base class control for both VehicleGPSIndicator and RTKGPSIndicator

Item {
    id:             control
    width:          gpsIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _rtkConnected:  QGroundControl.gpsRtk.connected.value
    property var    _gps:           _activeVehicle ? _activeVehicle.gps : null
    property var    _gps2:          _activeVehicle ? _activeVehicle.gps2 : null
    readonly property bool _gps2Available: _gps2 && _gps2.telemetryAvailable

    QGCPalette { id: qgcPal }

    component GpsCluster: Item {
        id: cluster

        property var    gpsFactGroup
        property bool   showRtkLabel: false

        // Lock type drives color (same palette as the battery icon). 3D with
        // few sats or poor HDOP is downgraded so a "3D" lock still looks weak.
        readonly property color statusColor: {
            var gps = cluster.gpsFactGroup
            if (!gps || !gps.telemetryAvailable) {
                return qgcPal.text
            }
            var lock = gps.lock.rawValue
            var sats = gps.count.value
            var hdop = gps.hdop.value
            var weak3d = sats < 6 || (!isNaN(hdop) && hdop > 2.0)
            switch (lock) {
            case 0:     // None
                return qgcPal.text
            case 1:     // No Fix
                return qgcPal.colorRed
            case 2:     // 2D Lock
                return qgcPal.colorOrange
            case 3:     // 3D Lock
                return weak3d ? qgcPal.colorYellow : qgcPal.colorGreen
            case 4:     // DGPS
                return qgcPal.colorGreen
            case 5:     // RTK float
                return qgcPal.colorYellowGreen
            case 6:     // RTK fixed
            case 7:     // Static
                return qgcPal.colorGreen
            default:
                return qgcPal.text
            }
        }

        width:          clusterRow.width
        anchors.top:    parent.top
        anchors.bottom: parent.bottom

        Row {
            id:             clusterRow
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        ScreenTools.defaultFontPixelWidth / 2

            Row {
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                spacing:        -ScreenTools.defaultFontPixelWidth / 2

                QGCLabel {
                    rotation:               90
                    text:                   qsTr("RTK")
                    color:                  cluster.statusColor
                    anchors.verticalCenter: parent.verticalCenter
                    visible:                cluster.showRtkLabel
                }

                QGCColoredImage {
                    width:              height
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    source:             "/qmlimages/Gps.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    opacity:            (cluster.gpsFactGroup && cluster.gpsFactGroup.telemetryAvailable) ? 1 : 0.5
                    color:              cluster.statusColor
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                visible:                cluster.gpsFactGroup
                spacing:                0

                QGCLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color:                    cluster.statusColor
                    text:                     cluster.gpsFactGroup ? cluster.gpsFactGroup.count.valueString : ""
                }

                QGCLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color:                    cluster.statusColor
                    visible:                  cluster.gpsFactGroup && !isNaN(cluster.gpsFactGroup.hdop.value)
                    text:                     cluster.gpsFactGroup ? cluster.gpsFactGroup.hdop.value.toFixed(1) : ""
                }
            }
        }
    }

    Row {
        id:             gpsIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        GpsCluster {
            gpsFactGroup:   control._gps
            showRtkLabel:   control._rtkConnected
        }

        GpsCluster {
            visible:        control._gps2Available
            gpsFactGroup:   control._gps2
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(gpsIndicatorPage, control)
    }

    Component {
        id: gpsIndicatorPage

        GPSIndicatorPage { }
    }
}
