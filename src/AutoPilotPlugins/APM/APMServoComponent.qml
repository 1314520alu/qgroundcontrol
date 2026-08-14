import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SetupPage {
    id:             servoPage
    pageComponent:  pageComponent
    showAdvanced:   false
    showPageDescription: false   // title/hint live in-page (match C lock)

    readonly property int _maxServos: 16

    FactPanelController {
        id: controller
    }

    ServoOutputMonitorController {
        id: servoMonitor
    }

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: true
    }

    function getFact(param) {
        return controller.parameterExists(-1, param)
               ? controller.getParameterFact(-1, param, false)
               : null
    }

    function servoExists(n) {
        return controller.parameterExists(-1, "SERVO" + n + "_FUNCTION")
    }

    Component {
        id: pageComponent

        // Visual lock: Layout C — single compressed matrix, 16 equal rows, no ±.
        Item {
            id: pageRoot
            width:  availableWidth
            height: availableHeight

            // Column ratios from C lock: 22 | 48 | 1.5fr | 40 | 40 | 40 | 36
            readonly property real _gap: Math.max(2, ScreenTools.defaultFontPixelWidth * 0.35)
            readonly property real _hPad: ScreenTools.defaultFontPixelWidth * 0.55
            readonly property real _units: 22 + 48 + 60 + 40 + 40 + 40 + 36   // 1.5fr ≈ 60
            readonly property real _innerW: Math.max(1, matrixPanel.width - 2 - _hPad * 2 - _gap * 6)
            readonly property real _u: _innerW / _units
            readonly property real _colIndex: 22 * _u
            readonly property real _colPos:   48 * _u
            readonly property real _colFunc:  60 * _u
            readonly property real _colNum:   40 * _u
            readonly property real _colRev:   36 * _u

            readonly property real _headerH: Math.max(14, ScreenTools.defaultFontPixelHeight * 0.95)
            // Title + hint ≈ 2 lines; keep controls short so 16 rows fit (C lock).
            readonly property real _rowBudget: Math.max(1, (height - ScreenTools.defaultFontPixelHeight * 2.4 - _headerH) / 16)
            readonly property real _ctrlH: Math.max(12, Math.min(ScreenTools.defaultFontPixelHeight * 1.05, _rowBudget - 3))
            readonly property real _chipSize: Math.min(_colIndex * 0.72, _ctrlH * 0.85)
            readonly property real _barH: Math.min(_ctrlH * 0.78, ScreenTools.defaultFontPixelHeight * 0.9)
            readonly property real _revW: Math.min(_colRev * 0.85, _ctrlH * 1.85)
            readonly property real _revH: Math.min(_ctrlH * 0.7, ScreenTools.defaultFontPixelHeight * 0.75)

            ColumnLayout {
                anchors.fill: parent
                spacing: Math.max(2, ScreenTools.defaultFontPixelHeight * 0.15)

                QGCLabel {
                    text: qsTr("Servo Outputs")
                    font.bold: true
                    font.pointSize: ScreenTools.defaultFontPointSize
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: qsTr("Compressed 16-row table · no horizontal scroll")
                    font.pointSize: ScreenTools.smallFontPointSize
                    color: qgcPal.text
                    opacity: 0.55
                    Layout.fillWidth: true
                }

                // White matrix panel (C lock)
                Rectangle {
                    id: matrixPanel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: ScreenTools.defaultBorderRadius
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        // Header strip
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: pageRoot._headerH
                            color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.06)

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: pageRoot._hPad
                                anchors.rightMargin: pageRoot._hPad
                                spacing: pageRoot._gap

                                Repeater {
                                    model: [
                                        { t: "#", w: pageRoot._colIndex },
                                        { t: qsTr("Position"), w: pageRoot._colPos },
                                        { t: qsTr("Function"), w: pageRoot._colFunc },
                                        { t: qsTr("Min"), w: pageRoot._colNum },
                                        { t: qsTr("Trim"), w: pageRoot._colNum },
                                        { t: qsTr("Max"), w: pageRoot._colNum },
                                        { t: qsTr("Reversed"), w: pageRoot._colRev }
                                    ]
                                    QGCLabel {
                                        required property var modelData
                                        width: modelData.w
                                        height: parent.height
                                        text: modelData.t
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pointSize: ScreenTools.smallFontPointSize * 0.92
                                        opacity: 0.6
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: qgcPal.groupBorder
                            opacity: 0.7
                        }

                        ColumnLayout {
                            id: rowsColumn
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 0

                            Repeater {
                                id: servoRepeater
                                model: _maxServos

                                Rectangle {
                                    id: rowRoot
                                    readonly property int servoIndex: index + 1
                                    readonly property var functionFact: getFact("SERVO" + servoIndex + "_FUNCTION")
                                    readonly property var minFact: getFact("SERVO" + servoIndex + "_MIN")
                                    readonly property var trimFact: getFact("SERVO" + servoIndex + "_TRIM")
                                    readonly property var maxFact: getFact("SERVO" + servoIndex + "_MAX")
                                    readonly property var revFact: getFact("SERVO" + servoIndex + "_REVERSED")
                                    readonly property bool exists: servoExists(servoIndex)

                                    property int pwmValue: servoMonitor.servoValue(index)
                                    readonly property double _rawValue: pwmValue >= 0 ? pwmValue : NaN
                                    readonly property double _minValue: minFact ? minFact.value : NaN
                                    readonly property double _maxValue: maxFact ? maxFact.value : NaN
                                    readonly property double _range: _maxValue - _minValue
                                    readonly property double _ratio: (_range > 0 && !isNaN(_rawValue) && !isNaN(_minValue))
                                                                       ? Math.max(0, Math.min(1, (_rawValue - _minValue) / _range))
                                                                       : 0
                                    readonly property bool _hasValidValue: pwmValue >= 0 && !isNaN(_rawValue)
                                                                          && !isNaN(_minValue) && !isNaN(_maxValue) && _range > 0
                                    // C lock: dim chip when Disabled (function == 0)
                                    readonly property bool _dimChip: !functionFact || Number(functionFact.value) === 0
                                    readonly property bool _revOn: revFact ? (Number(revFact.value) !== 0) : false

                                    visible: exists
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumHeight: 1
                                    color: (index % 2 === 1)
                                           ? Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.025)
                                           : "transparent"

                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width
                                        height: 1
                                        color: qgcPal.groupBorder
                                        opacity: 0.35
                                    }

                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: pageRoot._hPad
                                        anchors.rightMargin: pageRoot._hPad
                                        spacing: pageRoot._gap

                                        // Index chip
                                        Item {
                                            width: pageRoot._colIndex
                                            height: parent.height
                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: pageRoot._chipSize
                                                height: pageRoot._chipSize
                                                radius: Math.max(2, pageRoot._chipSize * 0.22)
                                                color: rowRoot._dimChip ? qgcPal.colorGrey : qgcPal.colorBlue
                                                QGCLabel {
                                                    anchors.centerIn: parent
                                                    text: String(rowRoot.servoIndex)
                                                    font.bold: true
                                                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                                                    color: "#ffffff"
                                                }
                                            }
                                        }

                                        // Position bar
                                        Item {
                                            width: pageRoot._colPos
                                            height: parent.height
                                            Rectangle {
                                                id: track
                                                anchors.verticalCenter: parent.verticalCenter
                                                width: parent.width
                                                height: pageRoot._barH
                                                color: Qt.rgba(qgcPal.colorGrey.r, qgcPal.colorGrey.g, qgcPal.colorGrey.b, 0.35)
                                                border.width: 1
                                                border.color: qgcPal.colorGrey
                                                radius: Math.max(2, height * 0.25)
                                                clip: true

                                                Rectangle {
                                                    anchors.top: parent.top
                                                    anchors.bottom: parent.bottom
                                                    anchors.left: parent.left
                                                    width: rowRoot._hasValidValue
                                                           ? Math.max(0, rowRoot._ratio * parent.width) : 0
                                                    color: qgcPal.colorGreen
                                                }

                                                QGCLabel {
                                                    anchors.centerIn: parent
                                                    z: 1
                                                    text: rowRoot._hasValidValue ? Math.round(rowRoot._rawValue) : "-"
                                                    font.bold: true
                                                    font.pointSize: ScreenTools.smallFontPointSize * 0.85
                                                    horizontalAlignment: Text.AlignHCenter
                                                    width: parent.width
                                                }
                                            }
                                        }

                                        // Function
                                        Item {
                                            width: pageRoot._colFunc
                                            height: parent.height
                                            FactComboBox {
                                                anchors.verticalCenter: parent.verticalCenter
                                                fact: rowRoot.functionFact
                                                indexModel: false
                                                sizeToContents: false
                                                width: parent.width
                                                height: pageRoot._ctrlH
                                                font.pointSize: ScreenTools.smallFontPointSize
                                                padding: 1
                                            }
                                        }

                                        // Min / Trim / Max — compact fields, no ±
                                        Item {
                                            width: pageRoot._colNum
                                            height: parent.height
                                            FactTextField {
                                                anchors.verticalCenter: parent.verticalCenter
                                                fact: rowRoot.minFact
                                                showUnits: false
                                                width: parent.width
                                                height: pageRoot._ctrlH
                                                font.pointSize: ScreenTools.smallFontPointSize
                                                _marginPadding: 1
                                                horizontalAlignment: TextInput.AlignHCenter
                                            }
                                        }
                                        Item {
                                            width: pageRoot._colNum
                                            height: parent.height
                                            FactTextField {
                                                anchors.verticalCenter: parent.verticalCenter
                                                fact: rowRoot.trimFact
                                                showUnits: false
                                                width: parent.width
                                                height: pageRoot._ctrlH
                                                font.pointSize: ScreenTools.smallFontPointSize
                                                _marginPadding: 1
                                                horizontalAlignment: TextInput.AlignHCenter
                                            }
                                        }
                                        Item {
                                            width: pageRoot._colNum
                                            height: parent.height
                                            FactTextField {
                                                anchors.verticalCenter: parent.verticalCenter
                                                fact: rowRoot.maxFact
                                                showUnits: false
                                                width: parent.width
                                                height: pageRoot._ctrlH
                                                font.pointSize: ScreenTools.smallFontPointSize
                                                _marginPadding: 1
                                                horizontalAlignment: TextInput.AlignHCenter
                                            }
                                        }

                                        // Compact reverse switch (C lock pill)
                                        Item {
                                            width: pageRoot._colRev
                                            height: parent.height

                                            Rectangle {
                                                id: revSwitch
                                                anchors.centerIn: parent
                                                width: pageRoot._revW
                                                height: pageRoot._revH
                                                radius: height / 2
                                                color: rowRoot._revOn ? qgcPal.colorBlue : Qt.rgba(qgcPal.colorGrey.r, qgcPal.colorGrey.g, qgcPal.colorGrey.b, 0.55)

                                                Rectangle {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    x: rowRoot._revOn ? parent.width - width - 1 : 1
                                                    width: parent.height - 2
                                                    height: parent.height - 2
                                                    radius: height / 2
                                                    color: "#ffffff"
                                                }

                                                MouseArea {
                                                    anchors.fill: parent
                                                    enabled: rowRoot.revFact !== null
                                                    onClicked: {
                                                        if (!rowRoot.revFact) {
                                                            return
                                                        }
                                                        const onVal = rowRoot.revFact.typeIsBool ? true : 1
                                                        const offVal = rowRoot.revFact.typeIsBool ? false : 0
                                                        rowRoot.revFact.value = rowRoot._revOn ? offVal : onVal
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Connections {
                target: servoMonitor
                function onServoValueChanged(servo, pwmValue) {
                    const item = servoRepeater.itemAt(servo)
                    if (item) {
                        item.pwmValue = pwmValue
                    }
                }
            }
        }
    }
}
