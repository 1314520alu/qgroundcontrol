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
    property var  _appSettings:     QGroundControl.settingsManager.appSettings

    property int    _motorCount:        _escs ? _escs.count : 0
    property int    _onlineMotorCount:  _getOnlineMotorCount()
    property bool   _escHealthy:        _getEscHealthStatus()
    property int    _h15MaxTempLevel:   _getH15MaxTempLevel()

    // ZY-XF200系留 + H15MD Plus official ESC bands (Hobbywing).
    readonly property bool _h15EscTempLimits: _appSettings
                                              && _appSettings.aircraftModel.rawValue === Xf200TetheredPowerVisual.aircraftModelZyXf200Tethered
    readonly property real _h15WarnC:  90
    readonly property real _h15AlarmC: 105
    readonly property int  _voiceIntervalMs: 20000

    property real _lastWarnSpeakMs:  0
    property real _lastAlarmSpeakMs: 0

    function _bitOnline(esc) {
        if (!esc) {
            return false
        }
        const id = Number(esc.id.rawValue)
        const bit = id % 4
        return (Number(esc.info.rawValue) & (1 << bit)) !== 0
    }

    function _tempDegC(esc) {
        if (!esc) {
            return NaN
        }
        const raw = Number(esc.temperature.rawValue)
        if (raw === 32767) {
            return NaN
        }
        return raw / 100
    }

    function _h15TempLevel(esc) {
        if (!_h15EscTempLimits || !esc) {
            return 0
        }
        const t = _tempDegC(esc)
        if (isNaN(t)) {
            return 0
        }
        if (t >= _h15AlarmC) {
            return 2
        }
        if (t >= _h15WarnC) {
            return 1
        }
        return 0
    }

    function _getH15MaxTempLevel() {
        if (!_h15EscTempLimits || !_escs || _motorCount === 0) {
            return 0
        }
        let maxLevel = 0
        for (let i = 0; i < _motorCount; i++) {
            maxLevel = Math.max(maxLevel, _h15TempLevel(_escs.get(i)))
        }
        return maxLevel
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
            if (_h15TempLevel(esc) !== 0) {
                return false
            }
        }
        return true
    }

    function getEscStatusColor() {
        if (_h15MaxTempLevel === 2) {
            return qgcPal.colorRed
        }
        if (_h15MaxTempLevel === 1) {
            return qgcPal.colorOrange
        }
        return _escHealthy ? qgcPal.colorGreen : qgcPal.colorRed
    }

    function getEscStatusText() {
        if (_h15MaxTempLevel === 2) {
            return qsTr("过温")
        }
        if (_h15MaxTempLevel === 1) {
            return qsTr("高温")
        }
        return _escHealthy ? qsTr("正常") : qsTr("异常")
    }

    function _hottestEscIndex() {
        let bestIdx = -1
        let bestT = -1
        for (let i = 0; i < _motorCount; i++) {
            const esc = _escs.get(i)
            const t = _tempDegC(esc)
            if (isNaN(t)) {
                continue
            }
            if (t > bestT) {
                bestT = t
                bestIdx = i
            }
        }
        return bestIdx
    }

    function _checkH15TempVoice() {
        if (!_h15EscTempLimits || !_escs || _motorCount === 0) {
            return
        }
        const level = _getH15MaxTempLevel()
        if (level === 0) {
            return
        }
        const now = Date.now()
        const hotIdx = _hottestEscIndex()
        const motorNo = hotIdx >= 0 ? (Number(_escs.get(hotIdx).id.rawValue) + 1) : 0
        if (level === 2) {
            if (now - _lastAlarmSpeakMs < _voiceIntervalMs) {
                return
            }
            _lastAlarmSpeakMs = now
            _lastWarnSpeakMs = now
            QGroundControl.say(qsTr("warning ESC %1 overtemperature").arg(motorNo))
            return
        }
        if (now - _lastWarnSpeakMs < _voiceIntervalMs) {
            return
        }
        _lastWarnSpeakMs = now
        QGroundControl.say(qsTr("warning ESC %1 high temperature").arg(motorNo))
    }

    Timer {
        interval: 2000
        running: control._h15EscTempLimits && control.showIndicator
        repeat: true
        onTriggered: control._checkH15TempVoice()
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
            color:              getEscStatusColor()
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
                text:           getEscStatusText()
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
