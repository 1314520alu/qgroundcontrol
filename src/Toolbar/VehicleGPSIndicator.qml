import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

GPSIndicator {
    objectName:     "toolbar_gpsIndicator"
    property bool showIndicator: _activeVehicle && (
                                     _activeVehicle.gps.telemetryAvailable
                                     || (_activeVehicle.gps2 && _activeVehicle.gps2.telemetryAvailable))

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
}
