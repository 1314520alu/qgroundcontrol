import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id: escPage
    pageComponent: escPageComponent
    showPageDescription: false   // title/hint live in left panel (match lock)

    FactPanelController {
        id: controller
    }

    Component {
        id: escPageComponent

        Item {
            id: pageRoot
            width: availableWidth
            height: availableHeight

            // ESC Configuration properties - supports both MOT_* (Copter/Rover/Sub) and Q_M_* (QuadPlane) prefixes
            property bool _isQuadPlane: !controller.parameterExists(-1, "MOT_PWM_TYPE") && controller.parameterExists(-1, "Q_M_PWM_TYPE")
            property string _escPrefix: _isQuadPlane ? "Q_M_" : "MOT_"

            property bool _motPwmTypeAvailable: controller.parameterExists(-1, _escPrefix + "PWM_TYPE")
            property bool _motPwmMinAvailable: controller.parameterExists(-1, _escPrefix + "PWM_MIN")
            property bool _motPwmMaxAvailable: controller.parameterExists(-1, _escPrefix + "PWM_MAX")
            property bool _motSpinArmAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_ARM")
            property bool _motSpinMinAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_MIN")
            property bool _motSpinMaxAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_MAX")
            property bool _servoDshotEscAvailable: controller.parameterExists(-1, "SERVO_DSHOT_ESC")
            property bool _servoDshotRateAvailable: controller.parameterExists(-1, "SERVO_DSHOT_RATE")

            property Fact _motPwmType: controller.getParameterFact(-1, _escPrefix + "PWM_TYPE", false /* reportMissing */)
            property Fact _motPwmMin: controller.getParameterFact(-1, _escPrefix + "PWM_MIN", false /* reportMissing */)
            property Fact _motPwmMax: controller.getParameterFact(-1, _escPrefix + "PWM_MAX", false /* reportMissing */)
            property Fact _motSpinArm: controller.getParameterFact(-1, _escPrefix + "SPIN_ARM", false /* reportMissing */)
            property Fact _motSpinMin: controller.getParameterFact(-1, _escPrefix + "SPIN_MIN", false /* reportMissing */)
            property Fact _motSpinMax: controller.getParameterFact(-1, _escPrefix + "SPIN_MAX", false /* reportMissing */)
            property Fact _servoDshotEsc: controller.getParameterFact(-1, "SERVO_DSHOT_ESC", false /* reportMissing */)
            property Fact _servoDshotRate: controller.getParameterFact(-1, "SERVO_DSHOT_RATE", false /* reportMissing */)

            property bool _isDshot: _motPwmTypeAvailable && _motPwmType && _motPwmType.rawValue >= 4

            property string _escCalParam: _isQuadPlane ? "Q_ESC_CAL" : "ESC_CALIBRATION"
            property bool _escCalibrationAvailable: controller.parameterExists(-1, _escCalParam)
            property Fact _escCalibration: controller.getParameterFact(-1, _escCalParam, false /* reportMissing */)

            property string _restartRequired: qsTr("Requires vehicle reboot")

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            readonly property real _pad: ScreenTools.defaultFontPixelHeight * 0.55
            readonly property real _gap: ScreenTools.defaultFontPixelWidth * 1.2
            readonly property real _panelRadius: ScreenTools.defaultBorderRadius
            // Match MotorComponent (~×50); UniRC 10 Pro / G20 content viewport (~690–780px after sidebar) needs side-by-side Layout A.
            readonly property bool _split: width >= ScreenTools.defaultFontPixelWidth * 48

            readonly property var _calSteps: [
                qsTr("Disconnect USB and battery so the flight controller powers down"),
                qsTr("Connect the battery"),
                qsTr("The arming tone will play (if a buzzer is attached)"),
                qsTr("If there is a safety button, press until solid red"),
                qsTr("You will hear a musical tone then two beeps"),
                qsTr("A few seconds later, beeps for each battery cell"),
                qsTr("A single long beep means end points are set"),
                qsTr("Disconnect the battery and power up normally")
            ]

            Component {
                id: configPanelBody

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: pageRoot._pad
                    spacing: ScreenTools.defaultFontPixelHeight * 0.45

                    QGCLabel {
                        text: qsTr("Configuration")
                        font.bold: true
                    }
                    QGCLabel {
                        text: qsTr("Configure and calibrate electronic speed controllers.")
                        font.pointSize: ScreenTools.smallFontPointSize
                        opacity: 0.55
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelWidth
                        visible: _motPwmTypeAvailable

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QGCLabel { text: qsTr("Output type") }
                            FactComboBox {
                                fact: _motPwmType
                                indexModel: false
                                Layout.fillWidth: true
                            }
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignBottom
                            radius: height / 2
                            color: qgcPal.button
                            border.color: qgcPal.buttonBorder
                            border.width: 1
                            implicitHeight: ScreenTools.implicitButtonHeight * 0.85
                            implicitWidth: rebootLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
                            QGCLabel {
                                id: rebootLabel
                                anchors.centerIn: parent
                                text: pageRoot._restartRequired
                                font.pointSize: ScreenTools.smallFontPointSize
                                color: qgcPal.text
                                opacity: 0.7
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelWidth
                        visible: _motPwmMinAvailable || _motPwmMaxAvailable

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            visible: _motPwmMinAvailable
                            QGCLabel { text: qsTr("Output PWM min") }
                            FactTextField {
                                fact: _motPwmMin
                                Layout.fillWidth: true
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            visible: _motPwmMaxAvailable
                            QGCLabel { text: qsTr("Output PWM max") }
                            FactTextField {
                                fact: _motPwmMax
                                Layout.fillWidth: true
                            }
                        }
                    }

                    LabelledFactTextField {
                        label: qsTr("Spin when armed")
                        fact: _motSpinArm
                        visible: _motSpinArmAvailable
                        textFieldShowHelp: true
                        Layout.fillWidth: true
                    }
                    LabelledFactTextField {
                        label: qsTr("Spin minimum")
                        fact: _motSpinMin
                        visible: _motSpinMinAvailable
                        textFieldShowHelp: true
                        Layout.fillWidth: true
                    }
                    LabelledFactTextField {
                        label: qsTr("Spin maximum")
                        fact: _motSpinMax
                        visible: _motSpinMaxAvailable
                        textFieldShowHelp: true
                        Layout.fillWidth: true
                    }

                    LabelledFactComboBox {
                        label: qsTr("DShot ESC type")
                        fact: _servoDshotEsc
                        indexModel: false
                        visible: _isDshot && _servoDshotEscAvailable
                        Layout.fillWidth: true
                    }
                    LabelledFactComboBox {
                        label: qsTr("DShot output rate")
                        fact: _servoDshotRate
                        indexModel: false
                        visible: _isDshot && _servoDshotRateAvailable
                        Layout.fillWidth: true
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Component {
                id: calPanelContent

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: pageRoot._pad
                    spacing: ScreenTools.defaultFontPixelHeight * 0.4
                    visible: _escCalibrationAvailable

                    QGCLabel {
                        text: qsTr("Calibration")
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: ScreenTools.defaultBorderRadius
                        color: Qt.rgba(qgcPal.warningText.r, qgcPal.warningText.g, qgcPal.warningText.b, 0.12)
                        border.color: qgcPal.warningText
                        border.width: 1
                        implicitHeight: warnRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.5

                        RowLayout {
                            id: warnRow
                            anchors.fill: parent
                            anchors.margins: ScreenTools.defaultFontPixelWidth * 0.8
                            spacing: ScreenTools.defaultFontPixelWidth * 0.6

                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("WARNING: Remove props prior to calibration!")
                                color: qgcPal.warningText
                                wrapMode: Text.WordWrap
                                font.bold: true
                            }
                        }
                    }

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Calibrate")
                        backgroundColor: qgcPal.buttonHighlight
                        textColor: qgcPal.buttonHighlightText
                        enabled: _escCalibration && _escCalibration.rawValue === 0
                        opacity: enabled ? 1.0 : 0.72
                        onClicked: if (_escCalibration) _escCalibration.rawValue = 3
                    }

                    QGCFlickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentWidth: width
                        contentHeight: stepsCol.implicitHeight
                        clip: true

                        ColumnLayout {
                            id: stepsCol
                            width: parent.width
                            spacing: ScreenTools.defaultFontPixelHeight * 0.28
                            enabled: _escCalibration && _escCalibration.rawValue === 3
                            opacity: enabled ? 1.0 : 0.55

                            Repeater {
                                model: pageRoot._calSteps
                                delegate: Rectangle {
                                    required property int index
                                    required property string modelData
                                    Layout.fillWidth: true
                                    radius: ScreenTools.defaultBorderRadius
                                    color: qgcPal.windowShade
                                    border.width: 1
                                    border.color: qgcPal.groupBorder
                                    implicitHeight: stepRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.35

                                    RowLayout {
                                        id: stepRow
                                        anchors.fill: parent
                                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.6
                                        spacing: ScreenTools.defaultFontPixelWidth * 0.7

                                        Rectangle {
                                            width: ScreenTools.defaultFontPixelHeight * 1.35
                                            height: width
                                            radius: width / 2
                                            color: qgcPal.buttonHighlight
                                            QGCLabel {
                                                anchors.centerIn: parent
                                                text: String(index + 1)
                                                color: qgcPal.buttonHighlightText
                                                font.bold: true
                                            }
                                        }
                                        QGCLabel {
                                            Layout.fillWidth: true
                                            text: modelData
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: pageRoot._gap
                visible: pageRoot._split

                Rectangle {
                    id: configPanel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 55
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    clip: true

                    Loader {
                        anchors.fill: parent
                        active: pageRoot._split
                        sourceComponent: configPanelBody
                    }
                }

                Rectangle {
                    id: calPanel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 45
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    clip: true

                    Loader {
                        anchors.fill: parent
                        active: pageRoot._split
                        sourceComponent: calPanelContent
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: pageRoot._gap
                visible: !pageRoot._split

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    clip: true

                    Loader {
                        anchors.fill: parent
                        active: !pageRoot._split
                        sourceComponent: configPanelBody
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    radius: pageRoot._panelRadius
                    clip: true

                    Loader {
                        anchors.fill: parent
                        active: !pageRoot._split
                        sourceComponent: calPanelContent
                    }
                }
            }
        }
    }

}
