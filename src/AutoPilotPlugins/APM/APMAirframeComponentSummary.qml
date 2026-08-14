import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    APMAirframeComponentController { id: controller }

    property Fact _frameClass: controller.getParameterFact(-1, "FRAME_CLASS")
    property Fact _frameType: controller.getParameterFact(-1, "FRAME_TYPE", false)
    property bool _frameTypeAvailable: controller.parameterExists(-1, "FRAME_TYPE")
    property int _motorCount: globals.activeVehicle ? globals.activeVehicle.motorCount : -1

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.35

        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth * 0.5
            SummaryChip { text: _frameClass.enumStringValue }
            SummaryChip {
                visible: _frameTypeAvailable
                text: _frameType ? _frameType.enumStringValue : ""
            }
            SummaryChip {
                visible: _motorCount > 0
                text: qsTr("%1 motors").arg(_motorCount)
            }
        }

        QGCLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: QGroundControl.globalPalette.text
            opacity: 0.75
            font.pointSize: ScreenTools.defaultFontPointSize * 0.9
            text: {
                const v = globals.activeVehicle
                if (!v || v.firmwareMajorVersion === -1) {
                    return qsTr("Firmware: Unknown")
                }
                return qsTr("Firmware %1.%2.%3%4")
                    .arg(v.firmwareMajorVersion)
                    .arg(v.firmwareMinorVersion)
                    .arg(v.firmwarePatchVersion)
                    .arg(v.firmwareVersionTypeString)
            }
        }
    }
}
