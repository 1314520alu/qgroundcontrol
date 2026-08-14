import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id:             flightModePage
    pageComponent:  flightModePageComponent

    readonly property string _modeChannelParam: controller.modeChannelParam
    readonly property string _modeParamPrefix:  controller.modeParamPrefix
    readonly property var    _pwmStrings:       [ "0–1230", "1231–1360", "1361–1490", "1491–1620", "1621–1749", "1750+" ]

    property real   _margins:           ScreenTools.defaultFontPixelHeight * 0.5
    property real   _rowHeight:         ScreenTools.defaultFontPixelHeight * 2.2
    property real   _comboWidth:        ScreenTools.defaultFontPixelWidth * 18
    property real   _maxContentWidth:   ScreenTools.defaultFontPixelWidth * 90
    property Fact   _nullFact
    property bool   _fltmodeChExists:   controller.parameterExists(-1, _modeChannelParam)
    property Fact   _fltmodeCh:         _fltmodeChExists ? controller.getParameterFact(-1, _modeChannelParam) : _nullFact
    property bool   _ch7OptAvailable:   controller.parameterExists(-1, "CH7_OPT")
    property int    _rcOptionStart:     _ch7OptAvailable ? 7 : 6
    property int    _rcOptionStop:      _ch7OptAvailable ? 12 : 16
    property bool   _customSimpleMode:  controller.simpleMode === APMFlightModesComponentController.SimpleModeCustom
    property int    _activeMode:        controller.activeFlightMode

    function _modeFact(modeIndex) {
        return controller.getParameterFact(-1, _modeParamPrefix + modeIndex)
    }

    function _activeModeName() {
        if (_activeMode < 1 || _activeMode > 6) {
            return ""
        }
        return _modeFact(_activeMode).enumStringValue
    }

    function _channelOptionActive(modelIndex) {
        return controller.channelOptionEnabled[modelIndex + (_ch7OptAvailable ? 1 : 0)]
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    APMFlightModesComponentController {
        id: controller
    }

    Component {
        id: flightModePageComponent

        ColumnLayout {
            width:   Math.min(availableWidth, _maxContentWidth)
            spacing: _margins

            SettingsGroupLayout {
                Layout.fillWidth: true
                heading:          qsTr("Flight Mode Settings") + (_fltmodeChExists ? "" : qsTr(" (Channel 5)"))
                contentSpacing:   _margins * 0.75

                RowLayout {
                    Layout.fillWidth: true
                    spacing:          _margins
                    visible:          _fltmodeChExists

                    QGCLabel {
                        text:             qsTr("Flight mode channel:")
                        Layout.alignment: Qt.AlignVCenter
                    }

                    QGCComboBox {
                        sizeToContents:      true
                        Layout.maximumWidth: _comboWidth
                        model: [
                            qsTr("Not assigned"), qsTr("Channel 1"), qsTr("Channel 2"),
                            qsTr("Channel 3"), qsTr("Channel 4"), qsTr("Channel 5"),
                            qsTr("Channel 6"), qsTr("Channel 7"), qsTr("Channel 8")
                        ]
                        currentIndex: _fltmodeCh.value
                        onActivated: (index) => { _fltmodeCh.value = index }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        visible:                _activeMode >= 1 && _activeMode <= 6
                        radius:                 height / 2
                        color:                  Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.18)
                        border.width:           1
                        border.color:           qgcPal.colorOrange
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.6
                        Layout.preferredWidth:  statusPillLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 2

                        QGCLabel {
                            id:               statusPillLabel
                            anchors.centerIn: parent
                            text:             qsTr("Current: Flight Mode %1 · %2").arg(_activeMode).arg(_activeModeName())
                            color:            qgcPal.colorOrange
                            font.pointSize:   ScreenTools.smallFontPointSize
                        }
                    }
                }

                Repeater {
                    model: 6

                    Rectangle {
                        id:                     modeRow
                        Layout.fillWidth:       true
                        Layout.preferredHeight: Math.max(_rowHeight, modeCombo.implicitHeight + _margins * 0.5)
                        color:                  isActive
                                                ? Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.10)
                                                : "transparent"
                        radius:                 ScreenTools.defaultBorderRadius

                        readonly property int  modeIndex: modelData + 1
                        readonly property bool isActive:  _activeMode === modeIndex

                        Rectangle {
                            anchors.left:   parent.left
                            anchors.top:    parent.top
                            anchors.bottom: parent.bottom
                            width:          Math.max(2, ScreenTools.defaultFontPixelWidth * 0.35)
                            radius:         width / 2
                            color:          qgcPal.colorOrange
                            visible:        modeRow.isActive
                        }

                        RowLayout {
                            anchors.fill:        parent
                            anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                            anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.5
                            spacing:             ScreenTools.defaultFontPixelWidth

                            Rectangle {
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.3
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.3
                                radius:                 width / 2
                                color:                  modeRow.isActive ? qgcPal.colorOrange : qgcPal.windowShade
                                border.width:           modeRow.isActive ? 0 : 1
                                border.color:           qgcPal.groupBorder

                                QGCLabel {
                                    anchors.centerIn: parent
                                    text:             modeRow.modeIndex
                                    font.bold:        true
                                    color:            modeRow.isActive ? "#ffffff" : qgcPal.text
                                }
                            }

                            QGCLabel {
                                text:                  qsTr("Flight Mode ") + modeRow.modeIndex
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 12
                                font.bold:             modeRow.isActive
                            }

                            FactComboBox {
                                id:                  modeCombo
                                sizeToContents:      true
                                Layout.maximumWidth: _comboWidth
                                Layout.preferredWidth: _comboWidth
                                fact:                _modeFact(modeRow.modeIndex)
                                indexModel:          false
                            }

                            Rectangle {
                                visible:                modeRow.isActive
                                radius:                 height / 2
                                color:                  qgcPal.colorOrange
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                                Layout.preferredWidth:  currentTag.implicitWidth + ScreenTools.defaultFontPixelWidth

                                QGCLabel {
                                    id:               currentTag
                                    anchors.centerIn: parent
                                    text:             qsTr("Current")
                                    color:            "#ffffff"
                                    font.pointSize:   ScreenTools.smallFontPointSize
                                    font.bold:        true
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                radius:                 ScreenTools.defaultBorderRadius
                                color:                  qgcPal.windowShade
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.3
                                Layout.preferredWidth:  pwmLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.5

                                QGCLabel {
                                    id:               pwmLabel
                                    anchors.centerIn: parent
                                    text:             _pwmStrings[modelData]
                                    font.pointSize:   ScreenTools.smallFontPointSize
                                }
                            }

                            QGCCheckBox {
                                visible:   _customSimpleMode
                                checked:   controller.simpleModeEnabled[modelData]
                                text:      qsTr("Simple")
                                onClicked: controller.setSimpleMode(modelData, checked)
                            }

                            QGCCheckBox {
                                visible:   _customSimpleMode
                                checked:   controller.superSimpleModeEnabled[modelData]
                                text:      qsTr("Super-Simple")
                                onClicked: controller.setSuperSimpleMode(modelData, checked)
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: _margins
                    visible: controller.simpleModesSupported

                    QGCLabel {
                        text:             qsTr("Simple Mode")
                        Layout.alignment: Qt.AlignVCenter
                    }

                    QGCComboBox {
                        model:        controller.simpleModeNames
                        currentIndex: controller.simpleMode
                        onActivated: (index) => { controller.simpleMode = index }
                    }
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth: true
                heading:          qsTr("Switch Options")
                contentSpacing:   _margins * 0.75

                Repeater {
                    model: _rcOptionStop - _rcOptionStart + 1

                    Rectangle {
                        id:                     optionRow
                        Layout.fillWidth:       true
                        Layout.preferredHeight: Math.max(_rowHeight, optCombo.implicitHeight + _margins * 0.5)
                        color:                  isActive
                                                ? Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.10)
                                                : "transparent"
                        radius:                 ScreenTools.defaultBorderRadius

                        readonly property int  channelIndex: modelData + _rcOptionStart
                        readonly property bool isActive:     _channelOptionActive(modelData)

                        Rectangle {
                            anchors.left:   parent.left
                            anchors.top:    parent.top
                            anchors.bottom: parent.bottom
                            width:          Math.max(2, ScreenTools.defaultFontPixelWidth * 0.35)
                            radius:         width / 2
                            color:          qgcPal.colorOrange
                            visible:        optionRow.isActive
                        }

                        RowLayout {
                            anchors.fill:        parent
                            anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                            anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.5
                            spacing:             ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                text:                  qsTr("Channel option %1 :").arg(optionRow.channelIndex)
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                                font.bold:             optionRow.isActive
                                Layout.alignment:      Qt.AlignVCenter
                            }

                            FactComboBox {
                                id:                  optCombo
                                sizeToContents:      true
                                Layout.maximumWidth: _comboWidth * 1.4
                                Layout.fillWidth:    true
                                fact:                controller.getParameterFact(-1, "RC" + optionRow.channelIndex + "_OPTION")
                                indexModel:          false
                            }

                            Rectangle {
                                visible:                optionRow.isActive
                                radius:                 height / 2
                                color:                  qgcPal.colorOrange
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                                Layout.preferredWidth:  optionCurrentTag.implicitWidth + ScreenTools.defaultFontPixelWidth

                                QGCLabel {
                                    id:               optionCurrentTag
                                    anchors.centerIn: parent
                                    text:             qsTr("Current")
                                    color:            "#ffffff"
                                    font.pointSize:   ScreenTools.smallFontPointSize
                                    font.bold:        true
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
