import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: _root

    property var parentToolInsets
    property var totalToolInsets: _totalToolInsets
    property var mapControl

    readonly property bool _hudStyle: QGroundControl.settingsManager.flyViewSettings.flyViewUiStyle.rawValue === 1

    property var  _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real _toolsMargin:   ScreenTools.defaultFontPixelWidth * 0.75
    property real _attitudeSize:  ScreenTools.defaultFontPixelHeight * 8

    visible: _hudStyle && !QGroundControl.videoManager.fullScreen

    QGCToolInsets {
        id:                   _totalToolInsets
        leftEdgeTopInset:     visible ? mapToolRail.x + mapToolRail.width + _toolsMargin : 0
        leftEdgeCenterInset:  leftEdgeTopInset
        leftEdgeBottomInset:  parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:    visible ? width - telemetryColumn.x + _toolsMargin : 0
        rightEdgeCenterInset: rightEdgeTopInset
        rightEdgeBottomInset: visible ? width - guidedButtons.x + _toolsMargin : 0
        topEdgeLeftInset:     visible ? attitudeWidget.y + attitudeWidget.height + _toolsMargin : 0
        topEdgeCenterInset:   visible ? headingTape.y + headingTape.height + _toolsMargin : 0
        topEdgeRightInset:    topEdgeCenterInset
        bottomEdgeLeftInset:  parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset: 0
        bottomEdgeRightInset: visible ? height - guidedButtons.y + _toolsMargin : 0
    }

    FlyViewHeadingTape {
        id:                       headingTape
        anchors.top:              parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        z:                        QGroundControl.zOrderWidgets
    }

    QGCAttitudeWidget {
        id:            attitudeWidget
        anchors.left:  parent.left
        anchors.top:   headingTape.bottom
        anchors.topMargin: _toolsMargin
        size:          _attitudeSize
        vehicle:       _activeVehicle
        showHeading:   false
        showPitch:     true
        z:             QGroundControl.zOrderWidgets
    }

    FlyViewHudMapToolRail {
        id:            mapToolRail
        anchors.left:  parent.left
        anchors.top:   attitudeWidget.bottom
        anchors.topMargin: _toolsMargin
        mapControl:    _root.mapControl
        z:             QGroundControl.zOrderWidgets
    }

    FlyViewHudTelemetryColumn {
        id:             telemetryColumn
        anchors.right:  parent.right
        anchors.top:    headingTape.bottom
        anchors.topMargin: _toolsMargin
        z:              QGroundControl.zOrderWidgets
    }

    FlyViewHudGuidedButtons {
        id:              guidedButtons
        anchors.right:   parent.right
        anchors.bottom:  parent.bottom
        z:               QGroundControl.zOrderWidgets
    }
}
