import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property var motors: []                 ///< QVariantList of maps from MotorComponentController
    property bool spatialLayout: true
    property bool safetyArmed: false        ///< dim when false; do not shadow Item.enabled
    property string selectedLetter: ""
    property var spinningLetters: ({})      ///< { "A": true, ... }
    property int throttlePercent: 10

    signal motorClicked(string letter, int motorIndex)

    readonly property real _cx: width / 2
    readonly property real _cy: height / 2 + ScreenTools.defaultFontPixelHeight * 0.15
    readonly property color _cwColor: "#33cc33"
    readonly property color _ccwColor: "#00b8e6"
    readonly property color _selColor: "#2563eb"
    readonly property color _frameFill: "#ebebeb"
    readonly property color _frameStroke: "#7a6f96"
    readonly property int _spinDurationMs: {
        const t = Math.max(0, Math.min(30, throttlePercent))
        return t <= 0 ? 900 : Math.max(80, 480 - t * 12)
    }
    // Fit 6 arms: chord ≈ radius, so disc must stay well under ~0.45×radius.
    readonly property real _letterBadge: ScreenTools.defaultFontPixelHeight * 1.25
    readonly property real _nodeSize: {
        const fit = Math.min(width, height)
        const byFont = ScreenTools.defaultFontPixelHeight * 2.9
        const byFit = fit * 0.118
        return Math.max(ScreenTools.defaultFontPixelHeight * 2.4, Math.min(byFont, byFit))
    }
    // Keep arms + outer letters inside the box (no clip of hubs/letters).
    readonly property real _radius: {
        const fit = Math.min(width, height)
        const margin = _letterBadge * 1.35 + ScreenTools.defaultFontPixelHeight * 0.6
        return Math.max(fit * 0.18, fit * 0.5 - margin - _nodeSize * 0.55)
    }
    readonly property real _letterOutset: _nodeSize * 0.55 + _letterBadge * 0.55
    // Tangential split between coaxial pair (must clear two letter badges).
    readonly property real _pairSplit: Math.max(_letterBadge * 0.72, ScreenTools.defaultFontPixelWidth * 1.6)

    /// Hub center: coaxial pair split tangentially so Top is CCW of arm, Bottom is CW
    /// (clockwise reading around the frame is then A then B, not BA).
    function _hubPos(angleDeg, stackLayer) {
        const rad = angleDeg * Math.PI / 180
        let split = 0
        if (stackLayer === "top") {
            split = -_pairSplit * 0.55   // CCW of arm tip
        } else if (stackLayer === "bottom") {
            split = _pairSplit * 0.55    // CW of arm tip
        }
        // Clockwise tangential unit in our coords (x=sin, y=-cos): (cos, sin)
        const ox = Math.cos(rad) * split
        const oy = Math.sin(rad) * split
        return Qt.point(
            _cx + _radius * Math.sin(rad) + ox,
            _cy - _radius * Math.cos(rad) + oy
        )
    }

    /// Letter outside hub; same CCW/CW split so A appears before B clockwise.
    function _letterPos(angleDeg, stackLayer) {
        const rad = angleDeg * Math.PI / 180
        const r = _radius + _letterOutset
        let split = 0
        if (stackLayer === "top") {
            split = -_pairSplit
        } else if (stackLayer === "bottom") {
            split = _pairSplit
        }
        const ox = Math.cos(rad) * split
        const oy = Math.sin(rad) * split
        return Qt.point(
            _cx + r * Math.sin(rad) + ox,
            _cy - r * Math.cos(rad) + oy
        )
    }

    function _dirColor(dir) {
        if (dir === "CW") {
            return _cwColor
        }
        if (dir === "CCW") {
            return _ccwColor
        }
        return qgcPal.text
    }

    function _motorAt(index) {
        if (!motors || index < 0 || index >= motors.length) {
            return null
        }
        return motors[index]
    }

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultFontPixelWidth
        color: qgcPal.window
        border.color: qgcPal.groupBorder
        border.width: 1
        clip: false

        Column {
            z: 2
            anchors.horizontalCenter: parent.horizontalCenter
            y: ScreenTools.defaultFontPixelHeight * 0.25
            spacing: 2
            Canvas {
                width: ScreenTools.defaultFontPixelWidth * 1.6
                height: ScreenTools.defaultFontPixelHeight * 0.85
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.fillStyle = "#ef4444"
                    ctx.beginPath()
                    ctx.moveTo(width / 2, 0)
                    ctx.lineTo(0, height)
                    ctx.lineTo(width, height)
                    ctx.closePath()
                    ctx.fill()
                }
            }
            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("机头")
                font.pointSize: ScreenTools.smallFontPointSize
                font.bold: true
                color: _frameStroke
            }
        }

        Canvas {
            id: frameCanvas
            anchors.fill: parent
            z: 0
            visible: root.spatialLayout
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!root.spatialLayout || !root.motors || root.motors.length === 0) {
                    return
                }

                const tips = {}
                for (let i = 0; i < root.motors.length; ++i) {
                    const m = root.motors[i]
                    const angle = Number(m.angleDeg)
                    const key = String(Math.round(angle * 10) / 10)
                    if (!tips[key]) {
                        const rad = angle * Math.PI / 180
                        tips[key] = {
                            x: root._cx + root._radius * Math.sin(rad),
                            y: root._cy - root._radius * Math.cos(rad)
                        }
                    }
                }

                ctx.lineCap = "round"
                for (const k in tips) {
                    const t = tips[k]
                    ctx.strokeStyle = "#c4bdd4"
                    ctx.lineWidth = ScreenTools.defaultFontPixelWidth * 1.6
                    ctx.beginPath()
                    ctx.moveTo(root._cx, root._cy)
                    ctx.lineTo(t.x, t.y)
                    ctx.stroke()
                    ctx.strokeStyle = root._frameStroke
                    ctx.lineWidth = ScreenTools.defaultFontPixelWidth * 0.6
                    ctx.beginPath()
                    ctx.moveTo(root._cx, root._cy)
                    ctx.lineTo(t.x, t.y)
                    ctx.stroke()
                }

                const body = ScreenTools.defaultFontPixelWidth * 3.4
                ctx.fillStyle = root._frameFill
                ctx.strokeStyle = root._frameStroke
                ctx.lineWidth = ScreenTools.defaultFontPixelWidth * 0.45
                ctx.beginPath()
                ctx.rect(root._cx - body / 2, root._cy - body / 2, body, body)
                ctx.fill()
                ctx.stroke()
            }
            Connections {
                target: root
                function onMotorsChanged() { frameCanvas.requestPaint() }
                function onWidthChanged() { frameCanvas.requestPaint() }
                function onHeightChanged() { frameCanvas.requestPaint() }
                function onSpatialLayoutChanged() { frameCanvas.requestPaint() }
            }
        }

        Grid {
            anchors.centerIn: parent
            z: 1
            visible: !root.spatialLayout
            columns: 4
            spacing: ScreenTools.defaultFontPixelWidth
            Repeater {
                model: root.spatialLayout ? 0 : (root.motors ? root.motors.length : 0)
                delegate: MotorNode {
                    motorIndexInList: index
                    spatial: false
                }
            }
        }

        Repeater {
            model: root.spatialLayout && root.motors ? root.motors.length : 0
            delegate: MotorNode {
                motorIndexInList: index
                spatial: true
            }
        }
    }

    component MotorNode: Item {
        id: node

        property int motorIndexInList: -1
        property bool spatial: true

        readonly property var motor: root._motorAt(motorIndexInList)
        readonly property string letter: motor ? String(motor.letter) : ""
        readonly property int boardIndex: motor ? Number(motor.motorIndex) : -1
        readonly property string dir: motor ? String(motor.dir) : "Unknown"
        readonly property string stackLayer: motor ? String(motor.layer) : "single"
        readonly property real angleDeg: motor ? Number(motor.angleDeg) : 0

        readonly property bool selected: root.selectedLetter.length > 0 && root.selectedLetter === letter
        readonly property bool spinning: letter.length > 0 && !!root.spinningLetters[letter]
        readonly property bool dimmed: root.selectedLetter.length > 0 && !selected
        readonly property color dirColor: root._dirColor(dir)

        width: spatial ? root.width : root._nodeSize
        height: spatial ? root.height : root._nodeSize
        visible: motor !== null
        opacity: !root.safetyArmed ? 0.55 : (dimmed ? 0.38 : 1)
        Behavior on opacity { NumberAnimation { duration: 160 } }
        z: selected ? 30 : (stackLayer === "top" ? 12 : 6)

        readonly property point hub: spatial
            ? root._hubPos(angleDeg, stackLayer)
            : Qt.point(width / 2, height / 2)
        readonly property point letterPt: spatial
            ? root._letterPos(angleDeg, stackLayer)
            : Qt.point(width - root._letterBadge * 0.35, -root._letterBadge * 0.15)

        // Non-spatial grid: size to node only
        Item {
            id: hubItem
            width: root._nodeSize
            height: width
            x: spatial ? hub.x - width / 2 : 0
            y: spatial ? hub.y - height / 2 : 0
            scale: selected ? 1.06 : 1
            Behavior on scale { NumberAnimation { duration: 160 } }

            // Selection glow
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 1.14
                height: width
                radius: width / 2
                color: "transparent"
                border.color: root._selColor
                border.width: selected ? 3 : 0
                opacity: selected ? 0.95 : 0
            }
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 1.02
                height: width
                radius: width / 2
                color: selected ? Qt.rgba(0.145, 0.388, 0.922, 0.22) : "transparent"
            }

            // Disc (slightly smaller than node box)
            Rectangle {
                id: disc
                anchors.centerIn: parent
                width: parent.width * 0.86
                height: width
                radius: width / 2
                color: selected ? "#eef4ff" : "#f3f4f6"
                border.color: node.dirColor
                border.width: 2.5

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: node.dirColor
                    opacity: node.spinning ? Math.min(0.5, 0.18 + root.throttlePercent / 70) : 0
                    Behavior on opacity { NumberAnimation { duration: 120 } }
                }

                Item {
                    id: rotor
                    anchors.centerIn: parent
                    width: parent.width * 0.78
                    height: width

                    property real bladeAngle: 0

                    NumberAnimation on bladeAngle {
                        running: node.spinning
                        from: 0
                        to: node.dir === "CW" ? 360 : -360
                        loops: Animation.Infinite
                        duration: root._spinDurationMs
                    }

                    transform: Rotation {
                        origin.x: rotor.width / 2
                        origin.y: rotor.height / 2
                        angle: rotor.bladeAngle
                    }

                    // Arc arrow showing spin direction (always visible; rotates while spinning)
                    Canvas {
                        id: arrowCanvas
                        anchors.fill: parent
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            if (node.dir === "Unknown") {
                                return
                            }
                            const cw = node.dir === "CW"
                            const cx = width / 2
                            const cy = height / 2
                            const r = width * 0.32
                            const start = cw ? (-0.9 * Math.PI) : (-0.1 * Math.PI)
                            const end = cw ? (0.5 * Math.PI) : (-1.5 * Math.PI)
                            ctx.strokeStyle = node.dirColor
                            ctx.lineWidth = Math.max(2, width * 0.07)
                            ctx.lineCap = "round"
                            ctx.beginPath()
                            ctx.arc(cx, cy, r, start, end, !cw)
                            ctx.stroke()

                            // Arrow head at arc end
                            const tipAng = end
                            const tipX = cx + r * Math.cos(tipAng)
                            const tipY = cy + r * Math.sin(tipAng)
                            const tang = tipAng + (cw ? Math.PI / 2 : -Math.PI / 2)
                            const s = width * 0.1
                            ctx.fillStyle = node.dirColor
                            ctx.beginPath()
                            ctx.moveTo(tipX, tipY)
                            ctx.lineTo(tipX - s * Math.cos(tang - 0.5), tipY - s * Math.sin(tang - 0.5))
                            ctx.lineTo(tipX - s * Math.cos(tang + 0.9), tipY - s * Math.sin(tang + 0.9))
                            ctx.closePath()
                            ctx.fill()
                        }
                        Connections {
                            target: node
                            function onDirChanged() { arrowCanvas.requestPaint() }
                        }
                        Component.onCompleted: requestPaint()
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width * 0.22
                        height: width
                        radius: width / 2
                        color: "#9ca3af"
                        border.color: node.dirColor
                        border.width: 1
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width * 0.4
                            height: width
                            radius: width / 2
                            color: "#e5e7eb"
                        }
                    }
                }
            }

            // 上 above top motor / 下 below bottom motor
            QGCLabel {
                anchors.horizontalCenter: hubItem.horizontalCenter
                anchors.bottom: hubItem.top
                anchors.bottomMargin: 1
                visible: node.stackLayer === "top"
                text: qsTr("上")
                color: "#0284c7"
                font.pointSize: ScreenTools.smallFontPointSize
                font.bold: true
            }
            QGCLabel {
                anchors.horizontalCenter: hubItem.horizontalCenter
                anchors.top: hubItem.bottom
                anchors.topMargin: 1
                visible: node.stackLayer === "bottom"
                text: qsTr("下")
                color: "#16a34a"
                font.pointSize: ScreenTools.smallFontPointSize
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (node.letter.length && node.boardIndex > 0) {
                        root.motorClicked(node.letter, node.boardIndex)
                    }
                }
            }
        }

        // Letter badge outside hub (spatial) or overlay (grid)
        Rectangle {
            id: letterBadge
            width: root._letterBadge
            height: width
            radius: width / 2
            x: spatial ? letterPt.x - width / 2 : hubItem.width - width * 0.35
            y: spatial ? letterPt.y - height / 2 : -height * 0.15
            color: node.selected ? root._selColor : "#ffffff"
            border.color: node.selected ? "#1d4ed8" : "#e11d48"
            border.width: 2
            z: 40
            QGCLabel {
                anchors.centerIn: parent
                text: node.letter
                color: node.selected ? "#ffffff" : "#e11d48"
                font.bold: true
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (node.letter.length && node.boardIndex > 0) {
                        root.motorClicked(node.letter, node.boardIndex)
                    }
                }
            }
        }
    }
}
