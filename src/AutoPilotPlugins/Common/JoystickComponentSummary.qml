import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    readonly property var _activeJoystick: joystickManager.activeJoystick

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.25

        QGCLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            visible: !_activeJoystick
            color: QGroundControl.globalPalette.colorOrange
            font.pointSize: ScreenTools.defaultFontPointSize * 0.85
            text: qsTr("No HID joystick enumerated.")
        }

        QGCLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            visible: !_activeJoystick
            opacity: 0.75
            font.pointSize: ScreenTools.defaultFontPointSize * 0.78
            text: qsTr("UniRC sticks usually feed the aircraft over the RC/link path, not Android Joystick.")
        }

        SummaryChip {
            visible: !_activeJoystick
            text: qsTr("Use Radio mapping")
        }

        Flow {
            Layout.fillWidth: true
            visible: !!_activeJoystick
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            SummaryChip {
                text: _activeJoystick && _activeJoystick.isGamepad
                          ? (_activeJoystick.gamepadType || qsTr("Gamepad"))
                          : qsTr("Joystick")
            }
            SummaryChip {
                visible: _activeJoystick && _activeJoystick.axisCount > 0
                text: qsTr("%1 axes").arg(_activeJoystick ? _activeJoystick.axisCount : 0)
            }
            SummaryChip {
                visible: _activeJoystick && _activeJoystick.buttonCount > 0
                text: qsTr("%1 buttons").arg(_activeJoystick ? _activeJoystick.buttonCount : 0)
            }
            SummaryChip {
                visible: _activeJoystick && _activeJoystick.batteryPercent >= 0
                text: qsTr("Batt %1%").arg(_activeJoystick ? _activeJoystick.batteryPercent : 0)
            }
        }
    }
}
