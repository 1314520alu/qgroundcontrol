import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Column {
    id:      control
    spacing: ScreenTools.defaultFontPixelWidth * 0.6

    property var _guidedController: globals.guidedControllerFlyView
    property real _buttonSize:      Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelHeight * 3.2)

    QGCPalette { id: qgcPal }

    HudCircleButton {
        visible:        true
        enabled:        _guidedController.showRTL
        highlight:      true
        iconSource:     "/res/rtl.svg"
        label:          _guidedController.rtlTitle
        onClicked: {
            _guidedController.closeAll()
            _guidedController.confirmAction(_guidedController.actionRTL)
        }
    }

    HudCircleButton {
        visible:        _guidedController.showLand && !_guidedController.showTakeoff
        enabled:        _guidedController.showLand
        iconSource:     "/res/land.svg"
        label:          _guidedController.landTitle
        onClicked: {
            _guidedController.closeAll()
            _guidedController.confirmAction(_guidedController.actionLand)
        }
    }

    HudCircleButton {
        visible:        _guidedController.showTakeoff || !_guidedController.showLand
        enabled:        _guidedController.showTakeoff
        iconSource:     "/res/takeoff.svg"
        label:          _guidedController.takeoffTitle
        onClicked: {
            _guidedController.closeAll()
            _guidedController.confirmAction(_guidedController.actionTakeoff)
        }
    }

    component HudCircleButton: Item {
        id:      buttonRoot
        width:   control._buttonSize
        height:  visible ? control._buttonSize : 0

        property bool   highlight:  false
        property bool   enabled:    true
        property string iconSource
        property string label

        signal clicked()

        opacity: enabled ? 1 : 0.4

        Rectangle {
            anchors.fill: parent
            radius:       width / 2
            color:        buttonRoot.highlight ? qgcPal.buttonHighlight : qgcPal.window
            border.color: qgcPal.buttonHighlight
            border.width: 1

            QGCColoredImage {
                anchors.centerIn:   parent
                width:              parent.width * 0.46
                height:             width
                source:             buttonRoot.iconSource
                fillMode:           Image.PreserveAspectFit
                color:              buttonRoot.highlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled:      buttonRoot.enabled
            onClicked:    buttonRoot.clicked()
        }
    }
}
