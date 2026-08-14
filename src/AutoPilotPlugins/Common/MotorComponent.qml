import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id: motorPage
    pageComponent: pageComponent

    // Kept for APMMotorComponent API compatibility (letters always used in diagram)
    property bool userLetterMotorIndices: true

    Component {
        id: pageComponent

        // Target remotes like Skydroid G20: 7" 1920×1200 landscape (~960×600 dp).
        // After Vehicle Setup sidebar, content is ~750×520 — height is the bottleneck,
        // so prefer diagram|controls split and compact controls (not a tall ValueSlider).
        Item {
            id: pageRoot
            width: availableWidth
            height: pageRoot.splitLayout
                    ? availableHeight
                    : Math.max(availableHeight, mainLayout.implicitHeight + ScreenTools.defaultFontPixelHeight)

            MotorComponentController {
                id: controller
            }

            property bool safetyOn: false
            property string selectedLetter: ""
            property int selectedMotorIndex: -1
            property var spinningLetters: ({})
            property var spinDeadlines: ({})   ///< letter -> epoch ms
            property string statusText: qsTr("请开启安全开关，再选择电机测试。")

            // G20 / Herelink-class: short landscape → split as soon as width fits both columns.
            readonly property bool shortLandscape: ScreenTools.isShortScreen
                    || availableHeight < ScreenTools.defaultFontPixelHeight * 30
            property bool splitLayout: width >= ScreenTools.defaultFontPixelWidth * 50
                    && (shortLandscape || width >= ScreenTools.defaultFontPixelWidth * 85)
            readonly property real _diagramHeight: {
                if (pageRoot.splitLayout) {
                    return Math.max(ScreenTools.defaultFontPixelHeight * 10,
                                    availableHeight - ScreenTools.defaultFontPixelHeight)
                }
                return Math.min(availableHeight * 0.32, ScreenTools.defaultFontPixelHeight * 11)
            }

            function _topologyLabel() {
                const name = controller.topologyName
                if (name === "DodecaHexa X Coaxial") {
                    return qsTr("共轴六轴 X")
                }
                if (name === "DodecaHexa + Coaxial") {
                    return qsTr("共轴六轴 +")
                }
                if (name === "OctoQuad X Coaxial") {
                    return qsTr("共轴八轴 X")
                }
                if (name === "Y6B Coaxial") {
                    return qsTr("共轴 Y6B")
                }
                if (name === "Quad X") {
                    return qsTr("四轴 X")
                }
                if (name === "Quad +") {
                    return qsTr("四轴 +")
                }
                if (name === "Hexa X") {
                    return qsTr("六轴 X")
                }
                if (name === "Octo X") {
                    return qsTr("八轴 X")
                }
                if (name === "Unknown") {
                    return qsTr("未知")
                }
                return name.length ? qsTr(name) : qsTr("—")
            }

            function _metaLine() {
                const name = controller.vehicleName.length ? controller.vehicleName : qsTr("—")
                const count = controller.motorCount < 0 ? qsTr("?") : String(controller.motors.length)
                return name + " · " + count + " " + qsTr("电机") + " · " + _topologyLabel()
            }

            function _vehicle() {
                return controller.vehicle
            }

            function _throttle() {
                return controller.clampThrottle(Math.round(sliderThrottle.value))
            }

            function _setThrottle(pct) {
                sliderThrottle.value = controller.clampThrottle(pct)
            }

            function _setSpinning(letter, on, durationMs) {
                const nextSpin = Object.assign({}, pageRoot.spinningLetters)
                const nextDead = Object.assign({}, pageRoot.spinDeadlines)
                if (on) {
                    nextSpin[letter] = true
                    nextDead[letter] = Date.now() + (durationMs || controller.motorTimeoutSecs * 1000)
                } else {
                    delete nextSpin[letter]
                    delete nextDead[letter]
                }
                pageRoot.spinningLetters = nextSpin
                pageRoot.spinDeadlines = nextDead
            }

            function _clearSpinning() {
                pageRoot.spinningLetters = ({})
                pageRoot.spinDeadlines = ({})
            }

            function _stopAllMotors() {
                const vehicle = _vehicle()
                if (!vehicle) {
                    return
                }
                const motors = controller.motors
                for (let i = 0; i < motors.length; ++i) {
                    vehicle.motorTest(motors[i].motorIndex, 0, 0, true)
                }
                _clearSpinning()
            }

            function _testMotor(letter, motorIndex) {
                const vehicle = _vehicle()
                if (!vehicle || !pageRoot.safetyOn) {
                    return
                }
                const pct = _throttle()
                if (pct === 0) {
                    pageRoot.statusText = qsTr("油门为 0%，请先提高油门再测试。")
                    return
                }
                vehicle.motorTest(motorIndex, pct, controller.motorTimeoutSecs, true)
                _setSpinning(letter, true)
                pageRoot.statusText = qsTr("正在测试电机 %1，油门 %2%，%3 秒后自动停止。")
                    .arg(letter).arg(pct).arg(controller.motorTimeoutSecs)
            }

            function _testSelected() {
                if (!pageRoot.safetyOn) {
                    pageRoot.statusText = qsTr("请先开启安全开关再测试。")
                    return
                }
                if (pageRoot.selectedLetter.length === 0 || pageRoot.selectedMotorIndex < 1) {
                    pageRoot.statusText = qsTr("请先在示意图上选择电机。")
                    return
                }
                _testMotor(pageRoot.selectedLetter, pageRoot.selectedMotorIndex)
            }

            function _testAll() {
                const vehicle = _vehicle()
                if (!vehicle || !pageRoot.safetyOn) {
                    return
                }
                const pct = _throttle()
                if (pct === 0) {
                    pageRoot.statusText = qsTr("油门为 0%，请先提高油门再测试。")
                    return
                }
                const motors = controller.motors
                for (let i = 0; i < motors.length; ++i) {
                    vehicle.motorTest(motors[i].motorIndex, pct, controller.motorTimeoutSecs, true)
                    _setSpinning(motors[i].letter, true)
                }
                pageRoot.statusText = qsTr("正在测试全部 %1 个电机，油门 %2%。")
                    .arg(motors.length).arg(pct)
            }

            Timer {
                interval: 200
                running: Object.keys(pageRoot.spinDeadlines).length > 0
                repeat: true
                onTriggered: {
                    const now = Date.now()
                    const nextSpin = Object.assign({}, pageRoot.spinningLetters)
                    const nextDead = Object.assign({}, pageRoot.spinDeadlines)
                    let changed = false
                    let lastCleared = ""
                    for (const letter in nextDead) {
                        if (nextDead[letter] <= now) {
                            delete nextSpin[letter]
                            delete nextDead[letter]
                            lastCleared = letter
                            changed = true
                        }
                    }
                    if (changed) {
                        pageRoot.spinningLetters = nextSpin
                        pageRoot.spinDeadlines = nextDead
                        if (Object.keys(nextDead).length === 0) {
                            pageRoot.statusText = lastCleared.length
                                ? qsTr("电机 %1 已自动停止。").arg(lastCleared)
                                : qsTr("电机测试结束。")
                        }
                    }
                }
            }

            GridLayout {
                id: mainLayout
                width: parent.width
                height: pageRoot.splitLayout ? parent.height : implicitHeight
                columns: pageRoot.splitLayout ? 2 : 1
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                columnSpacing: ScreenTools.defaultFontPixelWidth * 1.5

                MotorTestDiagram {
                    id: diagram
                    Layout.fillWidth: true
                    Layout.fillHeight: pageRoot.splitLayout
                    Layout.preferredWidth: pageRoot.splitLayout ? parent.width * 0.52 : parent.width
                    Layout.preferredHeight: pageRoot._diagramHeight
                    Layout.minimumWidth: pageRoot.splitLayout ? ScreenTools.defaultFontPixelWidth * 28 : 0
                    Layout.minimumHeight: ScreenTools.defaultFontPixelHeight * 9
                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                    motors: controller.motors
                    spatialLayout: controller.spatialLayout
                    safetyArmed: pageRoot.safetyOn
                    selectedLetter: pageRoot.selectedLetter
                    spinningLetters: pageRoot.spinningLetters
                    throttlePercent: pageRoot._throttle()
                    onMotorClicked: (letter, motorIndex) => {
                        pageRoot.selectedLetter = letter
                        pageRoot.selectedMotorIndex = motorIndex
                        if (pageRoot.safetyOn) {
                            pageRoot.statusText = qsTr("已选择电机 %1 — 点击「测试此电机」。").arg(letter)
                        } else {
                            pageRoot.statusText = qsTr("已选择电机 %1 — 请开启安全开关后再测试。").arg(letter)
                        }
                    }
                }

                // Right / below: scrollable so G20 short height never clips controls off-screen.
                Flickable {
                    id: controlsFlick
                    Layout.fillWidth: true
                    Layout.fillHeight: pageRoot.splitLayout
                    Layout.preferredWidth: pageRoot.splitLayout ? parent.width * 0.45 : parent.width
                    Layout.minimumWidth: pageRoot.splitLayout ? ScreenTools.defaultFontPixelWidth * 28 : 0
                    Layout.preferredHeight: pageRoot.splitLayout
                                            ? parent.height
                                            : controlsColumn.implicitHeight
                    contentWidth: width
                    contentHeight: controlsColumn.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick

                    ColumnLayout {
                        id: controlsColumn
                        width: controlsFlick.width
                        spacing: pageRoot.shortLandscape
                                 ? ScreenTools.defaultFontPixelHeight * 0.4
                                 : ScreenTools.defaultFontPixelHeight * 0.6

                        // Compact meta for short remotes
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: metaCol.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.7
                            radius: ScreenTools.defaultFontPixelWidth * 0.6
                            color: qgcPal.windowShade
                            border.color: qgcPal.groupBorder

                            ColumnLayout {
                                id: metaCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.8
                                spacing: 2

                                QGCLabel {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    font.bold: true
                                    text: pageRoot._metaLine()
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            visible: !controller.spatialLayout
                            wrapMode: Text.WordWrap
                            color: qgcPal.warningText
                            text: qsTr("警告：无法识别电机布局，显示编号备用视图（最多 16）。")
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: safetyRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.7
                            radius: ScreenTools.defaultFontPixelWidth * 0.6
                            color: pageRoot.safetyOn ? "#fffbeb" : "#fef2f2"
                            border.color: pageRoot.safetyOn ? "#fbbf24" : "#fecaca"

                            RowLayout {
                                id: safetyRow
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.8
                                spacing: ScreenTools.defaultFontPixelWidth

                                Switch {
                                    id: safetySwitch
                                    onClicked: {
                                        pageRoot.safetyOn = checked
                                        if (!checked) {
                                            pageRoot._stopAllMotors()
                                            pageRoot._setThrottle(0)
                                            pageRoot.statusText = qsTr("安全关闭 — 电机已停止，油门归零。")
                                        } else {
                                            pageRoot._setThrottle(controller.defaultThrottle)
                                            if (pageRoot.selectedLetter.length) {
                                                pageRoot.statusText = qsTr("安全已开 — 可点击「测试此电机」测试 %1。").arg(pageRoot.selectedLetter)
                                            } else {
                                                pageRoot.statusText = qsTr("安全已开 — 默认油门 %1%（上限 %2%）。请选择电机。")
                                                    .arg(controller.defaultThrottle).arg(controller.maxThrottle)
                                            }
                                        }
                                    }
                                }

                                QGCLabel {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    color: pageRoot.safetyOn ? "#b45309" : qgcPal.warningText
                                    font.bold: true
                                    text: pageRoot.safetyOn
                                          ? qsTr("注意：电机已使能")
                                          : qsTr("螺旋桨已拆除 — 开启后可测试")
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            enabled: pageRoot.safetyOn
                            opacity: enabled ? 1 : 0.4
                            spacing: ScreenTools.defaultFontPixelHeight * 0.25

                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    text: qsTr("油门")
                                    font.bold: true
                                }
                                QGCLabel {
                                    text: qsTr("上限 %1%").arg(controller.maxThrottle)
                                    color: "#b45309"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                }
                                Item { Layout.fillWidth: true }
                                QGCLabel {
                                    text: qsTr("%1%").arg(pageRoot._throttle())
                                    color: qgcPal.primaryButton
                                    font.bold: true
                                }
                            }

                            // Compact slider for G20-class short screens (ValueSlider is too tall).
                            QGCSlider {
                                id: sliderThrottle
                                Layout.fillWidth: true
                                from: 0
                                to: controller.maxThrottle
                                stepSize: 1
                                value: 0
                                displayValue: false
                                showBoundaryValues: !pageRoot.shortLandscape
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            enabled: pageRoot.safetyOn
                            opacity: enabled ? 1 : 0.45
                            implicitHeight: debugCol.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.7
                            radius: ScreenTools.defaultFontPixelWidth * 0.6
                            color: pageRoot.selectedLetter.length ? "#eff6ff" : qgcPal.windowShade
                            border.color: pageRoot.selectedLetter.length ? "#93c5fd" : qgcPal.groupBorder

                            ColumnLayout {
                                id: debugCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.8
                                spacing: ScreenTools.defaultFontPixelHeight * 0.3

                                QGCLabel {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    text: pageRoot.selectedLetter.length
                                          ? qsTr("单电机调试 · %1 @ %2%").arg(pageRoot.selectedLetter).arg(pageRoot._throttle())
                                          : qsTr("单电机调试 — 点选示意图")
                                    font.bold: true
                                    color: pageRoot.selectedLetter.length ? "#1d4ed8" : qgcPal.text
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    QGCButton {
                                        Layout.fillWidth: true
                                        primary: true
                                        enabled: pageRoot.safetyOn && pageRoot.selectedLetter.length > 0
                                        text: qsTr("测试此电机")
                                        onClicked: pageRoot._testSelected()
                                    }
                                    QGCButton {
                                        text: qsTr("停止")
                                        onClicked: {
                                            pageRoot._stopAllMotors()
                                            pageRoot.statusText = qsTr("已停止。")
                                        }
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            enabled: pageRoot.safetyOn
                            opacity: enabled ? 1 : 0.4
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCButton {
                                Layout.fillWidth: true
                                text: qsTr("全部")
                                onClicked: pageRoot._testAll()
                            }
                            QGCButton {
                                Layout.fillWidth: true
                                text: qsTr("全部停止")
                                onClicked: {
                                    pageRoot._stopAllMotors()
                                    pageRoot.statusText = qsTr("全部电机已停止。")
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            font.pointSize: ScreenTools.smallFontPointSize
                            text: pageRoot.statusText
                        }
                    }
                }
            }
        }
    }
}
