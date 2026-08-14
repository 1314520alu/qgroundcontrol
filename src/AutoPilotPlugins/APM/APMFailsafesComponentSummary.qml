import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    FactPanelController { id: controller }

    property Fact _batt1Monitor: controller.getParameterFact(-1, "BATT_MONITOR")
    property bool _batt1MonitorEnabled: _batt1Monitor.rawValue !== 0
    property Fact _batt1FSLowAct: controller.getParameterFact(-1, "BATT_FS_LOW_ACT", false)
    property Fact _batt1FSCritAct: controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false)
    property bool _batt1FSCritActAvailable: controller.parameterExists(-1, "BATT_FS_CRT_ACT")
    property Fact _thrFS: controller.getParameterFact(-1, "FS_THR_ENABLE", false)

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.35

        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            SummaryChip {
                visible: controller.vehicle.multiRotor && _thrFS
                text: qsTr("Throttle: %1").arg(_thrFS ? _thrFS.enumStringValue : "")
            }
            SummaryChip {
                visible: _batt1MonitorEnabled && _batt1FSLowAct
                text: qsTr("Batt low: %1").arg(_batt1FSLowAct ? _batt1FSLowAct.enumStringValue : "")
            }
            SummaryChip {
                visible: _batt1FSCritActAvailable && _batt1FSCritAct
                text: qsTr("Batt crit: %1").arg(_batt1FSCritAct ? _batt1FSCritAct.enumStringValue : "")
            }
        }
    }
}
