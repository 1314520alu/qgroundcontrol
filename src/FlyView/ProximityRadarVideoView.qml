import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Item {
    id:             _root
    anchors.fill:   parent
    visible:        proximityValues.telemetryAvailable

    property var    vehicle     ///< Vehicle object, undefined for ADSB vehicle
    // Video HUD: only show close obstacles (< 15 m). Far readings stay hidden.
    property real   range:  15
    readonly property real _maxDisplayDistanceM: 15

    // Video fills under the fly toolbar; keep radar in the visible band below it.
    readonly property real _topSafe:    ScreenTools.toolbarHeight + ScreenTools.defaultFontPixelHeight * 2
    readonly property real _bottomSafe: ScreenTools.defaultFontPixelHeight * 2.5
    readonly property real _safeHeight: Math.max(1, height - _topSafe - _bottomSafe)
    readonly property real _minlength:  Math.min(width, _safeHeight)
    readonly property real _ratio:      (_minlength * 0.42) / _root.range

    function _shouldShow(distanceM) {
        return !isNaN(distanceM) && distanceM > 0 && distanceM < _root._maxDisplayDistanceM
    }

    ProximityRadarValues {
        id:                     proximityValues
        vehicle:                _root.vehicle
        onRotationValueChanged: _sectorViewEllipsoid.requestPaint()
    }

    Item {
        id:                     radarHost
        anchors.left:           parent.left
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.topMargin:      _root._topSafe
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   _root._bottomSafe

        Canvas {
            id:             _sectorViewEllipsoid
            anchors.fill:   parent
            opacity:        0.65

            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.translate(width / 2, height / 2)
                ctx.strokeStyle = Qt.rgba(1, 0, 0, 1);
                ctx.lineWidth = Math.max(width / 55, ScreenTools.defaultFontPixelWidth * 0.35);
                ctx.scale(_root.width / _root._minlength, radarHost.height / _root._minlength);
                ctx.rotate(-Math.PI / 2 - Math.PI / 8);
                for (var i = 0; i < proximityValues.rgRotationValues.length; i++) {
                    var rotationValue = proximityValues.rgRotationValues[i]
                    if (_root._shouldShow(rotationValue)) {
                        var a = Math.PI / 4 * i;
                        ctx.beginPath();
                        ctx.arc(0, 0, Math.min(rotationValue, _root.range) * _root._ratio,
                                0 + a + Math.PI / 50, Math.PI / 4 + a - Math.PI / 50, false);
                        ctx.stroke();
                    }
                }
            }
        }

        Item {
            anchors.fill: parent

            Repeater {
                model: proximityValues.rgRotationValues.length

                QGCLabel {
                    x:                      (_sectorViewEllipsoid.width / 2) - (width / 2)
                    y:                      (_sectorViewEllipsoid.height / 2) - (height / 2)
                    text:                   proximityValues.rgRotationValueStrings[index]
                    font.bold:              true
                    visible:                _root._shouldShow(proximityValues.rgRotationValues[index])

                    transform: Translate {
                        x: Math.cos(-Math.PI / 2 + Math.PI / 4 * index)
                           * (Math.min(proximityValues.rgRotationValues[index], _root.range) * _root._ratio)
                        y: Math.sin(-Math.PI / 2 + Math.PI / 4 * index)
                           * (Math.min(proximityValues.rgRotationValues[index], _root.range) * _root._ratio)
                    }
                }
            }
            transform: Scale {
                origin.x:       _sectorViewEllipsoid.width / 2
                origin.y:       _sectorViewEllipsoid.height / 2
                xScale:         _root.width / _root._minlength
                yScale:         radarHost.height / _root._minlength
            }
        }
    }
}
