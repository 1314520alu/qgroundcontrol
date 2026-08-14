import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

/// Displays the value of a single channel as an indicator within a value range bar
Item {
    enum Mode {
        RawValue, // Display raw channel value
        MappedValue // Channel goes through a mapping process and can be reversed
    }

    required property int channelValueMin
    required property int channelValueMax
    required property int mode ///< Mode of display using Mode enum values
    property bool channelMapped: false
    property int channelValue: _valueBarRange / 2 + _valueBarMin
    property int deadbandValue: 0
    property bool deadbandEnabled: false
    /// Preview-style: gray track + blue fill to value + blue thumb
    property bool accentIndicator: false
    /// Compact Radio monitor: ~1.5× thicker track
    property bool thickTrack: false

    id: control
    implicitHeight: ScreenTools.defaultFontPixelHeight

    readonly property real _valueRangeBarOvershootPercent: accentIndicator ? 0.05 : 0.2
    readonly property int _halfRange: (channelValueMax - channelValueMin) / 2
    readonly property int _valueBarMin: channelValueMin - (_halfRange * _valueRangeBarOvershootPercent)
    readonly property int _valueBarMax: channelValueMax + (_halfRange * _valueRangeBarOvershootPercent)
    readonly property int _valueBarRange: Math.max(1, _valueBarMax - _valueBarMin)

    readonly property real _normalized: {
        const clamped = Math.max(_valueBarMin, Math.min(_valueBarMax, channelValue))
        return (clamped - _valueBarMin) / _valueBarRange
    }
    readonly property real _trackHeightFactor: accentIndicator ? (thickTrack ? 0.57 : 0.38) : 0.5
    readonly property real _thumbSize: Math.max(ScreenTools.defaultFontPixelHeight * (thickTrack ? 1.05 : 0.85),
                                                 height * (thickTrack ? 0.78 : 0.7))

    Component.onCompleted: {
        if (mode === RemoteControlChannelValueDisplay.RawValue) {
            if (channelMapped) {
                console.warn("RemoteControlChannelValueDisplay: channelMapped should not be true in Raw Value mode")
                channelMapped = false
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    // Track
    Rectangle {
        id: fullValueRangeBar
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: accentIndicator
                ? Math.max(thickTrack ? 9 : 6, parent.height * _trackHeightFactor)
                : (parent.height / 2)
        radius: height / 2
        color: accentIndicator
               ? Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.10)
               : qgcPal.windowShade
    }

    // Blue fill from left → current value (matches radio-cal-preview-split)
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: fullValueRangeBar.left
        height: fullValueRangeBar.height
        width: {
            const thumbInset = accentIndicator ? control._thumbSize / 2 : 0
            const usable = Math.max(0, fullValueRangeBar.width - (accentIndicator ? control._thumbSize : 0))
            const fill = thumbInset + usable * control._normalized
            return Math.max(fullValueRangeBar.height, Math.min(fullValueRangeBar.width, fill))
        }
        radius: height / 2
        color: qgcPal.buttonHighlight
        visible: accentIndicator && (control.mode === RemoteControlChannelValueDisplay.RawValue || control.channelMapped)
        opacity: 0.85
    }

    Rectangle {
        id: deadbandRange
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height
        width: _deadbandWidth
        x: _deadbandOffset
        radius: ScreenTools.defaultFontPixelHeight / 4
        color: qgcPal.buttonHighlight
        opacity: 0.35
        visible: !accentIndicator && control.deadbandEnabled && control.deadbandValue > 0
        readonly property real _rangeSpan: Math.max(1, control.channelValueMax - control.channelValueMin)
        readonly property real _usableWidth: parent.width / (1 + control._valueRangeBarOvershootPercent)
        readonly property real _deadbandWidth: Math.min(1, (control.deadbandValue * 2) / _rangeSpan) * _usableWidth
        readonly property real _deadbandOffset: (parent.width - _usableWidth) / 2 + (_usableWidth - _deadbandWidth) / 2
    }

    Rectangle {
        id: centerPointDisplay
        anchors.horizontalCenter: parent.horizontalCenter
        width: 1
        height: parent.height * 0.85
        color: accentIndicator ? Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.22) : qgcPal.window
        visible: !accentIndicator || control.mode === RemoteControlChannelValueDisplay.MappedValue
    }

    QGCLabel {
        id: notMappedLabel
        anchors.centerIn: parent
        text: qsTr("未映射")
        visible: control.mode === RemoteControlChannelValueDisplay.MappedValue && !control.channelMapped
    }

    // Thumb — keep fully inside the bar so it does not cover CH labels / values
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: control._thumbSize
        height: width
        x: {
            const travel = Math.max(0, parent.width - width)
            return control._normalized * travel
        }
        radius: width / 2
        color: accentIndicator ? qgcPal.buttonHighlight : qgcPal.text
        border.width: accentIndicator ? 2 : 0
        border.color: qgcPal.window
        visible: control.mode === RemoteControlChannelValueDisplay.RawValue || control.channelMapped
    }
}
