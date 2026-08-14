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

    property bool _isQuadPlane: !controller.parameterExists(-1, "MOT_PWM_TYPE") && controller.parameterExists(-1, "Q_M_PWM_TYPE")
    property string _escPrefix: _isQuadPlane ? "Q_M_" : "MOT_"
    property bool _motPwmTypeAvailable: controller.parameterExists(-1, _escPrefix + "PWM_TYPE")
    property Fact _motPwmType: controller.getParameterFact(-1, _escPrefix + "PWM_TYPE", false)
    property bool _isDshot: _motPwmTypeAvailable && _motPwmType && _motPwmType.rawValue >= 4
    property int _motorCount: controller.vehicle ? controller.vehicle.motorCount : -1

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.35

        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            SummaryChip {
                text: qsTr("Out: %1").arg(_motPwmTypeAvailable ? _motPwmType.enumStringValue : qsTr("Unknown"))
            }
            SummaryChip {
                text: qsTr("Motors: %1").arg(_motorCount > 0 ? _motorCount : "—")
            }
            SummaryChip {
                text: _isDshot ? qsTr("Protocol: DShot") : qsTr("Protocol: PWM")
            }
            SummaryChip {
                text: _isDshot ? qsTr("DShot: on") : qsTr("DShot: off")
                border.color: _isDshot ? QGroundControl.globalPalette.colorGreen : QGroundControl.globalPalette.buttonBorder
                textColor: _isDshot ? QGroundControl.globalPalette.colorGreen : QGroundControl.globalPalette.text
            }
        }
    }
}
