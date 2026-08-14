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

    property Fact mapRollFact: controller.getParameterFact(-1, "RCMAP_ROLL")
    property Fact mapPitchFact: controller.getParameterFact(-1, "RCMAP_PITCH")
    property Fact mapYawFact: controller.getParameterFact(-1, "RCMAP_YAW")
    property Fact mapThrottleFact: controller.getParameterFact(-1, "RCMAP_THROTTLE")

    function _ch(fact) {
        if (!fact || fact.value == 0) {
            return qsTr("—")
        }
        return fact.valueString
    }

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.4

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: ScreenTools.defaultFontPixelWidth * 0.6
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.35

            SummaryChip { Layout.fillWidth: true; text: qsTr("Roll  %1").arg(_ch(mapRollFact)) }
            SummaryChip { Layout.fillWidth: true; text: qsTr("Pitch  %1").arg(_ch(mapPitchFact)) }
            SummaryChip { Layout.fillWidth: true; text: qsTr("Throttle  %1").arg(_ch(mapThrottleFact)) }
            SummaryChip { Layout.fillWidth: true; text: qsTr("Yaw  %1").arg(_ch(mapYawFact)) }
        }

        QGCLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.7
            font.pointSize: ScreenTools.defaultFontPointSize * 0.85
            text: qsTr("Flight-controller RC input (primary control path).")
        }
    }
}
