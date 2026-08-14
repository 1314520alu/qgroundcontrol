import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:             _summaryRoot
    anchors.fill:   parent
    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
    anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
    color:          qgcPal.window

    property real _gap:         ScreenTools.defaultFontPixelWidth * 1.25
    property real _margins:     ScreenTools.defaultFontPixelHeight * 0.65
    property real _cardRadius:  ScreenTools.defaultFontPixelHeight * 0.35
    property real _cardHeight:  ScreenTools.defaultFontPixelHeight * 12.5
    property int  _gridColumns: {
        if (_summaryRoot.width >= ScreenTools.defaultFontPixelWidth * 52) {
            return 3
        }
        if (_summaryRoot.width >= ScreenTools.defaultFontPixelWidth * 30) {
            return 2
        }
        return 1
    }

    readonly property var _summaryOrder: [
        "AirframeComponentSummary",
        "SubFrameComponentSummary",
        "SensorsComponentSummary",
        "RadioComponentSummary",
        "JoystickComponentSummary",
        "PowerComponentSummary",
        "FlightModesComponentSummary",
        "FlightSafetyComponentSummary",
        "SafetyComponentSummary",
        "FailsafesComponentSummary",
        "ESCComponentSummary",
        "ESCTelemetryComponentSummary"
    ]

    property var _summaryComponents: []

    function capitalizeWords(sentence) {
        return sentence.replace(/(?:^|\s)\S/g, function(a) { return a.toUpperCase(); })
    }

    function _summarySortKey(comp) {
        if (!comp) {
            return 1000
        }
        const url = comp.summaryQmlSource.toString()
        for (let i = 0; i < _summaryOrder.length; i++) {
            if (url.indexOf(_summaryOrder[i]) >= 0) {
                return i
            }
        }
        return 100 + url.length
    }

    function _rebuildSummaryComponents() {
        const vehicle = QGroundControl.multiVehicleManager.activeVehicle
        if (!vehicle || !vehicle.autopilotPlugin) {
            _summaryComponents = []
            return
        }
        const raw = vehicle.autopilotPlugin.vehicleComponents
        const list = []
        for (let i = 0; i < raw.length; i++) {
            const comp = raw[i]
            if (comp && comp.summaryQmlSource.toString() !== "") {
                list.push(comp)
            }
        }
        list.sort(function(a, b) {
            const ka = _summarySortKey(a)
            const kb = _summarySortKey(b)
            if (ka !== kb) {
                return ka - kb
            }
            return a.name.localeCompare(b.name)
        })
        _summaryComponents = list
    }

    function _statusText(comp) {
        if (!comp) {
            return ""
        }
        const url = comp.summaryQmlSource.toString()
        if (url.indexOf("JoystickComponentSummary") >= 0) {
            const joy = typeof joystickManager !== "undefined" ? joystickManager.activeJoystick : null
            return joy ? qsTr("Ready") : qsTr("Not connected")
        }
        if (url.indexOf("RadioComponentSummary") >= 0) {
            return qsTr("Mapped")
        }
        if (comp.requiresSetup) {
            return comp.setupComplete ? qsTr("Ready") : qsTr("Needs setup")
        }
        return qsTr("Ready")
    }

    function _statusOk(comp) {
        if (!comp) {
            return true
        }
        const url = comp.summaryQmlSource.toString()
        if (url.indexOf("JoystickComponentSummary") >= 0) {
            const joy = typeof joystickManager !== "undefined" ? joystickManager.activeJoystick : null
            return !!joy
        }
        if (comp.requiresSetup) {
            return comp.setupComplete
        }
        return true
    }

    function _statusWarn(comp) {
        if (!comp) {
            return false
        }
        return comp.summaryQmlSource.toString().indexOf("JoystickComponentSummary") >= 0 && !_statusOk(comp)
    }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        function onActiveVehicleChanged() { _rebuildSummaryComponents() }
    }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.autopilotPlugin : null
        function onVehicleComponentsChanged() { _rebuildSummaryComponents() }
    }

    Connections {
        target: typeof joystickManager !== "undefined" ? joystickManager : null
        function onActiveJoystickChanged() { _rebuildSummaryComponents() }
    }

    Component.onCompleted: _rebuildSummaryComponents()

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      summaryColumn.height
        contentWidth:       _summaryRoot.width
        flickableDirection: Flickable.VerticalFlick

        ColumnLayout {
            id:             summaryColumn
            width:          _summaryRoot.width
            spacing:        ScreenTools.defaultFontPixelHeight * 0.75

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: setupComplete ? qgcPal.text : qgcPal.warningText
                font.bold: true
                font.pointSize: ScreenTools.defaultFontPointSize * 1.1
                horizontalAlignment: Text.AlignLeft
                text: setupComplete ? qsTr("Configuration summary") : qsTr("WARNING: Finish red-marked setup items before flight.")

                property bool setupComplete: QGroundControl.multiVehicleManager.activeVehicle ?
                                                 QGroundControl.multiVehicleManager.activeVehicle.autopilotPlugin.setupComplete : false
            }

            GridLayout {
                id:             _gridCtl
                Layout.fillWidth: true
                columns:        _summaryRoot._gridColumns
                columnSpacing:  _gap
                rowSpacing:     _gap

                Repeater {
                    model: _summaryRoot._summaryComponents

                    Rectangle {
                        id: card
                        Layout.fillWidth: true
                        Layout.preferredHeight: _cardHeight
                        Layout.minimumHeight: _cardHeight
                        Layout.maximumHeight: _cardHeight
                        radius: _cardRadius
                        clip: true
                        // Match Vehicle Config panels: windowShade + groupBorder
                        color: qgcPal.windowShade
                        border.width: 1
                        border.color: qgcPal.groupBorder

                        readonly property int cardIndex: index + 1
                        readonly property bool setupOk: _summaryRoot._statusOk(modelData)
                        readonly property bool setupWarn: _summaryRoot._statusWarn(modelData)
                        readonly property string pillText: _summaryRoot._statusText(modelData)

                        ColumnLayout {
                            id: mainLayout
                            anchors.fill: parent
                            anchors.margins: _margins
                            spacing: ScreenTools.defaultFontPixelHeight * 0.35

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: ScreenTools.defaultFontPixelWidth * 0.55

                                Rectangle {
                                    width: ScreenTools.defaultFontPixelHeight * 1.25
                                    height: width
                                    radius: width / 2
                                    color: qgcPal.button
                                    border.width: 1
                                    border.color: qgcPal.buttonBorder
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: card.cardIndex
                                        font.bold: true
                                        color: qgcPal.buttonText
                                        font.pointSize: ScreenTools.defaultFontPointSize * 0.85
                                    }
                                }

                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: capitalizeWords(modelData.name)
                                    font.bold: true
                                    font.pointSize: ScreenTools.defaultFontPointSize * 1.02
                                    elide: Text.ElideRight
                                }

                                SummaryStatusPill {
                                    text: card.pillText
                                    ok: card.setupOk
                                    warn: card.setupWarn
                                }
                            }

                            // Match SettingsGroupLayout dividers (groupBorder, not windowShadeDark)
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: qgcPal.groupBorder
                            }

                            Item {
                                id: contentClip
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                // Shrink card body when content exceeds available height (min 62%).
                                // Keep loader width fixed — widening while scaling breaks wrap math.
                                property real contentScale: 1.0

                                function refit() {
                                    if (!summaryLoader.item) {
                                        contentScale = 1.0
                                        return
                                    }
                                    const needed = summaryLoader.item.implicitHeight
                                    const avail = contentClip.height
                                    if (needed <= 0 || avail <= 0) {
                                        contentScale = 1.0
                                        return
                                    }
                                    contentScale = Math.min(1.0, Math.max(0.62, avail / needed))
                                }

                                onHeightChanged: Qt.callLater(refit)
                                onWidthChanged: Qt.callLater(refit)

                                Loader {
                                    id: summaryLoader
                                    width: contentClip.width
                                    transformOrigin: Item.TopLeft
                                    scale: contentClip.contentScale
                                    source: modelData.summaryQmlSource
                                    property var vehicleComponent: modelData

                                    onStatusChanged: {
                                        if (status === Loader.Ready) {
                                            Qt.callLater(contentClip.refit)
                                        }
                                    }
                                    onItemChanged: {
                                        if (item) {
                                            item.implicitHeightChanged.connect(contentClip.refit)
                                            Qt.callLater(contentClip.refit)
                                        }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData.setupSource.toString() !== "") {
                                    setupView.showVehicleComponentPanel(modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
