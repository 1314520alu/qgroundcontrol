import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id:             control
    objectName:     "toolbar_batteryIndicator"
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _appSettings:       QGroundControl.settingsManager.appSettings
    property bool   _xf200Tethered:     _appSettings.aircraftModel.rawValue === Xf200TetheredPowerVisual.aircraftModelZyXf200Tethered
    property bool       showIndicator:      _activeVehicle && (_xf200Tethered || _activeVehicle.batteries.count > 0)
    property bool       waitForParameters:  false
    property Component  expandedPageComponent
    property int        _xf200DrawerSlotId: 1

    property var    _batterySettings:   QGroundControl.settingsManager.batteryIndicatorSettings
    property Fact   _indicatorDisplay:  _batterySettings.valueDisplay
    property bool   _showPercentage:    _indicatorDisplay.rawValue === 0
    property bool   _showVoltage:       _indicatorDisplay.rawValue === 1
    property bool   _showBoth:          _indicatorDisplay.rawValue === 2
    property int    _lowestBatteryId:   -1      // -1: show all batteries, otherwise show only battery with this id

    // Properties to hold the thresholds
    property int threshold1: _batterySettings.threshold1.rawValue
    property int threshold2: _batterySettings.threshold2.rawValue

    width: _xf200Tethered ? xf200IndicatorRow.width : batteryIndicatorRow.width

    function _xf200Color(kind) {
        switch (kind) {
        case Xf200TetheredPowerVisual.Normal:
            return qgcPal.colorGreen
        case Xf200TetheredPowerVisual.Warn:
            return qgcPal.colorYellow
        case Xf200TetheredPowerVisual.Low:
            return qgcPal.colorOrange
        case Xf200TetheredPowerVisual.Critical:
        case Xf200TetheredPowerVisual.Emergency:
            return qgcPal.colorRed
        default:
            return qgcPal.text
        }
    }

    function _xf200PowerText(watts) {
        if (isNaN(watts)) {
            return qsTr("—")
        }
        if (Math.abs(watts) < 1000) {
            return Number(watts).toFixed(1) + " W"
        }
        return Number(watts / 1000).toFixed(1) + " kW"
    }

    function _xf200OpenSlot(slotId) {
        _xf200DrawerSlotId = slotId
        mainWindow.showIndicatorDrawer(xf200Popup, control)
    }

    function _recalcLowestBatteryIdFromVoltage() {
        if (_activeVehicle) {
            // If there is only one battery then it is the lowest
            if (_activeVehicle.batteries.count === 1) {
                _lowestBatteryId = _activeVehicle.batteries.get(0).id.rawValue
                return
            }

            // If we have valid voltage for all batteries we use that to determine lowest battery
            let allHaveVoltage = true
            for (var i = 0; i < _activeVehicle.batteries.count; i++) {
                let battery = _activeVehicle.batteries.get(i)
                if (isNaN(battery.voltage.rawValue)) {
                    allHaveVoltage = false
                    break
                }
            }
            if (allHaveVoltage) {
                let lowestBattery = _activeVehicle.batteries.get(0)
                let lowestBatteryId = lowestBattery.id.rawValue
                for (var i = 1; i < _activeVehicle.batteries.count; i++) {
                    let battery = _activeVehicle.batteries.get(i)
                    if (battery.voltage.rawValue < lowestBattery.voltage.rawValue) {
                        lowestBattery = battery
                        lowestBatteryId = battery.id.rawValue
                    }
                }
                _lowestBatteryId = lowestBatteryId
                return
            }
        }

        // Couldn't determine lowest battery, show all
        _lowestBatteryId = -1
    }

    function _recalcLowestBatteryIdFromPercentage() {
        if (_activeVehicle) {
            // If there is only one battery then it is the lowest
            if (_activeVehicle.batteries.count === 1) {
                _lowestBatteryId = _activeVehicle.batteries.get(0).id.rawValue
                return
            }

            // If we have valid percentage for all batteries we use that to determine lowest battery
            let allHavePercentage = true
            for (var i = 0; i < _activeVehicle.batteries.count; i++) {
                let battery = _activeVehicle.batteries.get(i)
                if (isNaN(battery.percentRemaining.rawValue)) {
                    allHavePercentage = false
                    break
                }
            }
            if (allHavePercentage) {
                let lowestBattery = _activeVehicle.batteries.get(0)
                let lowestBatteryId = lowestBattery.id.rawValue
                for (var i = 1; i < _activeVehicle.batteries.count; i++) {
                    let battery = _activeVehicle.batteries.get(i)
                    if (battery.percentRemaining.rawValue < lowestBattery.percentRemaining.rawValue) {
                        lowestBattery = battery
                        lowestBatteryId = battery.id.rawValue
                    }
                }
                _lowestBatteryId = lowestBatteryId
                return
            }
        }

        // Couldn't determine lowest battery, show all
        _lowestBatteryId = -1
    }

    function _recalcLowestBatteryIdFromChargeState() {
        if (_activeVehicle) {
            // If there is only one battery then it is the lowest
            if (_activeVehicle.batteries.count === 1) {
                _lowestBatteryId = _activeVehicle.batteries.get(0).id.rawValue
                return
            }

            // If we have valid chargeState for all batteries we use that to determine lowest battery
            let allHaveChargeState = true
            for (var i = 0; i < _activeVehicle.batteries.count; i++) {
                let battery = _activeVehicle.batteries.get(i)
                if (battery.chargeState.rawValue === MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    allHaveChargeState = false
                    break
                }
            }
            if (allHaveChargeState) {
                let lowestBattery = _activeVehicle.batteries.get(0)
                let lowestBatteryId = lowestBattery.id.rawValue
                for (var i = 1; i < _activeVehicle.batteries.count; i++) {
                    let battery = _activeVehicle.batteries.get(i)
                    if (battery.chargeState.rawValue > lowestBattery.chargeState.rawValue) {
                        lowestBattery = battery
                        lowestBatteryId = battery.id.rawValue
                    }
                }
                _lowestBatteryId = lowestBatteryId
                return
            }
        }

        // Couldn't determine lowest battery, show all
        _lowestBatteryId = -1
    }

    function _recalcLowestBatteryId() {
        if (!_activeVehicle || _activeVehicle.batteries.count === 0) {
            _lowestBatteryId = -1
            return
        }
        if (_batterySettings.valueDisplay.rawValue === 0) {
            // User wants percentage display so use that if available
            _recalcLowestBatteryIdFromPercentage()
        } else if (_batterySettings.valueDisplay.rawValue === 1) {
            // User wants voltage display so use that if available
            _recalcLowestBatteryIdFromVoltage()
        }
        // If we still dont have a lowest battery id then try charge state
        if (_lowestBatteryId === -1) {
            _recalcLowestBatteryIdFromChargeState()
        }
    }

    Component.onCompleted: _recalcLowestBatteryId()

    Connections {
        target: _activeVehicle ? _activeVehicle.batteries : null
        function onCountChanged() {_recalcLowestBatteryId() }
    }

    QGCPalette { id: qgcPal }

    RowLayout {
        id:             batteryIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2
        visible:        !_xf200Tethered

        Repeater {
            model: _activeVehicle ? _activeVehicle.batteries : 0

            Loader {
                Layout.fillHeight:  true
                sourceComponent:    batteryVisual
                visible:            control._lowestBatteryId === -1 || object.id.rawValue === control._lowestBatteryId || !control._batterySettings.consolidateMultipleBatteries.rawValue

                property var battery: object
            }
        }
    }

    RowLayout {
        id:                 xf200IndicatorRow
        objectName:         "toolbar_xf200PowerIndicator"
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelWidth / 3
        visible:            _xf200Tethered

        Repeater {
            model: [1, 2, 3, 4, 5]

            Item {
                id:                 slotChip
                Layout.fillHeight:  true
                implicitWidth:      slotRow.implicitWidth
                implicitHeight:     parent.height

                required property int modelData

                property int mavlinkId: Xf200TetheredPowerVisual.mavlinkBatteryId(modelData)
                property var battery: {
                    if (!control._activeVehicle) {
                        return null
                    }
                    var n = control._activeVehicle.batteries.count
                    for (var i = 0; i < n; i++) {
                        var candidate = control._activeVehicle.batteries.get(i)
                        if (candidate.id.rawValue === mavlinkId) {
                            return candidate
                        }
                    }
                    return null
                }
                property double voltage:     battery ? battery.voltage.rawValue : NaN
                property double powerWatts:  battery ? battery.instantPower.rawValue : NaN
                property int    chargeState: battery ? battery.chargeState.rawValue : MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED
                property int    kind:        Xf200TetheredPowerVisual.kind(voltage, chargeState)
                property bool   isPsu:       modelData <= 4

                RowLayout {
                    id:              slotRow
                    anchors.top:     parent.top
                    anchors.bottom:  parent.bottom
                    spacing:         ScreenTools.defaultFontPixelWidth / 6

                    Item {
                        Layout.alignment:       Qt.AlignVCenter
                        Layout.fillHeight:      true
                        Layout.preferredWidth:  height
                        Layout.minimumWidth:    height

                        QGCColoredImage {
                            anchors.fill:           parent
                            sourceSize.height:      height
                            fillMode:               Image.PreserveAspectFit
                            source:                 slotChip.isPsu
                                                    ? Xf200TetheredPowerVisual.psuSvg(slotChip.kind)
                                                    : Xf200TetheredPowerVisual.batterySvg(slotChip.kind)
                            color:                  control._xf200Color(slotChip.kind)
                        }

                        QGCLabel {
                            anchors.centerIn:   parent
                            visible:            slotChip.isPsu
                            text:               slotChip.modelData
                            font.bold:          true
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              control._xf200Color(slotChip.kind)
                        }
                    }

                    ColumnLayout {
                        Layout.alignment:   Qt.AlignVCenter
                        spacing:            0

                        QGCLabel {
                            Layout.alignment:   Qt.AlignHCenter
                            text:               isNaN(slotChip.voltage)
                                                ? qsTr("—")
                                                : Number(slotChip.voltage).toFixed(1) + " V"
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              qgcPal.text
                        }

                        QGCLabel {
                            Layout.alignment:   Qt.AlignHCenter
                            text:               control._xf200PowerText(slotChip.powerWatts)
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              qgcPal.text
                            visible:            slotChip.isPsu
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked:    control._xf200OpenSlot(slotChip.modelData)
                }
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        visible:        !_xf200Tethered
        onClicked:      mainWindow.showIndicatorDrawer(batteryPopup, control)
    }

    Component {
        id: batteryPopup

        ToolIndicatorPage {
            showExpand:         expandedComponent ? true : false
            waitForParameters:                  false
            expandedComponentWaitForParameters: true
            contentComponent:   batteryContentComponent
            expandedComponent:  batteryExpandedComponent
        }
    }

    Component {
        id: xf200Popup

        ToolIndicatorPage {
            showExpand:                         expandedComponent ? true : false
            waitForParameters:                  false
            expandedComponentWaitForParameters: true
            contentComponent:                   xf200SlotContentComponent
            expandedComponent:                  batteryExpandedComponent
        }
    }

    Component {
        id: xf200SlotContentComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 4

            property int slotId: control._xf200DrawerSlotId
            property int mavlinkId: Xf200TetheredPowerVisual.mavlinkBatteryId(slotId)
            property var battery: {
                if (!control._activeVehicle) {
                    return null
                }
                var n = control._activeVehicle.batteries.count
                for (var i = 0; i < n; i++) {
                    var candidate = control._activeVehicle.batteries.get(i)
                    if (candidate.id.rawValue === mavlinkId) {
                        return candidate
                    }
                }
                return null
            }
            property int chargeState: battery ? battery.chargeState.rawValue : MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED
            property double voltage: battery ? battery.voltage.rawValue : NaN
            property double currentA: battery ? battery.current.rawValue : NaN
            property double powerWatts: battery ? battery.instantPower.rawValue : NaN
            property double temperatureC: battery ? battery.temperature.rawValue : NaN

            QGCLabel {
                font.bold: true
                text: slotId <= 4 ? qsTr("Power %1").arg(slotId) : qsTr("Bus")
            }

            GridLayout {
                columns: 2
                columnSpacing: ScreenTools.defaultFontPixelWidth
                rowSpacing: ScreenTools.defaultFontPixelHeight / 6

                QGCLabel { text: qsTr("State") }
                QGCLabel {
                    text: chargeState === MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED
                          ? qsTr("—")
                          : battery.chargeState.enumStringValue
                }
                QGCLabel { text: qsTr("Voltage") }
                QGCLabel {
                    text: isNaN(voltage) ? qsTr("—") : Number(voltage).toFixed(1) + " V"
                }
                QGCLabel { text: qsTr("Current") }
                QGCLabel {
                    text: isNaN(currentA) ? qsTr("—") : Number(currentA).toFixed(1) + " A"
                }
                QGCLabel { text: qsTr("Power") }
                QGCLabel {
                    text: control._xf200PowerText(powerWatts)
                }
                QGCLabel { text: qsTr("Temperature") }
                QGCLabel {
                    text: isNaN(temperatureC) ? qsTr("—") : Number(temperatureC).toFixed(1) + " °C"
                }
            }
        }
    }

    Component {
        id: batteryVisual

        Row {
            Layout.fillHeight:  true
            spacing:            ScreenTools.defaultFontPixelWidth / 4

            function getBatteryColor() {
                switch (battery.chargeState.rawValue) {
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_OK:
                        if (!isNaN(battery.percentRemaining.rawValue)) {
                            if (battery.percentRemaining.rawValue > threshold1) {
                                return qgcPal.colorGreen
                            } else if (battery.percentRemaining.rawValue > threshold2) {
                                return qgcPal.colorYellowGreen
                            } else {
                                return qgcPal.colorYellow
                            }
                        } else {
                            return qgcPal.text
                        }
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_LOW:
                        return qgcPal.colorOrange
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_CRITICAL:
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_EMERGENCY:
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_FAILED:
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
                        return qgcPal.colorRed
                    default:
                        return qgcPal.text
                }
            }

            function getBatterySvgSource() {
                switch (battery.chargeState.rawValue) {
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_OK:
                        if (!isNaN(battery.percentRemaining.rawValue)) {
                            if (battery.percentRemaining.rawValue > threshold1) {
                                return "/qmlimages/BatteryGreen.svg"
                            } else if (battery.percentRemaining.rawValue > threshold2) {
                                return "/qmlimages/BatteryYellowGreen.svg"
                            } else {
                                return "/qmlimages/BatteryYellow.svg"
                            }
                        }
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_LOW:
                        return "/qmlimages/BatteryOrange.svg" // Low with orange svg
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_CRITICAL:
                        return "/qmlimages/BatteryCritical.svg" // Critical with red svg
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_EMERGENCY:
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_FAILED:
                    case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
                        return "/qmlimages/BatteryEMERGENCY.svg" // Exclamation mark
                    default:
                        return "/qmlimages/Battery.svg" // Fallback if percentage is unavailable
                }
            }

            function getBatteryPercentageText() {
                if (!isNaN(battery.percentRemaining.rawValue)) {
                    if (battery.percentRemaining.rawValue > 98.9) {
                        return qsTr("100%")
                    } else {
                        return battery.percentRemaining.valueString + battery.percentRemaining.units
                    }
                } else if (!isNaN(battery.voltage.rawValue)) {
                    return Number(battery.voltage.rawValue).toFixed(1) + " V"
                } else if (battery.chargeState.rawValue !== MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return qsTr("n/a")
            }

            function getBatteryVoltageText() {
                if (!isNaN(battery.voltage.rawValue)) {
                    return Number(battery.voltage.rawValue).toFixed(1) + " V"
                } else if (battery.chargeState.rawValue !== MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return qsTr("n/a")
            }

            Timer {
                id:         debounceRecalcTimer
                interval:   50
                running:    false
                repeat:     false
                onTriggered: {
                    control._recalcLowestBatteryId()
                }
            }
            Connections {
                target: battery.percentRemaining
                function onRawValueChanged() {
                    debounceRecalcTimer.restart()
                }
            }
            Connections {
                target: battery.voltage
                function onRawValueChanged() {
                    debounceRecalcTimer.restart()
                }
            }
            Connections {
                target: battery.chargeState
                function onRawValueChanged() {
                    debounceRecalcTimer.restart()
                }
            }

            QGCColoredImage {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                width:              height
                sourceSize.width:   width
                source:             getBatterySvgSource()
                fillMode:           Image.PreserveAspectFit
                color:              getBatteryColor()
            }

           ColumnLayout {
                id:                     batteryInfoColumn
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
                spacing:                0

                QGCLabel {
                    Layout.alignment:       Qt.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                    color:                  qgcPal.text
                    text:                   getBatteryPercentageText()
                    font.pointSize:         _showBoth ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    visible:                _showBoth || _showPercentage
                }

                QGCLabel {
                    Layout.alignment:       Qt.AlignHCenter
                    font.pointSize:         _showBoth ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    color:                  qgcPal.text
                    text:                   getBatteryVoltageText()
                    visible:                _showBoth || _showVoltage
                }
            }
        }
    }

    Component {
        id: batteryContentComponent

        Item {
            id: batteryTableRoot
            implicitWidth: tableWidth
            implicitHeight: tableCol.implicitHeight
            width: implicitWidth
            height: implicitHeight

            readonly property int colCount: 7
            readonly property real colGap: ScreenTools.defaultFontPixelWidth * 1.5
            readonly property real cellPadH: ScreenTools.defaultFontPixelWidth * 0.4
            readonly property real rowPad: ScreenTools.defaultFontPixelHeight / 5
            readonly property real fontPt: ScreenTools.defaultFontPointSize
            readonly property real chevronPad: ScreenTools.largeFontPixelHeight

            FontMetrics {
                id: headerFontMetrics
                font.family: ScreenTools.normalFontFamily
                font.pointSize: batteryTableRoot.fontPt
                font.bold: true
            }

            FontMetrics {
                id: cellFontMetrics
                font.family: ScreenTools.normalFontFamily
                font.pointSize: batteryTableRoot.fontPt
            }

            function _headerAt(col) {
                switch (col) {
                case 0: return qsTr("Battery")
                case 1: return qsTr("State")
                case 2: return qsTr("Remaining")
                case 3: return qsTr("Voltage")
                case 4: return qsTr("Current")
                case 5: return qsTr("Power")
                case 6: return qsTr("Temperature")
                }
                return ""
            }

            function _chargeStateText(battery) {
                if (battery.chargeState.rawValue === MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return qsTr("—")
                }
                return battery.chargeState.enumStringValue
            }

            function _remainingText(battery) {
                if (!isNaN(battery.percentRemaining.rawValue)) {
                    return battery.percentRemaining.valueString + battery.percentRemaining.units
                }
                if (!isNaN(battery.timeRemaining.rawValue)) {
                    return battery.timeRemainingStr.value
                }
                return qsTr("—")
            }

            function _voltageText(battery) {
                if (!isNaN(battery.voltage.rawValue)) {
                    return Number(battery.voltage.rawValue).toFixed(1) + " V"
                }
                return qsTr("—")
            }

            function _currentText(battery) {
                if (!isNaN(battery.current.rawValue)) {
                    return Number(battery.current.rawValue).toFixed(1) + " A"
                }
                return qsTr("—")
            }

            function _powerText(battery) {
                if (isNaN(battery.instantPower.rawValue)) {
                    return qsTr("—")
                }
                var watts = battery.instantPower.rawValue
                if (Math.abs(watts) < 1000) {
                    return Number(watts).toFixed(0) + " W"
                }
                return Number(watts / 1000).toFixed(2) + " kW"
            }

            function _temperatureText(battery) {
                if (!isNaN(battery.temperature.rawValue)) {
                    return Number(battery.temperature.rawValue).toFixed(1) + " °C"
                }
                return qsTr("—")
            }

            function _cellText(battery, col) {
                if (!battery) {
                    return qsTr("—")
                }
                switch (col) {
                case 0:
                    return _activeVehicle && _activeVehicle.batteries.count === 1
                           ? qsTr("Status") : battery.id.rawValue
                case 1: return _chargeStateText(battery)
                case 2: return _remainingText(battery)
                case 3: return _voltageText(battery)
                case 4: return _currentText(battery)
                case 5: return _powerText(battery)
                case 6: return _temperatureText(battery)
                }
                return qsTr("—")
            }

            function _isAbnormalBattery(battery) {
                switch (battery.chargeState.rawValue) {
                case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_LOW:
                case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_CRITICAL:
                case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_EMERGENCY:
                case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_FAILED:
                case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
                    return true
                }
                if (!isNaN(battery.percentRemaining.rawValue) &&
                        battery.percentRemaining.rawValue <= control.threshold2) {
                    return true
                }
                return false
            }

            function _rowBackground(index, battery) {
                if (_isAbnormalBattery(battery)) {
                    return Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.35)
                }
                return index % 2 ? qgcPal.windowShade : "transparent"
            }

            // Size columns from headers + worst-case samples, not live telemetry.
            // Measuring BATTERY_STATUS values here retriggered layout on every
            // voltage/current/power tick, which stuttered the UI and made the
            // modal drawer jump so taps outside would not close it.
            function _widthSamples(col) {
                switch (col) {
                case 0: return [qsTr("Status"), "99"]
                case 1: return [qsTr("Unhealthy"), qsTr("Emergency"), qsTr("Charging"), qsTr("Critical")]
                case 2: return ["100%", "0:00:00"]
                case 3: return ["999.9 V"]
                case 4: return ["999.9 A"]
                case 5: return ["99.99 kW", "999 W"]
                case 6: return ["99.9 °C"]
                }
                return []
            }

            function _columnWidth(col) {
                var w = headerFontMetrics.advanceWidth(_headerAt(col))
                var samples = _widthSamples(col)
                for (var i = 0; i < samples.length; i++) {
                    w = Math.max(w, cellFontMetrics.advanceWidth(samples[i]))
                }
                return Math.ceil(w + cellPadH * 2)
            }

            readonly property var colWidths: [
                _columnWidth(0), _columnWidth(1), _columnWidth(2),
                _columnWidth(3), _columnWidth(4), _columnWidth(5),
                _columnWidth(6)
            ]

            readonly property real bodyWidth: {
                var s = 0
                for (var i = 0; i < colCount; i++) {
                    s += colWidths[i]
                }
                return s + colGap * (colCount - 1)
            }

            readonly property real tableWidth: bodyWidth + chevronPad

            Column {
                id: tableCol
                width: batteryTableRoot.bodyWidth
                spacing: ScreenTools.defaultFontPixelHeight / 6

            Row {
                spacing: batteryTableRoot.colGap
                width: batteryTableRoot.bodyWidth

                Repeater {
                    model: batteryTableRoot.colCount

                    QGCLabel {
                        width: batteryTableRoot.colWidths[index]
                        font.pointSize: batteryTableRoot.fontPt
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.NoWrap
                        text: batteryTableRoot._headerAt(index)
                    }
                }
            }

            Rectangle {
                width: batteryTableRoot.bodyWidth
                height: 1
                color: qgcPal.groupBorder
            }

            Repeater {
                model: _activeVehicle ? _activeVehicle.batteries : 0

                Rectangle {
                    property var battery: object
                    width: batteryTableRoot.bodyWidth
                    height: batteryRow.implicitHeight + batteryTableRoot.rowPad * 2
                    radius: ScreenTools.defaultFontPixelWidth / 4
                    color: batteryTableRoot._rowBackground(index, battery)

                    Row {
                        id: batteryRow
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: batteryTableRoot.colGap

                        Repeater {
                            model: batteryTableRoot.colCount

                            QGCLabel {
                                width: batteryTableRoot.colWidths[index]
                                font.pointSize: batteryTableRoot.fontPt
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.NoWrap
                                text: batteryTableRoot._cellText(battery, index)
                            }
                        }
                    }
                }
            }
        }
        }
    }

    Component {
        id: batteryExpandedComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            property real batteryIconHeight: ScreenTools.defaultFontPixelWidth * 3

            FactPanelController { id: controller }

            SettingsGroupLayout {
                heading:            qsTr("Battery Display")
                Layout.fillWidth:   true
                visible:            !control._xf200Tethered

                FactCheckBoxSlider {
                    Layout.fillWidth:   true
                    fact:               _batterySettings.consolidateMultipleBatteries
                    text:               qsTr("Only show battery with lowest charge")
                    visible:            fact.userVisible
                }

                LabelledFactComboBox {
                    label:      qsTr("Value")
                    fact:       _batterySettings.valueDisplay
                    visible:    fact.userVisible
                }

                ColumnLayout {
                    QGCLabel { text: qsTr("Coloring") }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        // Battery 100%
                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth * 0.05  // Tighter spacing for icon and label
                            QGCColoredImage {
                                source: "/qmlimages/BatteryGreen.svg"
                                width: height
                                height: batteryIconHeight
                                fillMode: Image.PreserveAspectFit
                                color: qgcPal.colorGreen
                            }
                            QGCLabel { text: qsTr("100%") }
                        }

                        // Threshold 1
                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth * 0.05  // Tighter spacing for icon and field
                            QGCColoredImage {
                                source: "/qmlimages/BatteryYellowGreen.svg"
                                width: height
                                height: batteryIconHeight
                                fillMode: Image.PreserveAspectFit
                                color: qgcPal.colorYellowGreen
                            }
                            FactTextField {
                                id: threshold1Field
                                fact: _batterySettings.threshold1
                                implicitWidth: ScreenTools.defaultFontPixelWidth * 6
                                height: ScreenTools.defaultFontPixelHeight * 1.5
                                enabled: fact.userVisible
                                onEditingFinished: {
                                    // Validate and set the new threshold value
                                    _batterySettings.setThreshold1(parseInt(text));
                                }
                            }
                        }

                        // Threshold 2
                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth * 0.05  // Tighter spacing for icon and field
                            QGCColoredImage {
                                source: "/qmlimages/BatteryYellow.svg"
                                width: height
                                height: batteryIconHeight
                                fillMode: Image.PreserveAspectFit
                                color: qgcPal.colorYellow
                            }
                            FactTextField {
                                fact: _batterySettings.threshold2
                                implicitWidth: ScreenTools.defaultFontPixelWidth * 6
                                height: ScreenTools.defaultFontPixelHeight * 1.5
                                enabled: fact.userVisible
                                onEditingFinished: {
                                    // Validate and set the new threshold value
                                    _batterySettings.setThreshold2(parseInt(text));
                                }
                            }
                        }

                        // Low state
                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth * 0.05  // Tighter spacing for icon and label
                            QGCColoredImage {
                                source: "/qmlimages/BatteryOrange.svg"
                                width: height
                                height: batteryIconHeight
                                fillMode: Image.PreserveAspectFit
                                color: qgcPal.colorOrange
                            }
                            QGCLabel { text: qsTr("Low") }
                        }

                        // Critical state
                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth * 0.05  // Tighter spacing for icon and label
                            QGCColoredImage {
                                source: "/qmlimages/BatteryCritical.svg"
                                width: height
                                height: batteryIconHeight
                                fillMode: Image.PreserveAspectFit
                                color: qgcPal.colorRed
                            }
                            QGCLabel { text: qsTr("Critical") }
                        }
                    }
                }
            }

            Loader {
                Layout.fillWidth:   true
                source:             _activeVehicle.expandedToolbarIndicatorSource("Battery")
            }

            SettingsGroupLayout {
                visible: _activeVehicle.autopilotPlugin.knownVehicleComponentAvailable(AutoPilotPlugin.KnownPowerVehicleComponent) &&
                            QGroundControl.corePlugin.showAdvancedUI

                LabelledButton {
                    label:      qsTr("Vehicle Power")
                    buttonText: qsTr("Configure")

                    onClicked: {
                        mainWindow.showKnownVehicleComponentConfigPage(AutoPilotPlugin.KnownPowerVehicleComponent)
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }
        }
    }
}
