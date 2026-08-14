import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _escs: _vehicle ? _vehicle.escs : null
    property int _count: _escs ? Math.min(_escs.count, 16) : 0

    function _bitOnline(esc) {
        if (!esc) {
            return false
        }
        const id = Number(esc.id.rawValue)
        return (Number(esc.info.rawValue) & (1 << (id % 4))) !== 0
    }

    function _healthyCount() {
        if (!_escs) {
            return 0
        }
        let n = 0
        for (let i = 0; i < _count; i++) {
            const esc = _escs.get(i)
            if (_bitOnline(esc) && Number(esc.failureFlags.rawValue) === 0 && Number(esc.errorCount.rawValue) === 0) {
                n++
            }
        }
        return n
    }

    function _protocolLabel() {
        if (!_escs || _escs.count === 0) {
            return qsTr("—")
        }
        const ct = Number(_escs.get(0).connectionType.rawValue)
        if (ct === 0) {
            return qsTr("电调遥测")
        }
        return _escs.get(0).connectionType.enumStringValue
    }

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.35

        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            SummaryChip {
                text: _count > 0
                      ? qsTr("电调遥测: %1/%2 正常").arg(_healthyCount()).arg(_count)
                      : qsTr("电调遥测: 无")
                textColor: _count > 0 && _healthyCount() === _count
                           ? QGroundControl.globalPalette.colorGreen
                           : (_count > 0 ? QGroundControl.globalPalette.colorRed
                                         : QGroundControl.globalPalette.text)
                border.color: textColor === QGroundControl.globalPalette.text
                              ? QGroundControl.globalPalette.buttonBorder
                              : textColor
            }

            SummaryChip {
                visible: _count > 0
                text: qsTr("协议: %1").arg(_protocolLabel())
            }
        }
    }
}
