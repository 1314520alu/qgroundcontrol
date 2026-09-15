import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id:                     telemetryPage
    pageComponent:          pageComponent
    showAdvanced:           false
    showPageDescription:    false

    readonly property int _maxEscs: 16

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: true
    }

    Component {
        id: pageComponent

        Item {
            id: pageRoot
            width:  availableWidth
            height: availableHeight

            property var  _vehicle: globals.activeVehicle
            property var  _escs:    _vehicle ? _vehicle.escs : null
            property int  _rawCount: _escs ? _escs.count : 0
            property int  _escCount: Math.min(_rawCount, telemetryPage._maxEscs)
            property bool _hasTelemetry: _escCount > 0
            property bool _showCards: _escCount > 0 && _escCount <= 4
            property bool _showTable: _escCount > 4
            property bool _denseTable: _escCount > 8
            property int  _onlineCount: _countOnline()
            property int  _healthyCount: _countHealthy()
            property int  _totalErrors: _sumErrors()
            property bool _allHealthy: _hasTelemetry && _healthyCount === _escCount && _onlineCount === _escCount

            property var  _appSettings: QGroundControl.settingsManager.appSettings
            readonly property bool _h15EscTempLimits: _appSettings
                                                      && _appSettings.aircraftModel.rawValue === Xf200TetheredPowerVisual.aircraftModelZyXf200Tethered
            readonly property real _h15WarnC:  90
            readonly property real _h15AlarmC: 105

            readonly property real _gap: Math.max(2, ScreenTools.defaultFontPixelWidth * 0.35)
            readonly property real _hPad: ScreenTools.defaultFontPixelWidth * 0.55
            readonly property real _panelRadius: ScreenTools.defaultBorderRadius
            // Soft zebra — match Servo matrix (avoid harsh windowShadeLight contrast)
            readonly property color _rowAlt: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.04)
            readonly property color _rowFault: Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.10)
            readonly property color _rowWarn: Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.10)

            function _bitOnline(esc) {
                if (!esc) {
                    return false
                }
                const id = Number(esc.id.rawValue)
                const bit = id % 4
                return (Number(esc.info.rawValue) & (1 << bit)) !== 0
            }

            function _errorCount(esc) {
                return esc ? Number(esc.errorCount.rawValue) : 0
            }

            function _hasFaultFlags(esc) {
                return esc ? Number(esc.failureFlags.rawValue) !== 0 : false
            }

            function _isHealthy(esc) {
                return _bitOnline(esc) && !_hasFaultFlags(esc) && _errorCount(esc) === 0 && _h15TempLevel(esc) === 0
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

            function _tempColor(esc) {
                const level = _h15TempLevel(esc)
                if (level === 2) {
                    return qgcPal.colorRed
                }
                if (level === 1) {
                    return qgcPal.colorOrange
                }
                return qgcPal.text
            }

            function _rowFill(index, esc) {
                const tempLevel = _h15TempLevel(esc)
                if (!_bitOnline(esc) || _hasFaultFlags(esc) || tempLevel === 2) {
                    return _rowFault
                }
                if (_errorCount(esc) > 0 || tempLevel === 1) {
                    return _rowWarn
                }
                return (index % 2 === 1) ? _rowAlt : "transparent"
            }

            function _valueColor(esc, emphasizeError) {
                if (!_bitOnline(esc)) {
                    return qgcPal.text
                }
                if (emphasizeError) {
                    return qgcPal.colorRed
                }
                const tempLevel = _h15TempLevel(esc)
                if (tempLevel === 2) {
                    return qgcPal.colorRed
                }
                if (tempLevel === 1 || _hasFaultFlags(esc)) {
                    return qgcPal.colorOrange
                }
                return qgcPal.text
            }

            function _countOnline() {
                if (!_escs) {
                    return 0
                }
                let n = 0
                for (let i = 0; i < _escCount; i++) {
                    if (_bitOnline(_escs.get(i))) {
                        n++
                    }
                }
                return n
            }

            function _countHealthy() {
                if (!_escs) {
                    return 0
                }
                let n = 0
                for (let i = 0; i < _escCount; i++) {
                    if (_isHealthy(_escs.get(i))) {
                        n++
                    }
                }
                return n
            }

            function _sumErrors() {
                if (!_escs) {
                    return 0
                }
                let n = 0
                for (let i = 0; i < _escCount; i++) {
                    n += _errorCount(_escs.get(i))
                }
                return n
            }

            function _sumPowerKw() {
                if (!_escs) {
                    return 0
                }
                let watts = 0
                for (let i = 0; i < _escCount; i++) {
                    const esc = _escs.get(i)
                    watts += Number(esc.voltage.rawValue) * Number(esc.current.rawValue)
                }
                return watts / 1000
            }

            function _protocolLabel() {
                if (!_escs || _rawCount === 0) {
                    return qsTr("—")
                }
                const ct = Number(_escs.get(0).connectionType.rawValue)
                // APM ESC_TELEMETRY path leaves connectionType at 0 (PPM) — avoid misleading PPM label
                if (ct === 0) {
                    return qsTr("电调遥测")
                }
                return _escs.get(0).connectionType.enumStringValue
            }

            function _tempText(esc) {
                const raw = Number(esc.temperature.rawValue)
                if (raw === 32767) {
                    return qsTr("—")
                }
                return (raw / 100).toFixed(1) + " °C"
            }

            function _voltText(esc) {
                return Number(esc.voltage.rawValue).toFixed(1) + " V"
            }

            function _ampText(esc) {
                return Number(esc.current.rawValue).toFixed(1) + " A"
            }

            function _powerText(esc) {
                const watts = Number(esc.voltage.rawValue) * Number(esc.current.rawValue)
                return (watts / 1000).toFixed(2) + " kW"
            }

            function _failureText(esc) {
                if (!_bitOnline(esc)) {
                    return qsTr("离线")
                }
                const flags = Number(esc.failureFlags.rawValue)
                if (flags === 0) {
                    return qsTr("—")
                }
                const labels = []
                if (flags & (1 << 0)) labels.push(qsTr("过流"))
                if (flags & (1 << 1)) labels.push(qsTr("过压"))
                if (flags & (1 << 2)) labels.push(qsTr("过温"))
                if (flags & (1 << 3)) labels.push(qsTr("超速"))
                if (flags & (1 << 4)) labels.push(qsTr("指令异常"))
                if (flags & (1 << 5)) labels.push(qsTr("电机卡死"))
                if (flags & (1 << 6)) labels.push(qsTr("电调故障"))
                return labels.length ? labels.join("、") : qsTr("故障")
            }

            function _indexAccent(esc) {
                if (!_bitOnline(esc)) {
                    return qgcPal.colorRed
                }
                const tempLevel = _h15TempLevel(esc)
                if (tempLevel === 2 || _hasFaultFlags(esc)) {
                    return qgcPal.colorRed
                }
                if (tempLevel === 1 || _errorCount(esc) > 0) {
                    return qgcPal.colorOrange
                }
                return qgcPal.buttonHighlight
            }

            function _dash() { return qsTr("—") }

            ColumnLayout {
                anchors.fill: parent
                spacing: Math.max(2, ScreenTools.defaultFontPixelHeight * 0.15)

                QGCLabel {
                    text: qsTr("电调遥测")
                    font.bold: true
                    font.pointSize: ScreenTools.defaultFontPointSize * 1.05
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: {
                        if (!_hasTelemetry) {
                            return qsTr("等待电调遥测数据（ESC_TELEMETRY / ESC_INFO / ESC_STATUS）")
                        }
                        if (_showCards) {
                            return qsTr("实时电调遥测 · 卡片布局（1–4 路）")
                        }
                        if (_denseTable) {
                            return qsTr("最多 16 路压缩表 · MAVLink 电调遥测")
                        }
                        return qsTr("5–8 路紧凑表 · MAVLink 电调遥测")
                    }
                    font.pointSize: ScreenTools.smallFontPointSize
                    color: qgcPal.text
                    opacity: 0.6
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelWidth * 0.45

                    SummaryStatusPill {
                        visible: _hasTelemetry
                        text: _allHealthy ? qsTr("健康") : qsTr("异常")
                        ok: _allHealthy
                        warn: _hasTelemetry && !_allHealthy && _onlineCount === _escCount
                    }

                    SummaryChip {
                        visible: _hasTelemetry
                        text: qsTr("健康 %1/%2").arg(_healthyCount).arg(_escCount)
                        textColor: _allHealthy ? qgcPal.colorGreen : qgcPal.colorRed
                        border.color: textColor
                    }

                    SummaryChip {
                        visible: _hasTelemetry
                        text: qsTr("协议 %1").arg(_protocolLabel())
                    }

                    SummaryChip {
                        visible: _hasTelemetry
                        text: qsTr("错误 %1").arg(_totalErrors)
                        textColor: _totalErrors > 0 ? qgcPal.colorRed : qgcPal.text
                        border.color: _totalErrors > 0 ? qgcPal.colorRed : qgcPal.buttonBorder
                    }

                    SummaryChip {
                        visible: _hasTelemetry
                        text: qsTr("总功率 %1 kW").arg(pageRoot._sumPowerKw().toFixed(1))
                    }

                    SummaryChip {
                        visible: _hasTelemetry
                        text: qsTr("在线 %1").arg(_onlineCount)
                    }
                }

                Rectangle {
                    visible: !_hasTelemetry
                    Layout.fillWidth: true
                    Layout.preferredHeight: emptyLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.8
                    radius: pageRoot._panelRadius
                    color: Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.08)
                    border.width: 1
                    border.color: qgcPal.colorRed

                    QGCLabel {
                        id: emptyLabel
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        wrapMode: Text.WordWrap
                        color: qgcPal.colorRed
                        text: qsTr("未收到电调遥测。ArduPilot 通常发送 ESC_TELEMETRY_1_TO_4（Mission Planner 状态页的 esc*_rpm）。请确认已开启 DShot 遥测 / DroneCAN，且地面站链路为 MAVLink2。")
                    }
                }

                // ---- Card layout (1–4) ----
                GridLayout {
                    visible: _showCards
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: _escCount <= 1 ? 1 : 2
                    rowSpacing: pageRoot._gap * 1.5
                    columnSpacing: pageRoot._gap * 1.5

                    Repeater {
                        model: _showCards ? _escCount : 0

                        Rectangle {
                            required property int index

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: pageRoot._panelRadius
                            color: qgcPal.window
                            border.width: 1
                            border.color: {
                                const esc = _escs.get(index)
                                const tempLevel = pageRoot._h15TempLevel(esc)
                                if (tempLevel === 2) {
                                    return qgcPal.colorRed
                                }
                                if (tempLevel === 1) {
                                    return qgcPal.colorOrange
                                }
                                return pageRoot._isHealthy(esc) ? qgcPal.colorGreen : qgcPal.colorRed
                            }

                            property var esc: _escs.get(index)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: ScreenTools.defaultFontPixelHeight * 0.45
                                spacing: ScreenTools.defaultFontPixelHeight * 0.25

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.5

                                    Rectangle {
                                        width: ScreenTools.defaultFontPixelHeight * 1.35
                                        height: width
                                        radius: width / 2
                                        color: pageRoot._indexAccent(esc)

                                        QGCLabel {
                                            anchors.centerIn: parent
                                            text: Number(esc.id.rawValue) + 1
                                            color: qgcPal.buttonHighlightText
                                            font.bold: true
                                        }
                                    }

                                    SummaryChip {
                                        text: pageRoot._bitOnline(esc) ? qsTr("在线") : qsTr("离线")
                                        textColor: pageRoot._bitOnline(esc) ? qgcPal.colorGreen : qgcPal.colorRed
                                        border.color: textColor
                                    }

                                    Item { Layout.fillWidth: true }
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2
                                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.2

                                    QGCLabel { text: qsTr("转速"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: esc.rpm.valueString; font.bold: true }

                                    QGCLabel { text: qsTr("电压"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: pageRoot._voltText(esc); font.bold: true }

                                    QGCLabel { text: qsTr("电流"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: pageRoot._ampText(esc); font.bold: true }

                                    QGCLabel { text: qsTr("功率"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel { text: pageRoot._powerText(esc); font.bold: true }

                                    QGCLabel { text: qsTr("温度"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel {
                                        text: pageRoot._tempText(esc)
                                        font.bold: true
                                        color: pageRoot._tempColor(esc)
                                    }

                                    QGCLabel { text: qsTr("错误"); opacity: 0.55; font.pointSize: ScreenTools.smallFontPointSize }
                                    QGCLabel {
                                        text: esc.errorCount.valueString
                                        font.bold: true
                                        color: pageRoot._errorCount(esc) > 0 ? qgcPal.colorRed : qgcPal.text
                                    }

                                    QGCLabel {
                                        text: qsTr("故障")
                                        opacity: 0.55
                                        font.pointSize: ScreenTools.smallFontPointSize
                                        visible: pageRoot._failureText(esc) !== pageRoot._dash()
                                    }
                                    QGCLabel {
                                        text: pageRoot._failureText(esc)
                                        font.bold: true
                                        color: qgcPal.colorRed
                                        visible: pageRoot._failureText(esc) !== pageRoot._dash()
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }

                                Item { Layout.fillHeight: true }
                            }
                        }
                    }
                }

                // ---- Table layout (5–16) ----
                Rectangle {
                    id: matrixPanel
                    visible: _showTable
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    clip: true

                    readonly property real _units: 18 + 32 + 32 + 32 + 34 + 32 + 24 + 48
                    readonly property real _innerW: Math.max(1, width - 2 - pageRoot._hPad * 2 - pageRoot._gap * 7)
                    readonly property real _u: _innerW / _units
                    readonly property real _colIndex: 18 * _u
                    readonly property real _colRpm:   32 * _u
                    readonly property real _colV:     32 * _u
                    readonly property real _colA:     32 * _u
                    readonly property real _colP:     34 * _u
                    readonly property real _colT:     32 * _u
                    readonly property real _colErr:   24 * _u
                    readonly property real _colFault: 48 * _u
                    readonly property real _headerH: Math.max(16, ScreenTools.defaultFontPixelHeight * 1.05)
                    readonly property real _rowH: {
                        const budget = Math.max(1, (height - 2 - _headerH) / Math.max(1, _escCount))
                        return budget
                    }
                    readonly property real _chipSize: Math.min(_colIndex * 0.75, Math.max(12, _rowH * 0.7))
                    // Prefer readable weight over ultra-dense thin glyphs on remotes
                    readonly property real _fontPt: Math.max(
                                                       ScreenTools.defaultFontPointSize * 0.92,
                                                       _denseTable ? ScreenTools.smallFontPointSize * 1.05
                                                                   : ScreenTools.defaultFontPointSize * 0.95)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: matrixPanel._headerH
                            color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.35)

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: pageRoot._hPad
                                anchors.rightMargin: pageRoot._hPad
                                spacing: pageRoot._gap

                                QGCLabel { width: matrixPanel._colIndex; anchors.verticalCenter: parent.verticalCenter; text: qsTr("#"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                                QGCLabel { width: matrixPanel._colRpm; anchors.verticalCenter: parent.verticalCenter; text: qsTr("转速"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                                QGCLabel { width: matrixPanel._colV; anchors.verticalCenter: parent.verticalCenter; text: qsTr("电压"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                                QGCLabel { width: matrixPanel._colA; anchors.verticalCenter: parent.verticalCenter; text: qsTr("电流"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                                QGCLabel { width: matrixPanel._colP; anchors.verticalCenter: parent.verticalCenter; text: qsTr("功率"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                                QGCLabel { width: matrixPanel._colT; anchors.verticalCenter: parent.verticalCenter; text: qsTr("温度"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                                QGCLabel { width: matrixPanel._colErr; anchors.verticalCenter: parent.verticalCenter; text: qsTr("错误"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                                QGCLabel { width: matrixPanel._colFault; anchors.verticalCenter: parent.verticalCenter; text: qsTr("故障"); font.bold: true; font.pointSize: matrixPanel._fontPt }
                            }
                        }

                        Repeater {
                            model: _showTable ? _escCount : 0

                            Rectangle {
                                required property int index

                                Layout.fillWidth: true
                                Layout.preferredHeight: matrixPanel._rowH
                                color: pageRoot._rowFill(index, esc)

                                property var esc: _escs.get(index)
                                property bool online: pageRoot._bitOnline(esc)
                                property bool healthy: pageRoot._isHealthy(esc)
                                property bool hasErrors: pageRoot._errorCount(esc) > 0

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    width: 3
                                    visible: !healthy
                                    color: online ? (hasErrors || pageRoot._hasFaultFlags(esc) ? qgcPal.colorOrange : qgcPal.colorRed)
                                                  : qgcPal.colorRed
                                }

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: pageRoot._hPad
                                    anchors.rightMargin: pageRoot._hPad
                                    spacing: pageRoot._gap

                                    Item {
                                        width: matrixPanel._colIndex
                                        height: parent.height

                                        Rectangle {
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: matrixPanel._chipSize
                                            height: width
                                            radius: width / 2
                                            color: pageRoot._indexAccent(esc)

                                            QGCLabel {
                                                anchors.centerIn: parent
                                                text: Number(esc.id.rawValue) + 1
                                                color: qgcPal.buttonHighlightText
                                                font.bold: true
                                                font.pointSize: matrixPanel._fontPt * 0.95
                                            }
                                        }
                                    }

                                    QGCLabel {
                                        width: matrixPanel._colRpm
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: online ? esc.rpm.valueString : pageRoot._dash()
                                        font.pointSize: matrixPanel._fontPt
                                        font.bold: true
                                        color: pageRoot._valueColor(esc, false)
                                        elide: Text.ElideRight
                                        opacity: online ? 1 : 0.45
                                    }
                                    QGCLabel {
                                        width: matrixPanel._colV
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: online ? pageRoot._voltText(esc) : pageRoot._dash()
                                        font.pointSize: matrixPanel._fontPt
                                        font.bold: true
                                        color: pageRoot._valueColor(esc, false)
                                        elide: Text.ElideRight
                                        opacity: online ? 1 : 0.45
                                    }
                                    QGCLabel {
                                        width: matrixPanel._colA
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: online ? pageRoot._ampText(esc) : pageRoot._dash()
                                        font.pointSize: matrixPanel._fontPt
                                        font.bold: true
                                        color: pageRoot._valueColor(esc, false)
                                        elide: Text.ElideRight
                                        opacity: online ? 1 : 0.45
                                    }
                                    QGCLabel {
                                        width: matrixPanel._colP
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: online ? pageRoot._powerText(esc) : pageRoot._dash()
                                        font.pointSize: matrixPanel._fontPt
                                        font.bold: true
                                        color: pageRoot._valueColor(esc, false)
                                        elide: Text.ElideRight
                                        opacity: online ? 1 : 0.45
                                    }
                                    QGCLabel {
                                        width: matrixPanel._colT
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: online ? pageRoot._tempText(esc) : pageRoot._dash()
                                        font.pointSize: matrixPanel._fontPt
                                        font.bold: true
                                        color: online ? pageRoot._tempColor(esc) : pageRoot._valueColor(esc, false)
                                        elide: Text.ElideRight
                                        opacity: online ? 1 : 0.45
                                    }
                                    QGCLabel {
                                        width: matrixPanel._colErr
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: online ? esc.errorCount.valueString : pageRoot._dash()
                                        font.pointSize: matrixPanel._fontPt
                                        font.bold: true
                                        color: pageRoot._valueColor(esc, hasErrors)
                                        elide: Text.ElideRight
                                        opacity: online ? 1 : 0.45
                                    }

                                    Item {
                                        width: matrixPanel._colFault
                                        height: parent.height

                                        SummaryChip {
                                            anchors.verticalCenter: parent.verticalCenter
                                            visible: pageRoot._failureText(esc) !== pageRoot._dash()
                                            text: pageRoot._failureText(esc)
                                            textColor: online ? qgcPal.colorRed : qgcPal.text
                                            border.color: textColor
                                            fillColor: qgcPal.button
                                        }

                                        QGCLabel {
                                            anchors.verticalCenter: parent.verticalCenter
                                            visible: pageRoot._failureText(esc) === pageRoot._dash()
                                            text: pageRoot._dash()
                                            font.pointSize: matrixPanel._fontPt
                                            font.bold: true
                                            opacity: 0.45
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    visible: !_hasTelemetry
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }
}
