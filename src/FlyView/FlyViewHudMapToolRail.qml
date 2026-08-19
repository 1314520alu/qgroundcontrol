import QtQuick

import QGroundControl
import QGroundControl.Controls

Column {
    id:      control
    spacing: ScreenTools.defaultFontPixelWidth * 0.5

    property var mapControl
    property real _buttonSize: Math.max(ScreenTools.minTouchPixels * 0.85, ScreenTools.defaultFontPixelHeight * 2.6)

    QGCPalette { id: qgcPal }

    function _centerOnVehicle() {
        if (mapControl && mapControl.centerOnVehicle) {
            mapControl.centerOnVehicle()
        }
    }

    function _cycleMapType() {
        var mapSettings = QGroundControl.settingsManager.flightMapSettings
        var types = QGroundControl.mapEngineManager.mapTypeList(mapSettings.mapProvider.rawValue)
        if (!types || types.length === 0) {
            return
        }
        var current = mapSettings.mapType.rawValue
        var nextIndex = 0
        for (var i = 0; i < types.length; i++) {
            if (types[i] == current) {
                nextIndex = (i + 1) % types.length
                break
            }
        }
        mapSettings.mapType.rawValue = types[nextIndex]
    }

    HudRailButton {
        iconSource: "/InstrumentValueIcons/target.svg"
        onClicked:  control._centerOnVehicle()
    }
    HudRailButton {
        iconSource: "/res/waypoint.svg"
        onClicked:  mainWindow.showPlanView()
    }
    HudRailButton {
        iconSource: "/qmlimages/Plan.svg"
        onClicked:  mainWindow.showPlanView()
    }
    HudRailButton {
        iconSource: "/InstrumentValueIcons/layers.svg"
        onClicked:  control._cycleMapType()
    }
    HudRailButton {
        iconSource: "/InstrumentValueIcons/cog.svg"
        onClicked:  mainWindow.showToolSelectDialog()
    }

    component HudRailButton: Item {
        id:     railButton
        width:  control._buttonSize
        height: control._buttonSize

        property string iconSource
        signal clicked()

        Rectangle {
            anchors.fill: parent
            radius:       width / 2
            color:        qgcPal.window
            border.color: qgcPal.groupBorder
            border.width: 1
            opacity:      0.92

            QGCColoredImage {
                anchors.centerIn: parent
                width:            parent.width * 0.46
                height:           width
                source:           railButton.iconSource
                fillMode:         Image.PreserveAspectFit
                color:            qgcPal.buttonText
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked:    railButton.clicked()
        }
    }
}
