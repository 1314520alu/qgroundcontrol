import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    APMSensorsComponentController { id: controller }
    APMSensorParams {
        id: sensorParams
        factPanelController: controller
    }

    function _compassInstalledCount() {
        let n = 0
        for (let i = 0; i < sensorParams.rgCompassAvailable.length; i++) {
            if (sensorParams.rgCompassAvailable[i]) {
                n++
            }
        }
        return n
    }

    readonly property int _compassInstalled: _compassInstalledCount()

    readonly property int _imuCount: sensorParams.rgInsId ? sensorParams.rgInsId.length : 0
    readonly property int _baroCount: sensorParams.rgBaroId ? sensorParams.rgBaroId.length : 0

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.4

        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            SummaryChip {
                text: qsTr("Compass ×%1").arg(_compassInstalled)
            }
            SummaryChip {
                text: controller.accelSetupNeeded ? qsTr("IMU setup") : qsTr("IMU ×%1").arg(_imuCount)
                border.color: controller.accelSetupNeeded
                                  ? QGroundControl.globalPalette.colorOrange
                                  : QGroundControl.globalPalette.buttonBorder
                textColor: controller.accelSetupNeeded
                               ? QGroundControl.globalPalette.colorOrange
                               : QGroundControl.globalPalette.text
            }
            SummaryChip {
                visible: sensorParams.baroIdAvailable
                text: qsTr("Baro ×%1").arg(_baroCount)
            }
        }

        QGCLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.7
            font.pointSize: ScreenTools.defaultFontPointSize * 0.85
            text: qsTr("Open Sensors for full device IDs.")
        }
    }
}
