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
        property string indexLabel:   ""

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
                    text:                   cluster.showRtkLabel ? qsTr("RTK") : cluster.indexLabel
                    color:                  qgcPal.text
                    anchors.verticalCenter: parent.verticalCenter
                    visible:                cluster.showRtkLabel || cluster.indexLabel.length > 0
                }

                QGCColoredImage {
                    width:              height
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    source:             "/qmlimages/Gps.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    opacity:            (cluster.gpsFactGroup && cluster.gpsFactGroup.count.value >= 0) ? 1 : 0.5
                    color:              qgcPal.text
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                visible:                cluster.gpsFactGroup && !isNaN(cluster.gpsFactGroup.hdop.value)
                spacing:                0

                QGCLabel {
                    anchors.horizontalCenter: hdopValue.horizontalCenter
                    color:                    qgcPal.text
                    text:                     cluster.gpsFactGroup ? cluster.gpsFactGroup.count.valueString : ""
                }

                QGCLabel {
                    id:     hdopValue
                    color:  qgcPal.text
                    text:   cluster.gpsFactGroup ? cluster.gpsFactGroup.hdop.value.toFixed(1) : ""
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
            indexLabel:     "2"
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
