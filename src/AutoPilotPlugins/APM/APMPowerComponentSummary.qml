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
    APMBatteryParams {
        id: battParams
        controller: controller
        batteryIndex: 0
    }

    readonly property int _maxVisibleBatteries: 2
    readonly property int _enabledCount: battParams.getEnabledBatteryCount()
    readonly property int _slotCount: battParams.getBatteryCount()

    function _enabledIndices() {
        const indices = []
        for (let i = 0; i < _slotCount; i++) {
            const mon = controller.getParameterFact(-1, battParams.prefixForIndex(i) + "MONITOR", false)
            if (mon && mon.rawValue !== 0) {
                indices.push(i)
            }
        }
        return indices
    }

    property var _visibleIndices: {
        const all = _enabledCount >= 0 ? _enabledIndices() : []
        return all.slice(0, _maxVisibleBatteries)
    }
    property int _hiddenEnabled: Math.max(0, _enabledCount - _maxVisibleBatteries)

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.35

        SummaryChip {
            text: qsTr("%1 of %2 packs enabled").arg(_enabledCount).arg(_slotCount)
        }

        Repeater {
            model: _visibleIndices
            delegate: ColumnLayout {
                required property int modelData
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelHeight * 0.12

                property int battIndex: modelData
                property string _prefix: battParams.prefixForIndex(battIndex)
                property string _label: battParams.labelForIndex(battIndex)
                property Fact _monitor: controller.getParameterFact(-1, _prefix + "MONITOR")
                property bool _capacityAvailable: controller.parameterExists(-1, _prefix + "CAPACITY")
                property Fact _capacity: _capacityAvailable ? controller.getParameterFact(-1, _prefix + "CAPACITY") : null

                Flow {
                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelWidth * 0.45

                    SummaryChip {
                        text: qsTr("Batt%1 · %2").arg(_label).arg(_monitor.enumStringValue)
                    }
                    SummaryChip {
                        visible: _capacityAvailable
                        text: _capacity ? (_capacity.valueString + " " + _capacity.units) : ""
                    }
                }
            }
        }

        SummaryChip {
            visible: _hiddenEnabled > 0
            text: qsTr("+%1 more — open Power").arg(_hiddenEnabled)
        }

        QGCLabel {
            visible: _enabledCount === 0
            text: qsTr("No battery monitor enabled")
            color: QGroundControl.globalPalette.warningText
        }
    }
}
