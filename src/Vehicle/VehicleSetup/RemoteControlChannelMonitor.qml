/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Generic version of Channel Monitor which should work with both RC Transmitters and Joysticks
/// Used to display raw channel values
GridLayout {
    required property int channelCount
    required property int channelValueMin
    required property int channelValueMax

    /// First channel number shown (1-based). Use 5 when attitude axes already cover CH1–4.
    property int channelStart: 1
    property bool twoColumn: false
    property int columnCount: twoColumn ? 2 : 1
    property bool compact: false
    property string title: qsTr("通道监视")

    /// Should be called by consumers whenever a raw channel value changes.
    /// `channel` is 0-based from the controller (CH1 → 0).
    function rawChannelValueChanged(channel, channelValue) {
        const itemIndex = channel - (channelStart - 1)
        if (itemIndex < 0 || itemIndex >= channelCount) {
            return
        }
        const row = channelMonitorRepeater.itemAt(itemIndex)
        if (!row) {
            return
        }
        let channelValueDisplayItem = row._channelValueDisplayLoader.item
        let filteredChannelValue = Math.min(Math.max(channelValue, channelValueMin), channelValueMax)
        channelValueDisplayItem.channelValue = filteredChannelValue
        if (row._valueLabel) {
            row._valueLabel.text = filteredChannelValue
        }
    }

    id: control
    columns: Math.max(1, columnCount)
    columnSpacing: ScreenTools.defaultFontPixelWidth * (compact ? 0.65 : 1.0)
    rowSpacing: ScreenTools.defaultFontPixelHeight * (compact ? 0.28 : 0.45)

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    QGCLabel {
        Layout.columnSpan: parent.columns
        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
        Layout.preferredHeight: implicitHeight
        Layout.maximumHeight: implicitHeight
        Layout.bottomMargin: compact ? ScreenTools.defaultFontPixelHeight * 0.15 : 0
        text: control.title
        font.bold: true
        font.pointSize: ScreenTools.defaultFontPointSize * (compact ? 0.95 : 1.05)
    }

    Repeater {
        id: channelMonitorRepeater
        model: channelCount

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: control.compact
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * (compact ? 1.35 : 1.7)
            Layout.minimumHeight: ScreenTools.defaultFontPixelHeight * (compact ? 1.15 : 1.4)
            Layout.alignment: Qt.AlignTop
            spacing: ScreenTools.defaultFontPixelWidth * (control.compact ? 0.55 : 0.35)

            property var _channelValueDisplayLoader: channelValueDisplayLoader
            property var _valueLabel: valueLabel
            readonly property int channelNumber: control.channelStart + index

            QGCLabel {
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * (control.columns >= 4 ? 3.6 : 3.5)
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * (control.columns >= 4 ? 3.6 : 3.5)
                Layout.alignment: Qt.AlignVCenter
                z: 1
                text: qsTr("CH%1").arg(channelNumber)
                font.pointSize: ScreenTools.defaultFontPointSize * (compact ? 0.85 : 1.0)
            }

            Loader {
                id: channelValueDisplayLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * (compact ? 1.25 : 1.4)
                Layout.maximumHeight: ScreenTools.defaultFontPixelHeight * (compact ? 1.6 : 2.0)
                Layout.alignment: Qt.AlignVCenter
                clip: true
                sourceComponent: RemoteControlChannelValueDisplay {
                    mode: RemoteControlChannelValueDisplay.RawValue
                    channelValueMin: control.channelValueMin
                    channelValueMax: control.channelValueMax
                    accentIndicator: true
                    thickTrack: control.compact
                }
            }

            QGCLabel {
                id: valueLabel
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * (control.columns >= 4 ? 3.8 : 4.5)
                Layout.alignment: Qt.AlignVCenter
                horizontalAlignment: Text.AlignRight
                font.family: ScreenTools.fixedFontFamily
                font.pointSize: ScreenTools.defaultFontPointSize * (compact ? 0.85 : 1.0)
                text: "—"
                opacity: 0.7
            }
        }
    }
}
