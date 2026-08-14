import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

/// Base class for Remote Control Calibration (supports both RC and Joystick)
/// Layout: stick preview | attitude channels, channel monitor below (radio-cal-preview-split).
Item {
    id: root

    required property var controller
    property Component additionalSetupComponent
    property Component additionalMonitorComponent
    /// When true (Radio page), fill parent and keep all cards in one viewport — no page scroll.
    property bool compactSinglePage: false

    // Controllers need access to these UI elements
    property alias statusText: statusText
    property alias cancelButton: cancelButton
    property alias nextButton: nextButton

    property bool useDeadband: false
    property bool _deadbandActive: useDeadband

    property real _cardRadius: ScreenTools.defaultFontPixelHeight * 0.4
    property real _cardPad: compactSinglePage
                            ? ScreenTools.defaultFontPixelHeight * 0.35
                            : ScreenTools.defaultFontPixelHeight * 0.85
    property real _rowGap: compactSinglePage
                           ? ScreenTools.defaultFontPixelHeight * 0.25
                           : ScreenTools.defaultFontPixelHeight * 0.85
    property real _stickSize: {
        return Math.min(ScreenTools.defaultFontPixelHeight * 6.2,
                        Math.max(ScreenTools.defaultFontPixelHeight * 4.8,
                                 (width * 0.42) - ScreenTools.defaultFontPixelWidth * 8))
    }
    readonly property bool _split: width >= ScreenTools.defaultFontPixelWidth * 55
    readonly property color _cardColor: qgcPal.window
    readonly property color _pageColor: qgcPal.windowShade
    readonly property color _cardBorder: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.10)
    readonly property int _monitorChannelStart: compactSinglePage ? 5 : 1
    readonly property int _monitorChannels: {
        const n = controller.channelCount
        const start = root._monitorChannelStart
        if (compactSinglePage) {
            // Attitude card already covers CH1–4; monitor shows CH5–16 (12 slots).
            if (n <= 0) {
                return 12
            }
            return Math.max(0, Math.min(n - (start - 1), 12))
        }
        return n > 0 ? Math.min(n, 16) : 8
    }
    /// Compact Radio: 4 columns → 3 rows for CH5–16.
    readonly property bool _monitorFourCol: compactSinglePage && _split
    readonly property int _monitorColumns: _monitorFourCol ? 4 : (_split ? 2 : 1)

    implicitHeight: mainColumn.implicitHeight
    implicitWidth: mainColumn.implicitWidth
    // compactSinglePage: height comes from anchors.fill in RadioComponent wrapper

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    Rectangle {
        anchors.fill: parent
        color: root._pageColor
    }

    ColumnLayout {
        id: mainColumn
        anchors.fill: compactSinglePage ? parent : undefined
        width: parent.width
        height: compactSinglePage ? parent.height : undefined
        spacing: root._rowGap

        // Top: 摇杆预览 | 姿态通道  (~62% when compact)
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: compactSinglePage
            Layout.preferredHeight: compactSinglePage ? parent.height * 0.60 : -1
            Layout.minimumHeight: compactSinglePage ? 0 : -1
            Layout.maximumHeight: compactSinglePage ? parent.height * 0.62 : -1
            columns: root._split ? 2 : 1
            columnSpacing: ScreenTools.defaultFontPixelWidth * (compactSinglePage ? 0.75 : 1.0)
            rowSpacing: root._rowGap

            // —— 摇杆预览 ——
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                Layout.alignment: compactSinglePage ? Qt.AlignVCenter : Qt.AlignTop
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 24
                Layout.minimumHeight: compactSinglePage ? 0 : -1
                implicitHeight: stickInner.implicitHeight + root._cardPad * 2
                radius: root._cardRadius
                color: root._cardColor
                border.width: 1
                border.color: root._cardBorder
                clip: true

                ColumnLayout {
                    id: stickInner
                    anchors.fill: parent
                    anchors.margins: root._cardPad
                    spacing: compactSinglePage ? ScreenTools.defaultFontPixelHeight * 0.2
                                               : ScreenTools.defaultFontPixelHeight * 0.6

                    QGCLabel {
                        text: qsTr("摇杆预览")
                        font.bold: true
                        font.pointSize: ScreenTools.defaultFontPointSize * (compactSinglePage ? 0.95 : 1.05)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCComboBox {
                            id: transmitterModeComboBox
                            Layout.fillWidth: true
                            model: [
                                qsTr("模式1（日本手）"),
                                qsTr("模式2（美国手）"),
                                qsTr("模式3"),
                                qsTr("模式4")
                            ]
                            enabled: !controller.calibrating
                            onActivated: (index) => controller.transmitterMode = index + 1
                            Component.onCompleted: currentIndex = controller.transmitterMode - 1
                            Connections {
                                target: controller
                                function onTransmitterModeChanged() {
                                    transmitterModeComboBox.currentIndex = controller.transmitterMode - 1
                                }
                            }
                        }

                        QGCCheckBox {
                            text: qsTr("油门居中")
                            checked: controller.centeredThrottle
                            enabled: !controller.calibrating
                            visible: !controller.joystickMode
                            onClicked: controller.centeredThrottle = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: compactSinglePage
                        Layout.alignment: Qt.AlignHCenter
                        Layout.minimumHeight: compactSinglePage
                                            ? ScreenTools.defaultFontPixelHeight * 3.2
                                            : -1
                        spacing: ScreenTools.defaultFontPixelWidth * (compactSinglePage ? 0.8 : 1.25)

                        ColumnLayout {
                            spacing: ScreenTools.defaultFontPixelHeight * 0.2
                            Layout.fillWidth: true
                            Layout.fillHeight: compactSinglePage
                            Layout.alignment: Qt.AlignHCenter

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: compactSinglePage
                                Layout.preferredWidth: compactSinglePage ? -1 : root._stickSize
                                Layout.preferredHeight: compactSinglePage ? -1 : root._stickSize
                                Layout.minimumWidth: ScreenTools.defaultFontPixelHeight * 2.8
                                Layout.minimumHeight: ScreenTools.defaultFontPixelHeight * 2.8

                                Rectangle {
                                    readonly property real side: Math.min(parent.width, parent.height)
                                    width: side
                                    height: side
                                    anchors.centerIn: parent
                                    radius: ScreenTools.defaultFontPixelHeight * 0.35
                                    color: qgcPal.windowShade
                                    border.width: 1
                                    border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: parent.width * 0.78
                                        height: width
                                        radius: width / 2
                                        color: "transparent"
                                        border.color: qgcPal.buttonHighlight
                                        border.width: 1

                                        Rectangle {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 1
                                            height: parent.height * 0.7
                                            color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.2)
                                        }
                                        Rectangle {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: parent.width * 0.7
                                            height: 1
                                            color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.2)
                                        }

                                        Rectangle {
                                            property real adjust: (parent.width / 2) - (width / 2) - 2
                                            x: parent.width / 2 + adjust * controller.stickDisplayPositions[0] - width / 2
                                            y: parent.height / 2 + adjust * -controller.stickDisplayPositions[1] - height / 2
                                            width: Math.max(ScreenTools.defaultFontPixelHeight * 0.7, parent.width * 0.14)
                                            height: width
                                            radius: width / 2
                                            color: qgcPal.buttonHighlight
                                        }
                                    }
                                }
                            }

                            QGCLabel {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("左杆")
                                opacity: 0.7
                                font.pointSize: ScreenTools.defaultFontPointSize * 0.9
                            }
                        }

                        ColumnLayout {
                            spacing: ScreenTools.defaultFontPixelHeight * 0.2
                            Layout.fillWidth: true
                            Layout.fillHeight: compactSinglePage
                            Layout.alignment: Qt.AlignHCenter
                            visible: !controller.singleStickDisplay

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: compactSinglePage
                                Layout.preferredWidth: compactSinglePage ? -1 : root._stickSize
                                Layout.preferredHeight: compactSinglePage ? -1 : root._stickSize
                                Layout.minimumWidth: ScreenTools.defaultFontPixelHeight * 2.8
                                Layout.minimumHeight: ScreenTools.defaultFontPixelHeight * 2.8

                                Rectangle {
                                    readonly property real side: Math.min(parent.width, parent.height)
                                    width: side
                                    height: side
                                    anchors.centerIn: parent
                                    radius: ScreenTools.defaultFontPixelHeight * 0.35
                                    color: qgcPal.windowShade
                                    border.width: 1
                                    border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: parent.width * 0.78
                                        height: width
                                        radius: width / 2
                                        color: "transparent"
                                        border.color: qgcPal.buttonHighlight
                                        border.width: 1

                                        Rectangle {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 1
                                            height: parent.height * 0.7
                                            color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.2)
                                        }
                                        Rectangle {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: parent.width * 0.7
                                            height: 1
                                            color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.2)
                                        }

                                        Rectangle {
                                            property real adjust: (parent.width / 2) - (width / 2) - 2
                                            x: parent.width / 2 + adjust * controller.stickDisplayPositions[2] - width / 2
                                            y: parent.height / 2 + adjust * -controller.stickDisplayPositions[3] - height / 2
                                            width: Math.max(ScreenTools.defaultFontPixelHeight * 0.7, parent.width * 0.14)
                                            height: width
                                            radius: width / 2
                                            color: qgcPal.buttonHighlight
                                        }
                                    }
                                }
                            }

                            QGCLabel {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("右杆")
                                opacity: 0.7
                                font.pointSize: ScreenTools.defaultFontPointSize * 0.9
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelWidth * 0.85

                        QGCButton {
                            id: nextButton
                            primary: true
                            Layout.fillWidth: true
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.55 : 2.2)
                            text: qsTr("Calibrate")

                            onClicked: {
                                if (text === qsTr("Calibrate") || text === qsTr("开始校准")) {
                                    if (controller.channelCount < controller.minChannelCount) {
                                        let errorMessage = ""
                                        let title = ""
                                        if (controller.joystickMode) {
                                            title = qsTr("Joystick Not Ready")
                                            errorMessage = qsTr("%1 axes or more are needed to fly. Joystick is reporting %2 axes.").arg(controller.minChannelCount).arg(controller.channelCount)
                                        } else {
                                            title = qsTr("Not Ready")
                                            errorMessage = controller.channelCount === 0
                                                    ? qsTr("Please turn on RC transmitter.")
                                                    : qsTr("%1 channels or more are needed to fly.").arg(controller.minChannelCount)
                                        }
                                        QGroundControl.showMessageDialog(root, title, errorMessage)
                                        return
                                    } else if (!controller.joystickMode) {
                                        const vehicle = QGroundControl.multiVehicleManager.activeVehicle
                                        QGroundControl.showMessageDialog(
                                                    root, qsTr("Zero Trims"),
                                                    qsTr("Before calibrating you should zero all your trims and subtrims. Click Ok to start Calibration.\n\n%1").arg(
                                                        (vehicle && vehicle.px4Firmware)
                                                        ? ""
                                                        : qsTr("Please ensure all motor power is disconnected AND all props are removed from the vehicle.")),
                                                    Dialog.Ok,
                                                    function() { controller.nextButtonClicked() })
                                        return
                                    }
                                }
                                controller.nextButtonClicked()
                            }
                        }

                        QGCButton {
                            id: cancelButton
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 9
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.55 : 2.2)
                            text: qsTr("Cancel")
                            onClicked: controller.cancelButtonClicked()
                        }

                        QGCButton {
                            text: qsTr("One-Sided")
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.55 : 2.2)
                            visible: controller.oneSidedButtonVisible
                            onClicked: controller.oneSidedButtonClicked()
                        }
                    }

                    QGCLabel {
                        id: statusText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        maximumLineCount: compactSinglePage ? 1 : 3
                        elide: Text.ElideRight
                        visible: text.length > 0
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        opacity: 0.55
                        visible: statusText.text.length === 0
                        font.pointSize: ScreenTools.defaultFontPointSize * (compactSinglePage ? 0.85 : 1.0)
                        text: qsTr("移动摇杆以映射通道")
                    }
                }
            }

            // —— 姿态通道 ——
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                Layout.alignment: compactSinglePage ? Qt.AlignVCenter : Qt.AlignTop
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 24
                Layout.minimumHeight: compactSinglePage ? 0 : -1
                implicitHeight: attitudeInner.implicitHeight + root._cardPad * 2
                radius: root._cardRadius
                color: root._cardColor
                border.width: 1
                border.color: root._cardBorder
                clip: true

                ColumnLayout {
                    id: attitudeInner
                    anchors.fill: parent
                    anchors.leftMargin: root._cardPad
                    anchors.rightMargin: root._cardPad
                    anchors.bottomMargin: root._cardPad
                    anchors.topMargin: compactSinglePage
                                       ? ScreenTools.defaultFontPixelHeight * 0.2
                                       : root._cardPad
                    spacing: compactSinglePage ? ScreenTools.defaultFontPixelHeight * 0.35
                                               : ScreenTools.defaultFontPixelHeight * 0.75

                    QGCLabel {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.preferredHeight: implicitHeight
                        Layout.maximumHeight: implicitHeight
                        text: qsTr("姿态通道")
                        font.bold: true
                        font.pointSize: ScreenTools.defaultFontPointSize * (compactSinglePage ? 0.95 : 1.05)
                    }

                    Item { Layout.fillHeight: true; Layout.minimumHeight: 0; visible: !compactSinglePage && root._split }

                    Repeater {
                        model: 4

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: compactSinglePage
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.6 : 2.1)
                            Layout.minimumHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.35 : 1.7)
                            spacing: ScreenTools.defaultFontPixelWidth * 0.55

                            readonly property var axisInfo: [
                                { name: qsTr("俯仰"), mapped: controller.pitchChannelMapped,    value: controller.adjustedPitchChannelValue,    deadband: controller.pitchDeadband },
                                { name: qsTr("横滚"), mapped: controller.rollChannelMapped,     value: controller.adjustedRollChannelValue,     deadband: controller.rollDeadband },
                                { name: qsTr("航向"), mapped: controller.yawChannelMapped,      value: controller.adjustedYawChannelValue,      deadband: controller.yawDeadband },
                                { name: qsTr("油门"), mapped: controller.throttleChannelMapped, value: controller.adjustedThrottleChannelValue, deadband: controller.throttleDeadband }
                            ][index]

                            QGCLabel {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 5
                                Layout.alignment: Qt.AlignVCenter
                                text: axisInfo.name
                                elide: Text.ElideRight
                            }

                            RemoteControlChannelValueDisplay {
                                Layout.fillWidth: true
                                Layout.fillHeight: compactSinglePage
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.35 : 1.6)
                                Layout.maximumHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.8 : 2.2)
                                Layout.alignment: Qt.AlignVCenter
                                mode: RemoteControlChannelValueDisplay.MappedValue
                                channelValueMin: controller.channelValueMin
                                channelValueMax: controller.channelValueMax
                                channelMapped: axisInfo.mapped
                                channelValue: axisInfo.value
                                deadbandValue: axisInfo.deadband
                                deadbandEnabled: root._deadbandActive
                                accentIndicator: true
                                thickTrack: compactSinglePage
                            }

                            QGCLabel {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 4.5
                                Layout.alignment: Qt.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                                text: axisInfo.mapped ? axisInfo.value : "—"
                                font.family: ScreenTools.fixedFontFamily
                                font.pointSize: ScreenTools.defaultFontPointSize * 0.9
                                opacity: axisInfo.mapped ? 1 : 0.45
                            }

                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                radius: height / 2
                                color: axisInfo.mapped
                                       ? Qt.rgba(qgcPal.colorGreen.r, qgcPal.colorGreen.g, qgcPal.colorGreen.b, 0.16)
                                       : Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.08)
                                implicitHeight: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 1.25 : 1.45)
                                implicitWidth: mappedPillLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.2

                                QGCLabel {
                                    id: mappedPillLabel
                                    anchors.centerIn: parent
                                    text: axisInfo.mapped ? qsTr("已映射") : qsTr("未映射")
                                    color: axisInfo.mapped ? qgcPal.colorGreen : qgcPal.text
                                    opacity: axisInfo.mapped ? 1 : 0.55
                                    font.pointSize: ScreenTools.defaultFontPointSize * 0.78
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true; Layout.minimumHeight: 0; visible: !compactSinglePage }

                    // Aux / extra axes only on Joystick (non-compact) — keep Radio one-page
                    QGCLabel {
                        text: qsTr("Aux Extensions")
                        visible: !compactSinglePage && (controller.pitchExtensionEnabled || controller.rollExtensionEnabled)
                        font.bold: true
                    }

                    Repeater {
                        model: [
                            { name: qsTr("Pitch"), extensionEnabled: controller.pitchExtensionEnabled, mapped: controller.pitchExtensionChannelMapped, value: controller.adjustedPitchExtensionChannelValue, deadband: controller.pitchExtensionDeadband },
                            { name: qsTr("Roll"),  extensionEnabled: controller.rollExtensionEnabled,  mapped: controller.rollExtensionChannelMapped,  value: controller.adjustedRollExtensionChannelValue,  deadband: controller.rollExtensionDeadband }
                        ]

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            visible: !compactSinglePage && modelData.extensionEnabled
                            spacing: ScreenTools.defaultFontPixelWidth * 0.6

                            QGCLabel {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 9
                                text: modelData.name
                            }

                            RemoteControlChannelValueDisplay {
                                Layout.fillWidth: true
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.3
                                mode: RemoteControlChannelValueDisplay.MappedValue
                                channelValueMin: controller.channelValueMin
                                channelValueMax: controller.channelValueMax
                                channelMapped: modelData.mapped
                                channelValue: modelData.value
                                deadbandValue: modelData.deadband
                                deadbandEnabled: root._deadbandActive
                            }

                            QGCLabel {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 5
                                horizontalAlignment: Text.AlignRight
                                text: modelData.mapped ? modelData.value : "—"
                                font.family: ScreenTools.fixedFontFamily
                            }
                        }
                    }

                    QGCLabel {
                        text: qsTr("Additional Axes")
                        visible: !compactSinglePage && (controller.additionalAxis1Enabled || controller.additionalAxis2Enabled || controller.additionalAxis3Enabled ||
                                 controller.additionalAxis4Enabled || controller.additionalAxis5Enabled || controller.additionalAxis6Enabled)
                        font.bold: true
                    }

                    Repeater {
                        model: [
                            { name: qsTr("Aux 1"), extensionEnabled: controller.additionalAxis1Enabled, mapped: controller.additionalAxis1ChannelMapped, value: controller.adjustedAdditionalAxis1ChannelValue, deadband: controller.additionalAxis1Deadband },
                            { name: qsTr("Aux 2"), extensionEnabled: controller.additionalAxis2Enabled, mapped: controller.additionalAxis2ChannelMapped, value: controller.adjustedAdditionalAxis2ChannelValue, deadband: controller.additionalAxis2Deadband },
                            { name: qsTr("Aux 3"), extensionEnabled: controller.additionalAxis3Enabled, mapped: controller.additionalAxis3ChannelMapped, value: controller.adjustedAdditionalAxis3ChannelValue, deadband: controller.additionalAxis3Deadband },
                            { name: qsTr("Aux 4"), extensionEnabled: controller.additionalAxis4Enabled, mapped: controller.additionalAxis4ChannelMapped, value: controller.adjustedAdditionalAxis4ChannelValue, deadband: controller.additionalAxis4Deadband },
                            { name: qsTr("Aux 5"), extensionEnabled: controller.additionalAxis5Enabled, mapped: controller.additionalAxis5ChannelMapped, value: controller.adjustedAdditionalAxis5ChannelValue, deadband: controller.additionalAxis5Deadband },
                            { name: qsTr("Aux 6"), extensionEnabled: controller.additionalAxis6Enabled, mapped: controller.additionalAxis6ChannelMapped, value: controller.adjustedAdditionalAxis6ChannelValue, deadband: controller.additionalAxis6Deadband }
                        ]

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            visible: !compactSinglePage && modelData.extensionEnabled
                            spacing: ScreenTools.defaultFontPixelWidth * 0.6

                            QGCLabel {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 9
                                text: modelData.name
                            }

                            RemoteControlChannelValueDisplay {
                                Layout.fillWidth: true
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.3
                                mode: RemoteControlChannelValueDisplay.MappedValue
                                channelValueMin: controller.channelValueMin
                                channelValueMax: controller.channelValueMax
                                channelMapped: modelData.mapped
                                channelValue: modelData.value
                                deadbandValue: modelData.deadband
                                deadbandEnabled: root._deadbandActive
                            }

                            QGCLabel {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 5
                                horizontalAlignment: Text.AlignRight
                                text: modelData.mapped ? modelData.value : "—"
                                font.family: ScreenTools.fixedFontFamily
                            }
                        }
                    }
                }
            }
        }

        // —— 通道监视 ——
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: compactSinglePage
            Layout.preferredHeight: compactSinglePage ? parent.height * 0.36 : -1
            Layout.minimumHeight: compactSinglePage ? 0 : -1
            Layout.maximumHeight: compactSinglePage ? parent.height * 0.40 : -1
            implicitHeight: monitorInner.implicitHeight + root._cardPad * 2
            radius: root._cardRadius
            color: root._cardColor
            border.width: 1
            border.color: root._cardBorder
            clip: true

            ColumnLayout {
                id: monitorInner
                anchors.fill: parent
                anchors.leftMargin: root._cardPad
                anchors.rightMargin: root._cardPad
                anchors.bottomMargin: root._cardPad
                // Keep title near the top of the card (less empty band above 通道监视)
                anchors.topMargin: compactSinglePage
                                   ? ScreenTools.defaultFontPixelHeight * 0.2
                                   : root._cardPad
                spacing: ScreenTools.defaultFontPixelHeight * (compactSinglePage ? 0.08 : 0.45)

                RemoteControlChannelMonitor {
                    id: channelMonitor
                    Layout.fillWidth: true
                    Layout.fillHeight: compactSinglePage
                    twoColumn: root._split && !root._monitorFourCol
                    columnCount: root._monitorColumns
                    compact: compactSinglePage
                    title: qsTr("通道监视")
                    channelStart: root._monitorChannelStart
                    channelCount: root._monitorChannels
                    channelValueMin: controller.channelValueMin
                    channelValueMax: controller.channelValueMax

                    Connections {
                        target: controller
                        function onRawChannelValueChanged(channel, channelValue) {
                            channelMonitor.rawChannelValueChanged(channel, channelValue)
                        }
                    }
                }

                Loader {
                    Layout.fillWidth: true
                    visible: !compactSinglePage
                    sourceComponent: additionalMonitorComponent
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            visible: !compactSinglePage && status === Loader.Ready && item
            sourceComponent: additionalSetupComponent
        }
    }
}
