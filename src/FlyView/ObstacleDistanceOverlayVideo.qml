import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    anchors.fill: parent
    property var showText: obstacleDistance._showText
    // Video HUD: only show sectors closer than this (meters). Far hits like max-range stay hidden.
    readonly property real _maxDisplayDistanceM: 15

    function drawSegment(ctx, range, centerX, centerY, lengthFrom, lengthTo, radFrom, radTo, grad) {
        // Qt expects the angles to be in respect of the X axis, The Incoming Angles are in FRD From Front goind Clockwise
        // Transform coordinates to Qt XY
        radFrom -= Math.PI / 2
        radTo -= Math.PI / 2
        const topSrcX = centerX + lengthFrom * Math.cos(radFrom)
        const topSrcY = centerY + lengthFrom * Math.sin(radFrom)
        const topDstX = centerX + lengthFrom * Math.cos(radTo)
        const topDstY = centerY + lengthFrom * Math.sin(radTo)

        ctx.save()
        ctx.setTransform(2, 0, 0, 1, -centerX, 0)
        ctx.beginPath()
        ctx.fillStyle = grad
        ctx.moveTo(topSrcX, topSrcY)
        ctx.arc(centerX, centerY, lengthFrom, radFrom, radTo, false)
        ctx.arc(centerX, centerY, lengthTo, radTo, radFrom, true)
        ctx.lineTo(topSrcX, topSrcY)
        ctx.fill()
        ctx.closePath()
        ctx.restore()

        if (range > 0) {
            ctx.save()
            ctx.setTransform(2, 0, 0, 2, -centerX, 0)
            ctx.beginPath()
            ctx.strokeStyle = Qt.rgba(0, 0, 0, 0.8)
            ctx.font = "bold 10px sans-serif"
            ctx.lineWidth = 2
            ctx.fillStyle = Qt.rgba(1, 1, 1, 0.8)
            const text = obstacleDistance._rangeToShow(range)
            const textX = topSrcX + (topDstX - topSrcX) / 2
            const textY = topSrcY + (topDstY - topSrcY) / 2
            ctx.strokeText(text, textX, textY / 2)
            ctx.fillText(text, textX, textY / 2)
            ctx.closePath()
            ctx.restore()
        }
    }

    function paintObstacleOverlay(ctx) {
        const topSafe = ScreenTools.toolbarHeight + ScreenTools.defaultFontPixelHeight * 2
        const bottomSafe = ScreenTools.defaultFontPixelHeight * 2.5
        const safeHeight = Math.max(1, root.height - topSafe - bottomSafe)
        const centerX = root.width / 2
        const centerY = topSafe + safeHeight / 2
        const maxRadiusPixels = 0.9 * safeHeight / 2
        const minRadiusPixels = maxRadiusPixels * 0.2
        const segmentHeightPixels = minRadiusPixels / 8
        const levelMeters = 5
        const displayMax = root._maxDisplayDistanceM
        const levelNum = Math.max(1, displayMax / levelMeters)

        var grad = ctx.createRadialGradient(centerX, centerY, maxRadiusPixels - segmentHeightPixels * levelNum * 2, centerX, centerY, maxRadiusPixels)
        grad.addColorStop(0, Qt.rgba(1, 0, 0, 0.9))
        grad.addColorStop(0.1, Qt.rgba(1, 0, 0, 0.3))
        grad.addColorStop(0.5, Qt.rgba(1, 0.64, 0, 0.3))
        grad.addColorStop(0.65, Qt.rgba(1, 0.64, 0, 0.2))
        grad.addColorStop(0.95, Qt.rgba(0, 1, 0, 0.1))
        grad.addColorStop(1, Qt.rgba(0, 1, 0, 0))

        const segNum = 16
        const incDeg = 360 / segNum
        for (var s = 0; s < segNum; ++s) {
            const deg = s * 360.0 / segNum
            const rad = deg * Math.PI / 180.0
            const i = obstacleDistance._degToRangeIdx(deg, false)
            const iNext = obstacleDistance._degToRangeIdx(deg + incDeg, false)
            var rangeMin = obstacleDistance._maxRadiusMeters

            const end = i < iNext ? iNext : obstacleDistance._rangesLen + iNext
            for (var ii = i; ii < end; ++ii) {
                const r = obstacleDistance._ranges[ii % obstacleDistance._rangesLen] / 100
                if (r < rangeMin)
                    rangeMin = r
            }

            // Only show obstacles closer than 15 m on video.
            if (!(rangeMin > 0 && rangeMin < displayMax)) {
                continue
            }

            const lengthFrom = maxRadiusPixels
            const radFrom = rad
            const radTo = radFrom + incDeg * Math.PI / 180.0 - 0.03
            var range = displayMax
            for (var jj = 0; jj < levelNum; ++jj) {
                const from = lengthFrom - jj * segmentHeightPixels * 2
                const to = from - segmentHeightPixels
                const rangeInLevel = displayMax - (jj + 1) * levelMeters
                const isLast = jj >= levelNum - 1
                if (rangeMin > rangeInLevel || isLast)
                    range = rangeMin
                const rangeToShow = showText ? range : 0
                drawSegment(ctx, rangeToShow, centerX, centerY, from, to, radFrom, radTo, grad)
                if (range == rangeMin)
                    break
            }
        }
    }

    ObstacleDistanceOverlay {
        id: obstacleDistance
    }
}
