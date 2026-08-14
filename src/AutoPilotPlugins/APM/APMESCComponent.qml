import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id: escPage
    pageComponent: escPageComponent
    showPageDescription: false   // title/hint live in left panel (match lock)

    FactPanelController {
        id: controller
    }

    Component {
        id: escPageComponent

        Item {
            id: pageRoot
            width: availableWidth
            height: availableHeight

            // ESC Configuration properties - supports both MOT_* (Copter/Rover/Sub) and Q_M_* (QuadPlane) prefixes
            property bool _isQuadPlane: !controller.parameterExists(-1, "MOT_PWM_TYPE") && controller.parameterExists(-1, "Q_M_PWM_TYPE")
            property string _escPrefix: _isQuadPlane ? "Q_M_" : "MOT_"

            property bool _motPwmTypeAvailable: controller.parameterExists(-1, _escPrefix + "PWM_TYPE")
            property bool _motPwmMinAvailable: controller.parameterExists(-1, _escPrefix + "PWM_MIN")
            property bool _motPwmMaxAvailable: controller.parameterExists(-1, _escPrefix + "PWM_MAX")
            property bool _motSpinArmAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_ARM")
            property bool _motSpinMinAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_MIN")
            property bool _motSpinMaxAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_MAX")
            property bool _servoDshotEscAvailable: controller.parameterExists(-1, "SERVO_DSHOT_ESC")
            property bool _servoDshotRateAvailable: controller.parameterExists(-1, "SERVO_DSHOT_RATE")

            property Fact _motPwmType: controller.getParameterFact(-1, _escPrefix + "PWM_TYPE", false /* reportMissing */)
            property Fact _motPwmMin: controller.getParameterFact(-1, _escPrefix + "PWM_MIN", false /* reportMissing */)
            property Fact _motPwmMax: controller.getParameterFact(-1, _escPrefix + "PWM_MAX", false /* reportMissing */)
            property Fact _motSpinArm: controller.getParameterFact(-1, _escPrefix + "SPIN_ARM", false /* reportMissing */)
            property Fact _motSpinMin: controller.getParameterFact(-1, _escPrefix + "SPIN_MIN", false /* reportMissing */)
            property Fact _motSpinMax: controller.getParameterFact(-1, _escPrefix + "SPIN_MAX", false /* reportMissing */)
            property Fact _servoDshotEsc: controller.getParameterFact(-1, "SERVO_DSHOT_ESC", false /* reportMissing */)
            property Fact _servoDshotRate: controller.getParameterFact(-1, "SERVO_DSHOT_RATE", false /* reportMissing */)

            property bool _isDshot: _motPwmTypeAvailable && _motPwmType && _motPwmType.rawValue >= 4

            property string _escCalParam: _isQuadPlane ? "Q_ESC_CAL" : "ESC_CALIBRATION"
            property bool _escCalibrationAvailable: controller.parameterExists(-1, _escCalParam)
            property Fact _escCalibration: controller.getParameterFact(-1, _escCalParam, false /* reportMissing */)

            property string _restartRequired: qsTr("Requires vehicle reboot")
            property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 15
            property real _comboWidth: ScreenTools.defaultFontPixelWidth * 30

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            readonly property real _pad: ScreenTools.defaultFontPixelHeight * 0.55
            readonly property real _gap: ScreenTools.defaultFontPixelWidth * 1.2
            readonly property real _panelRadius: ScreenTools.defaultBorderRadius
            readonly property bool _split: width >= ScreenTools.defaultFontPixelWidth * 55

            RowLayout {
                anchors.fill: parent
                spacing: pageRoot._gap
                visible: pageRoot._split

                Rectangle {
                    id: configPanel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 55
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    // placeholder ColumnLayout for Task 2
                }

                Rectangle {
                    id: calPanel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 45
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    // placeholder for Task 3
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: pageRoot._gap
                visible: !pageRoot._split

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    // placeholder ColumnLayout for Task 2
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    // placeholder for Task 3
                }
            }
        }
    }

}
