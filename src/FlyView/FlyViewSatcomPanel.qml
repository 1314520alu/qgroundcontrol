import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Compact satellite-antenna control pad for the Weitong aircraft.
/// Telemetry lives in the toolbar; this pad is unfold/fold + azimuth/elevation.
Rectangle {
    id: root

    property var controller: QGroundControl.satcomAntennaController

    visible: controller && controller.enabled
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.82)
    radius: ScreenTools.defaultFontPixelWidth / 2
    border.width: 1
    border.color: qgcPal.groupBorder

    implicitWidth: content.implicitWidth + ScreenTools.defaultFontPixelWidth
    implicitHeight: content.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.5

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property real _pad: ScreenTools.defaultFontPixelWidth * 0.45
    readonly property real _btnW: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 7)
    readonly property real _btnH: Math.max(ScreenTools.minTouchPixels * 0.85, ScreenTools.defaultFontPixelHeight * 1.8)
    readonly property bool _ready: controller && controller.connected

    ColumnLayout {
        id: content
        anchors.centerIn: parent
        spacing: ScreenTools.defaultFontPixelHeight * 0.2

        QGCLabel {
            text: qsTr("Antenna")
            font.bold: true
            font.pointSize: ScreenTools.smallFontPointSize
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            spacing: root._pad
            Layout.alignment: Qt.AlignHCenter

            QGCButton {
                text: qsTr("Unfold")
                Layout.preferredWidth: root._btnW
                Layout.preferredHeight: root._btnH
                enabled: root._ready
                onClicked: root.controller.unfoldAntenna()
            }
            QGCButton {
                text: qsTr("Fold")
                Layout.preferredWidth: root._btnW
                Layout.preferredHeight: root._btnH
                enabled: root._ready
                onClicked: root.controller.foldAntenna()
            }
        }

        GridLayout {
            columns: 3
            columnSpacing: root._pad
            rowSpacing: root._pad
            Layout.alignment: Qt.AlignHCenter
            enabled: root._ready

            Item { Layout.preferredWidth: root._btnH; Layout.preferredHeight: root._btnH }

            QGCButton {
                text: qsTr("El+")
                Layout.preferredWidth: root._btnH
                Layout.preferredHeight: root._btnH
                onClicked: root.controller.elevationUp()
            }

            Item { Layout.preferredWidth: root._btnH; Layout.preferredHeight: root._btnH }

            QGCButton {
                text: qsTr("Az-")
                Layout.preferredWidth: root._btnH
                Layout.preferredHeight: root._btnH
                onClicked: root.controller.azimuthDown()
            }

            Item { Layout.preferredWidth: root._btnH; Layout.preferredHeight: root._btnH }

            QGCButton {
                text: qsTr("Az+")
                Layout.preferredWidth: root._btnH
                Layout.preferredHeight: root._btnH
                onClicked: root.controller.azimuthUp()
            }

            Item { Layout.preferredWidth: root._btnH; Layout.preferredHeight: root._btnH }

            QGCButton {
                text: qsTr("El-")
                Layout.preferredWidth: root._btnH
                Layout.preferredHeight: root._btnH
                onClicked: root.controller.elevationDown()
            }

            Item { Layout.preferredWidth: root._btnH; Layout.preferredHeight: root._btnH }
        }
    }
}
