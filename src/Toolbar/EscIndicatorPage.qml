import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ToolIndicatorPage {
    id:             control
    showExpand:     false
    // Drawer sizes from Loader → this page's implicitWidth (width alone does not expand RowLayout)
    implicitWidth:  _panelWidth
    width:          _panelWidth

    property var    activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle
    property var    _escs:          activeVehicle ? activeVehicle.escs : null
    property int    _rawCount:      _escs ? _escs.count : 0
    property int    _escCount:      Math.min(_rawCount, 16)
    property bool   _hasTelemetry:  _escCount > 0
    property bool   _showCards:     _escCount > 0 && _escCount <= 4
    property bool   _showTable:     _escCount > 4
    property bool   _denseTable:    _escCount > 8

    // ZY-XF200系留 uses Hobbywing H15MD Plus — official ESC temp bands.
    property var    _appSettings:   QGroundControl.settingsManager.appSettings
    readonly property bool _h15EscTempLimits: _appSettings
                                              && _appSettings.aircraftModel.rawValue === Xf200TetheredPowerVisual.aircraftModelZyXf200Tethered
    readonly property real _h15WarnC:  90
    readonly property real _h15AlarmC: 105

    // Table mode: content-sized width (12-esc dense on UniRC ≈ 40% screen, not 62%).
    // Card mode: a bit wider for 2×2 labels without stretching empty columns.
    readonly property real _panelWidth: {
        const mw = (typeof mainWindow !== "undefined" && mainWindow && mainWindow.contentItem)
                   ? mainWindow.contentItem.width
                   : ScreenTools.defaultFontPixelWidth * 120
        const fw = ScreenTools.defaultFontPixelWidth
        if (_showTable) {
            const target = mw * 0.46
            return Math.min(fw * 74, Math.max(fw * 56, target))
        }
        const target = mw * 0.48
        return Math.min(fw * 72, Math.max(fw * 52, target))
    }

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: true
    }

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

    /// 0 = ok / N/A, 1 = H15 warn (90–105°C), 2 = H15 alarm (≥105°C). Only active on ZY-XF200系留.
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

    function _dash() { return qsTr("—") }

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

    contentComponent: Component {
        // Item root: Loader/RowLayout honor implicitWidth; ColumnLayout width alone was ignored
        Item {
            id:             pageHost
            width:          control._panelWidth
            implicitWidth:  control._panelWidth
            height:         pageRoot.implicitHeight
            implicitHeight: pageRoot.implicitHeight

            ColumnLayout {
            id:             pageRoot
            spacing:        ScreenTools.defaultFontPixelHeight * 0.2
            width:          pageHost.width

            readonly property real _gap: Math.max(2, ScreenTools.defaultFontPixelWidth * 0.35)
            readonly property real _hPad: ScreenTools.defaultFontPixelWidth * 0.5
            readonly property real _panelRadius: ScreenTools.defaultBorderRadius
            readonly property real _fontPt: control._denseTable
                                            ? ScreenTools.defaultFontPointSize * 0.85
                                            : ScreenTools.defaultFontPointSize * 0.9
            // Soft translucent fills — map shows through indicator drawer
            readonly property color _panelFill: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.42)
            readonly property color _rowAlt: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.05)
            readonly property color _rowFault: Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.14)
            readonly property color _rowWarn: Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.12)
            readonly property color _headerFill: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.28)

            function _rowFill(index, esc) {
                const tempLevel = control._h15TempLevel(esc)
                if (!control._bitOnline(esc) || control._hasFaultFlags(esc) || tempLevel === 2) {
                    return _rowFault
                }
                if (control._errorCount(esc) > 0 || tempLevel === 1) {
                    return _rowWarn
                }
                return (index % 2 === 1) ? _rowAlt : "transparent"
            }

            function _valueColor(esc, emphasizeError) {
                if (!control._bitOnline(esc)) {
                    return qgcPal.text
                }
                if (emphasizeError) {
                    return qgcPal.colorRed
                }
                const tempLevel = control._h15TempLevel(esc)
                if (tempLevel === 2 || control._hasFaultFlags(esc)) {
                    return tempLevel === 2 ? qgcPal.colorRed : qgcPal.colorOrange
                }
                if (tempLevel === 1) {
                    return qgcPal.colorOrange
                }
                return qgcPal.text
            }

            QGCLabel {
                text: qsTr("电调状态概览")
                font.bold: true
                font.pointSize: ScreenTools.defaultFontPointSize * 0.95
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("正常电机 %1/%2").arg(control._countHealthy()).arg(control._escCount)
                    font.pointSize: pageRoot._fontPt
                    font.bold: true
                    color: control._countHealthy() === control._escCount && control._escCount > 0
                           ? qgcPal.colorGreen : qgcPal.colorRed
                    visible: control._hasTelemetry
                }

                QGCLabel {
                    text: qsTr("错误总数 %1").arg(control._sumErrors())
                    font.pointSize: pageRoot._fontPt
                    font.bold: true
                    color: control._sumErrors() > 0 ? qgcPal.colorRed : qgcPal.text
                    visible: control._hasTelemetry
                }

                QGCLabel {
                    text: qsTr("总功率 %1 kW").arg(control._sumPowerKw().toFixed(1))
                    font.pointSize: pageRoot._fontPt
                    font.bold: true
                    visible: control._hasTelemetry
                }

                Item { Layout.fillWidth: true }
            }

            QGCLabel {
                visible: !control._hasTelemetry
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: qgcPal.colorRed
                font.pointSize: pageRoot._fontPt
                text: qsTr("未收到电调遥测")
            }

            // ---- Cards (1–4) ----
            GridLayout {
                visible: control._showCards
                Layout.fillWidth: true
                columns: control._escCount <= 1 ? 1 : 2
                rowSpacing: pageRoot._gap
                columnSpacing: pageRoot._gap

                Repeater {
                    model: control._showCards ? control._escCount : 0

                    Rectangle {
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 8.0
                        radius: pageRoot._panelRadius
                        color: {
                            const esc = control._escs.get(index)
                            if (control._errorCount(esc) > 0 || control._hasFaultFlags(esc) || !control._bitOnline(esc)
                                    || control._h15TempLevel(esc) === 2) {
                                return pageRoot._rowFault
                            }
                            if (control._h15TempLevel(esc) === 1) {
                                return pageRoot._rowWarn
                            }
                            return pageRoot._panelFill
                        }
                        border.width: 1
                        border.color: {
                            const esc = control._escs.get(index)
                            const tempLevel = control._h15TempLevel(esc)
                            if (tempLevel === 2) {
                                return qgcPal.colorRed
                            }
                            if (tempLevel === 1) {
                                return qgcPal.colorOrange
                            }
                            return control._isHealthy(esc) ? qgcPal.colorGreen : qgcPal.colorRed
                        }

                        property var esc: control._escs.get(index)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: ScreenTools.defaultFontPixelWidth * 0.55
                            spacing: ScreenTools.defaultFontPixelHeight * 0.12

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: ScreenTools.defaultFontPixelWidth * 0.4

                                Rectangle {
                                    width: ScreenTools.defaultFontPixelHeight * 1.15
                                    height: width
                                    radius: width / 2
                                    color: control._indexAccent(esc)

                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: Number(esc.id.rawValue) + 1
                                        color: qgcPal.buttonHighlightText
                                        font.bold: true
                                        font.pointSize: pageRoot._fontPt
                                    }
                                }

                                QGCLabel {
                                    text: control._bitOnline(esc)
                                          ? qsTr("电机 %1").arg(Number(esc.id.rawValue) + 1)
                                          : qsTr("电机 %1 - 离线").arg(Number(esc.id.rawValue) + 1)
                                    font.bold: true
                                    font.pointSize: pageRoot._fontPt
                                    color: control._bitOnline(esc) ? qgcPal.text : qgcPal.colorRed
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: ScreenTools.defaultFontPixelWidth
                                rowSpacing: 1

                                QGCLabel { text: qsTr("转速"); opacity: 0.55; font.pointSize: pageRoot._fontPt * 0.92 }
                                QGCLabel { text: esc.rpm.valueString; font.bold: true; font.pointSize: pageRoot._fontPt }

                                QGCLabel { text: qsTr("电压"); opacity: 0.55; font.pointSize: pageRoot._fontPt * 0.92 }
                                QGCLabel { text: control._voltText(esc); font.bold: true; font.pointSize: pageRoot._fontPt }

                                QGCLabel { text: qsTr("电流"); opacity: 0.55; font.pointSize: pageRoot._fontPt * 0.92 }
                                QGCLabel { text: control._ampText(esc); font.bold: true; font.pointSize: pageRoot._fontPt }

                                QGCLabel { text: qsTr("功率"); opacity: 0.55; font.pointSize: pageRoot._fontPt * 0.92 }
                                QGCLabel { text: control._powerText(esc); font.bold: true; font.pointSize: pageRoot._fontPt }

                                QGCLabel { text: qsTr("温度"); opacity: 0.55; font.pointSize: pageRoot._fontPt * 0.92 }
                                QGCLabel {
                                    text: control._tempText(esc)
                                    font.bold: true
                                    font.pointSize: pageRoot._fontPt
                                    color: control._tempColor(esc)
                                }

                                QGCLabel { text: qsTr("错误"); opacity: 0.55; font.pointSize: pageRoot._fontPt * 0.92 }
                                QGCLabel {
                                    text: esc.errorCount.valueString
                                    font.bold: true
                                    font.pointSize: pageRoot._fontPt
                                    color: control._errorCount(esc) > 0 ? qgcPal.colorRed : qgcPal.text
                                }
                            }
                        }
                    }
                }
            }

            // ---- Table (5–16) ----
            Rectangle {
                id: matrixPanel
                visible: control._showTable
                Layout.fillWidth: true
                Layout.preferredHeight: _headerH + control._escCount * _rowH + 2
                color: pageRoot._panelFill
                border.width: 1
                border.color: Qt.rgba(qgcPal.groupBorder.r, qgcPal.groupBorder.g, qgcPal.groupBorder.b, 0.55)
                radius: pageRoot._panelRadius
                clip: true

                // Prefer content floors; weights only share leftover. rpm/err/fault stay narrow.
                readonly property real _fw: ScreenTools.defaultFontPixelWidth
                readonly property real _units: 12 + 18 + 28 + 20 + 26 + 24 + 12 + 14
                readonly property real _innerW: Math.max(1, width - 2 - pageRoot._hPad * 2 - pageRoot._gap * 7)
                readonly property real _u: _innerW / _units
                readonly property real _colIndex: Math.max(12 * _u, _fw * 2.0)
                readonly property real _colRpm:   Math.max(18 * _u, _fw * 3.0)
                readonly property real _colV:     Math.max(28 * _u, _fw * 6.5)
                readonly property real _colA:     Math.max(20 * _u, _fw * 3.8)
                readonly property real _colP:     Math.max(26 * _u, _fw * 6.2)
                readonly property real _colT:     Math.max(24 * _u, _fw * 5.5)
                readonly property real _colErr:   Math.max(12 * _u, _fw * 2.6)
                readonly property real _colFault: Math.max(14 * _u, _fw * 2.8)
                readonly property real _headerH: Math.max(15, ScreenTools.defaultFontPixelHeight * 0.95)
                readonly property real _rowH: control._denseTable
                                             ? Math.max(16, ScreenTools.defaultFontPixelHeight * 1.0)
                                             : Math.max(18, ScreenTools.defaultFontPixelHeight * 1.15)
                readonly property real _chipSize: Math.min(_colIndex * 0.85, Math.max(11, _rowH * 0.72))

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: matrixPanel._headerH
                        color: pageRoot._headerFill

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: pageRoot._hPad
                            anchors.rightMargin: pageRoot._hPad
                            spacing: pageRoot._gap

                            QGCLabel { width: matrixPanel._colIndex; anchors.verticalCenter: parent.verticalCenter; text: qsTr("#"); font.bold: true; font.pointSize: pageRoot._fontPt }
                            QGCLabel { width: matrixPanel._colRpm; anchors.verticalCenter: parent.verticalCenter; text: qsTr("转速"); font.bold: true; font.pointSize: pageRoot._fontPt }
                            QGCLabel { width: matrixPanel._colV; anchors.verticalCenter: parent.verticalCenter; text: qsTr("电压"); font.bold: true; font.pointSize: pageRoot._fontPt }
                            QGCLabel { width: matrixPanel._colA; anchors.verticalCenter: parent.verticalCenter; text: qsTr("电流"); font.bold: true; font.pointSize: pageRoot._fontPt }
                            QGCLabel { width: matrixPanel._colP; anchors.verticalCenter: parent.verticalCenter; text: qsTr("功率"); font.bold: true; font.pointSize: pageRoot._fontPt }
                            QGCLabel { width: matrixPanel._colT; anchors.verticalCenter: parent.verticalCenter; text: qsTr("温度"); font.bold: true; font.pointSize: pageRoot._fontPt }
                            QGCLabel { width: matrixPanel._colErr; anchors.verticalCenter: parent.verticalCenter; text: qsTr("错误"); font.bold: true; font.pointSize: pageRoot._fontPt }
                            QGCLabel { width: matrixPanel._colFault; anchors.verticalCenter: parent.verticalCenter; text: qsTr("故障"); font.bold: true; font.pointSize: pageRoot._fontPt }
                        }
                    }

                    Repeater {
                        model: control._showTable ? control._escCount : 0

                        Rectangle {
                            required property int index

                            Layout.fillWidth: true
                            Layout.preferredHeight: matrixPanel._rowH
                            color: pageRoot._rowFill(index, esc)

                            property var esc: control._escs.get(index)
                            property bool online: control._bitOnline(esc)
                            property bool healthy: control._isHealthy(esc)
                            property bool hasErrors: control._errorCount(esc) > 0

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: 2
                                visible: !healthy
                                color: online ? (hasErrors || control._hasFaultFlags(esc) ? qgcPal.colorOrange : qgcPal.colorRed)
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
                                        color: control._indexAccent(esc)

                                        QGCLabel {
                                            anchors.centerIn: parent
                                            text: Number(esc.id.rawValue) + 1
                                            color: qgcPal.buttonHighlightText
                                            font.bold: true
                                            font.pointSize: pageRoot._fontPt * 0.92
                                        }
                                    }
                                }

                                QGCLabel {
                                    width: matrixPanel._colRpm
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: online ? esc.rpm.valueString : control._dash()
                                    font.pointSize: pageRoot._fontPt
                                    font.bold: true
                                    color: pageRoot._valueColor(esc, false)
                                    opacity: online ? 1 : 0.45
                                }
                                QGCLabel {
                                    width: matrixPanel._colV
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: online ? control._voltText(esc) : control._dash()
                                    font.pointSize: pageRoot._fontPt
                                    font.bold: true
                                    color: pageRoot._valueColor(esc, false)
                                    opacity: online ? 1 : 0.45
                                }
                                QGCLabel {
                                    width: matrixPanel._colA
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: online ? control._ampText(esc) : control._dash()
                                    font.pointSize: pageRoot._fontPt
                                    font.bold: true
                                    color: pageRoot._valueColor(esc, false)
                                    opacity: online ? 1 : 0.45
                                }
                                QGCLabel {
                                    width: matrixPanel._colP
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: online ? control._powerText(esc) : control._dash()
                                    font.pointSize: pageRoot._fontPt
                                    font.bold: true
                                    color: pageRoot._valueColor(esc, false)
                                    opacity: online ? 1 : 0.45
                                }
                                QGCLabel {
                                    width: matrixPanel._colT
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: online ? control._tempText(esc) : control._dash()
                                    font.pointSize: pageRoot._fontPt
                                    font.bold: true
                                    color: online ? control._tempColor(esc) : pageRoot._valueColor(esc, false)
                                    opacity: online ? 1 : 0.45
                                }
                                QGCLabel {
                                    width: matrixPanel._colErr
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: online ? esc.errorCount.valueString : control._dash()
                                    font.pointSize: pageRoot._fontPt
                                    font.bold: true
                                    color: pageRoot._valueColor(esc, hasErrors)
                                    opacity: online ? 1 : 0.45
                                }
                                QGCLabel {
                                    width: matrixPanel._colFault
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: control._failureText(esc)
                                    font.pointSize: pageRoot._fontPt
                                    font.bold: true
                                    color: control._failureText(esc) !== control._dash() ? qgcPal.colorRed : qgcPal.text
                                    elide: Text.ElideRight
                                    opacity: control._failureText(esc) !== control._dash() ? 1 : 0.45
                                }
                            }
                        }
                    }
                }
            }
            } // ColumnLayout pageRoot
        } // Item pageHost
    }
}
