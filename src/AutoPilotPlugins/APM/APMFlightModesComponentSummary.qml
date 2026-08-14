import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    FactPanelController { id: controller }

    property bool _roverFirmware: controller.parameterExists(-1, "MODE1")
    property Fact flightMode1: controller.getParameterFact(-1, _roverFirmware ? "MODE1" : "FLTMODE1")
    property Fact flightMode2: controller.getParameterFact(-1, _roverFirmware ? "MODE2" : "FLTMODE2")
    property Fact flightMode3: controller.getParameterFact(-1, _roverFirmware ? "MODE3" : "FLTMODE3")
    property Fact flightMode4: controller.getParameterFact(-1, _roverFirmware ? "MODE4" : "FLTMODE4")
    property Fact flightMode5: controller.getParameterFact(-1, _roverFirmware ? "MODE5" : "FLTMODE5")
    property Fact flightMode6: controller.getParameterFact(-1, _roverFirmware ? "MODE6" : "FLTMODE6")

    GridLayout {
        id: mainLayout
        width: parent.width
        columns: 2
        columnSpacing: ScreenTools.defaultFontPixelWidth * 0.5
        rowSpacing: ScreenTools.defaultFontPixelHeight * 0.35

        SummaryChip { Layout.fillWidth: true; text: "1  " + flightMode1.enumStringValue }
        SummaryChip { Layout.fillWidth: true; text: "2  " + flightMode2.enumStringValue }
        SummaryChip { Layout.fillWidth: true; text: "3  " + flightMode3.enumStringValue }
        SummaryChip { Layout.fillWidth: true; text: "4  " + flightMode4.enumStringValue }
        SummaryChip { Layout.fillWidth: true; text: "5  " + flightMode5.enumStringValue }
        SummaryChip { Layout.fillWidth: true; text: "6  " + flightMode6.enumStringValue }
    }
}
