import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id:             control
    objectName:     "toolbar_escIndicator"
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          escIndicatorRow.width

    property bool showIndicator: _escs && _escs.count > 0

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property var  _escs:            _activeVehicle ? _activeVehicle.escs : null

    property int    _motorCount:        _escs ? _escs.count : 0
    property int    _onlineMotorCount:  _getOnlineMotorCount()
    property bool   _escHealthy:        _getEscHealthStatus()

    function _bitOnline(esc) {
        if (!esc) {
            return false
        }
        const id = Number(esc.id.rawValue)
        const bit = id % 4
        return (Number(esc.info.rawValue) & (1 << bit)) !== 0
    }

    function _getOnlineMotorCount() {
        if (!_escs || _motorCount === 0) {
            return 0
        }
        let count = 0
        for (let i = 0; i < _motorCount; i++) {
            if (_bitOnline(_escs.get(i))) {
                count++
            }
        }
        return count
    }

    function _getEscHealthStatus() {
        if (!_escs || _motorCount === 0) {
            return false
        }
        if (_onlineMotorCount !== _motorCount) {
            return false
        }
        for (let index = 0; index < _motorCount; index++) {
            const esc = _escs.get(index)
            if (!_bitOnline(esc)) {
                return false
            }
            if (Number(esc.failureFlags.rawValue) !== 0) {
                return false
            }
            if (Number(esc.errorCount.rawValue) !== 0) {
                return false
            }
        }
        return true
    }

    function getEscStatusColor() {
        return _escHealthy ? qgcPal.colorGreen : qgcPal.colorRed
    }

    QGCPalette { id: qgcPal }

    Row {
        id:             escIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage {
            id:                 escIcon
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             "/qmlimages/EscIndicator.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            color:              qgcPal.text
        }

        Column {
            id:                     escValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                color:                      qgcPal.text
                text:                       _onlineMotorCount.toString()
                font.pointSize:             ScreenTools.smallFontPointSize
            }

            QGCLabel {
                color:          getEscStatusColor()
                text:           _escHealthy ? qsTr("正常") : qsTr("异常")
                font.pointSize: ScreenTools.smallFontPointSize
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(escIndicatorPage, control)
    }

    Component {
        id: escIndicatorPage

        EscIndicatorPage { }
    }
}
