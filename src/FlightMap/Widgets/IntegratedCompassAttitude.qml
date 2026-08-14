import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id:             control
    implicitWidth:  (compassRadius * 2) + attitudeSpacing + attitudeSize
    implicitHeight: implicitWidth

    property alias attitudeSize:                rollIndicator.attitudeSize
    property alias attitudeSpacing:             rollIndicator.attitudeSpacing
    property real extraInset:                   attitudeSize + attitudeSpacing
    property real extraValuesWidth:             0
    property real defaultCompassRadius:         (mainWindow.width * 0.15) / 2
    property real maxCompassRadius:             ScreenTools.defaultFontPixelHeight * 7 / 2
    property real compassRadius:                Math.min(defaultCompassRadius, maxCompassRadius)
    property real compassBorder:                ScreenTools.defaultFontPixelHeight / 2
    property var  vehicle:                      globals.activeVehicle
    property var  qgcPal:                       QGroundControl.globalPalette
    property bool usedByMultipleVehicleList:    false

    property real _totalAttitudeSize:           attitudeSize + attitudeSpacing
    property bool _showAttitudeValues:          !usedByMultipleVehicleList
    property real _valueLabelGap:               ScreenTools.defaultFontPixelWidth * 0.5
    property real _compassCenterX:              compassRadius
    property real _compassCenterY:              _totalAttitudeSize + compassRadius
    property real _rollDegrees:                 vehicle ? vehicle.roll.rawValue : 0
    property real _pitchDegrees:                vehicle ? vehicle.pitch.rawValue : 0

    IntegratedAttitudeIndicator {
        id:                     rollIndicator
        x:                      -_totalAttitudeSize
        attitudeAngleDegrees:   _rollDegrees
        compassRadius:          control.compassRadius

        // Roll value inside the top roll arc window, to the right of the center tick
        QGCLabel {
            visible:                _showAttitudeValues
            text:                   Math.round(_rollDegrees) + "°"
            color:                  qgcPal.text
            font.bold:              true
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: _valueLabelGap + width / 2
            anchors.top:            parent.top
            height:                 attitudeSize
            verticalAlignment:      Text.AlignVCenter
        }
    }

    IntegratedAttitudeIndicator {
        id:                     pitchIndicator
        x:                      -_totalAttitudeSize
        attitudeAngleDegrees:   _pitchDegrees
        compassRadius:          control.compassRadius
        attitudeSize:           control.attitudeSize
        attitudeSpacing:        control.attitudeSpacing
        transformOrigin:        Item.Center
        rotation:               90

        // Pitch value inside the pitch arc window (same local slot as roll; counter-rotate so text stays upright)
        QGCLabel {
            visible:                _showAttitudeValues
            text:                   Math.round(_pitchDegrees) + "°"
            color:                  qgcPal.text
            font.bold:              true
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: _valueLabelGap + width / 2
            anchors.top:            parent.top
            height:                 attitudeSize
            verticalAlignment:      Text.AlignVCenter
            rotation:               -90
            transformOrigin:        Item.Center
        }
    }

    Rectangle {
        y:      _totalAttitudeSize
        width:  compassRadius * 2
        height: width
        radius: width / 2
        color:  usedByMultipleVehicleList ? qgcPal.window : qgcPal.windowTransparent

        QGCCompassWidget {
            size:                       parent.width - compassBorder
            vehicle:                    control.vehicle
            usedByMultipleVehicleList:  control.usedByMultipleVehicleList
            anchors.centerIn:           parent
        }
    }
}
