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

    property Fact _copterFenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE", false)
    property Fact _copterFenceType: controller.getParameterFact(-1, "FENCE_TYPE", false)
    property Fact _armingCheckFact: controller.getParameterFact(-1, "ARMING_CHECK", false)
    property Fact _armingSkipCheckFact: controller.getParameterFact(-1, "ARMING_SKIPCHK", false)
    property Fact _rtlAlt: controller.getParameterFact(-1, "RTL_ALT_M", false)

    readonly property string _armingText: {
        if (_armingCheckFact) {
            return (_armingCheckFact.value & 1) ? qsTr("Enabled") : qsTr("Some disabled")
        }
        if (_armingSkipCheckFact) {
            return _armingSkipCheckFact.value === 0 ? qsTr("Enabled") : qsTr("Some disabled")
        }
        return qsTr("—")
    }

    readonly property bool _armingOk: {
        if (_armingCheckFact) {
            return (_armingCheckFact.value & 1) !== 0
        }
        if (_armingSkipCheckFact) {
            return _armingSkipCheckFact.value === 0
        }
        return true
    }

    readonly property string _fenceText: {
        if (!controller.vehicle.multiRotor || !_copterFenceEnable || !_copterFenceType) {
            return qsTr("—")
        }
        if (_copterFenceEnable.value == 0 || _copterFenceType.value == 0) {
            return qsTr("Disabled")
        }
        return qsTr("Enabled")
    }

    readonly property bool _fenceOn: _fenceText === qsTr("Enabled")

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.4

        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            SummaryChip {
                text: qsTr("Arming: %1").arg(_armingText)
                border.color: _armingOk ? QGroundControl.globalPalette.colorGreen : QGroundControl.globalPalette.colorOrange
                textColor: _armingOk ? QGroundControl.globalPalette.colorGreen : QGroundControl.globalPalette.colorOrange
            }
            SummaryChip {
                visible: controller.vehicle.multiRotor
                text: qsTr("Fence: %1").arg(_fenceText)
                border.color: _fenceOn ? QGroundControl.globalPalette.colorGreen : QGroundControl.globalPalette.colorRed
                textColor: _fenceOn ? QGroundControl.globalPalette.colorGreen : QGroundControl.globalPalette.colorRed
            }
            SummaryChip {
                visible: controller.vehicle.multiRotor && _rtlAlt
                text: qsTr("RTL %1").arg(_rtlAlt.value == 0 ? qsTr("current") : (_rtlAlt.valueString + " " + _rtlAlt.units))
            }
        }
    }
}
