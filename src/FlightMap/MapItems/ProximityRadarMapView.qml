import QtQuick
import QtQuick.Shapes
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

MapQuickItem {
    id:             _root
    objectName:     "proximityRadarMapView"
    visible:        proximityValues.telemetryAvailable && coordinate.isValid

    property var    vehicle                                                         /// Vehicle object, undefined for ADSB vehicle
    property var    map
    property double heading:    vehicle ? vehicle.heading.value : Number.NaN    ///< Vehicle heading, NAN for none

    anchorPoint.x:  vehicleItem.width  / 2
    anchorPoint.y:  vehicleItem.height / 2

    property real   _ratio: 1

    // Density-independent stroke width for the sensor arcs and limit circle
    readonly property real _strokeWidth: ScreenTools.defaultFontPixelHeight / 4

    // Each of the 8 sensor sectors covers 45 degrees, centered on its rotation direction.
    // Sector 0 is vehicle-forward, which is -90 degrees in PathAngleArc coordinates.
    readonly property real _sectorSweepAngle:      360 / 8
    readonly property real _firstSectorStartAngle: -90 - (_sectorSweepAngle / 2)

    // Cap the rendered circle size to keep item geometry sane at deep zoom levels. The clamp only
    // engages once the circle's rim is more than two viewport-widths from the vehicle, so the
    // clamped (wrong-sized) portion is offscreen and never visible.
    readonly property real _maximumDiameter: Math.max(map.width, map.height) * 4

    function calcSize() {
        var scaleLinePixelLength    = 100
        var leftCoord               = map.toCoordinate(Qt.point(0, 0), false /* clipToViewPort */)
        var rightCoord              = map.toCoordinate(Qt.point(scaleLinePixelLength, 0), false /* clipToViewPort */)
        var scaleLineMeters         = leftCoord.distanceTo(rightCoord)
        if (!isFinite(scaleLineMeters) || scaleLineMeters <= 0) {
            _ratio = 0
            return
        }
        _ratio = scaleLinePixelLength / scaleLineMeters
    }

    function _clampedDiameter(requestedDiameter) {
        if (!isFinite(requestedDiameter) || requestedDiameter < 0) {
            return 0
        }
        return Math.min(requestedDiameter, _maximumDiameter)
    }

    function _sectorVisible(sectorIndex) {
        return !isNaN(proximityValues.rgRotationValues[sectorIndex])
    }

    function _sectorRadius(sectorIndex) {
        var sectorDistance = proximityValues.rgRotationValues[sectorIndex]
        if (!_sectorVisible(sectorIndex)) {
            return 0
        }
        // Clamp for the same reason as _clampedDiameter: keep geometry coordinates sane at deep zoom
        return _clampedDiameter(sectorDistance * _ratio * 2) / 2
    }

    function _sectorColor(sectorIndex) {
        return _sectorVisible(sectorIndex) ? Qt.rgba(1, 0, 0, 1) : Qt.rgba(0, 0, 0, 0)
    }

    function _sectorStartAngle(sectorIndex) {
        return _firstSectorStartAngle + (sectorIndex * _sectorSweepAngle)
    }

    function _sectorMidAngleRad(sectorIndex) {
        // PathAngleArc: 0° = 3 o'clock, clockwise; sector 0 mid = forward (-90°).
        return (_sectorStartAngle(sectorIndex) + _sectorSweepAngle / 2) * Math.PI / 180
    }

    ProximityRadarValues {
        id:                     proximityValues
        vehicle:                _root.vehicle
    }

    Connections {
        target:             map
        function onWidthChanged() { scaleTimer.restart() }
        function onHeightChanged() { scaleTimer.restart() }
        function onZoomLevelChanged() { scaleTimer.restart() }
    }

    Timer {
        id:                 scaleTimer
        interval:           100
        running:            false
        repeat:             false
        onTriggered:        calcSize()
    }

    sourceItem: Item {
        id:         vehicleItem
        width:      detectionLimitCircle.width
        height:     detectionLimitCircle.height

        Component.onCompleted: calcSize()

        readonly property real _headingAngle: isNaN(heading) ? 0 : heading

        Rectangle {
            id:                 detectionLimitCircle
            width:              _clampedDiameter(proximityValues.maxDistance * 2 * _ratio)
            height:             width
            color:              Qt.rgba(1, 1, 1, 0)
            border.color:       Qt.rgba(1, 1, 1, 1)
            border.width:       _strokeWidth
            radius:             width * 0.5
            opacity:            0.5

            transform: Rotation {
                origin.x:       detectionLimitCircle.width  / 2
                origin.y:       detectionLimitCircle.height / 2
                angle:          vehicleItem._headingAngle
            }
        }

        // Sensor arcs — semi-transparent
        Shape {
            id:                 vehicleSensors
            anchors.fill:       detectionLimitCircle
            opacity:            0.5

            transform: Rotation {
                origin.x:       vehicleSensors.width  / 2
                origin.y:       vehicleSensors.height / 2
                angle:          vehicleItem._headingAngle
            }

            Instantiator {
                model: 8

                delegate: ShapePath {
                    required property int index

                    strokeColor:    _sectorColor(index)
                    strokeWidth:    _strokeWidth
                    fillColor:      "transparent"

                    PathAngleArc {
                        centerX:    vehicleSensors.width  / 2
                        centerY:    vehicleSensors.height / 2
                        radiusX:    _sectorRadius(index)
                        radiusY:    radiusX
                        startAngle: _sectorStartAngle(index)
                        sweepAngle: _sectorSweepAngle
                    }
                }

                onObjectAdded: (index, object) => vehicleSensors.data.push(object)
            }
        }

        // Small distance labels beside each red arc (full opacity for readability)
        Item {
            anchors.fill: detectionLimitCircle

            transform: Rotation {
                origin.x: detectionLimitCircle.width / 2
                origin.y: detectionLimitCircle.height / 2
                angle:    vehicleItem._headingAngle
            }

            Repeater {
                model: 8

                QGCLabel {
                    required property int index

                    visible:            _sectorVisible(index)
                    text:               proximityValues.rgRotationValueStrings[index]
                    font.pointSize:     ScreenTools.defaultFontPointSize * 0.7
                    font.bold:          true
                    color:              "white"
                    style:              Text.Outline
                    styleColor:         "black"

                    x: detectionLimitCircle.width / 2
                       + _sectorRadius(index) * Math.cos(_sectorMidAngleRad(index))
                       - width / 2
                    y: detectionLimitCircle.height / 2
                       + _sectorRadius(index) * Math.sin(_sectorMidAngleRad(index))
                       - height / 2
                }
            }
        }
    }
}
