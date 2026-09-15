import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property url iconSource: ""
    property string label: ""
    property bool selected: false
    property bool emphasized: false
    /// Recording-in-progress: icon + label + ring turn red
    property bool recording: false
    /// Active laser / similar: icon + label + ring turn blue
    property bool accentBlue: false
    // Parent bars set this so labels never collide on short landscape
    property real circleSize: 0

    signal clicked()
    signal pressed()
    signal released()

    readonly property real _circleSize: circleSize > 0
        ? circleSize
        : Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 4.0)

    readonly property color _accentRed: qgcPal.colorRed
    readonly property color _accentBlue: qgcPal.colorBlue
    readonly property color _glyphColor: {
        if (!root.enabled) {
            return Qt.rgba(1, 1, 1, 0.45)
        }
        if (root.recording) {
            return root._accentRed
        }
        if (root.accentBlue) {
            return root._accentBlue
        }
        return "#FFFFFF"
    }
    readonly property color _labelColor: {
        if (!root.enabled) {
            return Qt.rgba(1, 1, 1, 0.5)
        }
        if (root.recording) {
            return root._accentRed
        }
        if (root.accentBlue) {
            return root._accentBlue
        }
        return "#FFFFFF"
    }

    // Size from content; parent may widen us so the circle/label stay centered in the bar
    implicitWidth: buttonCol.width
    width: implicitWidth
    height: buttonCol.height

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    function triggerPhotoFlash() {
        photoFlashAnim.restart()
    }

    Column {
        id: buttonCol
        width: Math.max(root._circleSize, labelItem.implicitWidth)
        x: root.width > width ? (root.width - width) / 2 : 0
        spacing: Math.max(2, ScreenTools.defaultFontPixelHeight * 0.1)

        Rectangle {
            id: circle
            width: root._circleSize
            height: width
            radius: width * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            scale: pressArea.pressed ? 0.88 : 1.0
            color: {
                if (photoFlashAnim.running) {
                    return Qt.rgba(1, 1, 1, 0.55 + 0.35 * photoFlashAnim.opacityValue)
                }
                if (root.recording) {
                    return Qt.rgba(root._accentRed.r, root._accentRed.g, root._accentRed.b, 0.35)
                }
                if (root.accentBlue) {
                    return Qt.rgba(root._accentBlue.r, root._accentBlue.g, root._accentBlue.b, 0.35)
                }
                return Qt.rgba(0, 0, 0, root.emphasized ? 0.72 : 0.58)
            }
            border.width: (pressArea.pressed || root.selected || root.recording || root.accentBlue) ? 2 : 1
            border.color: {
                if (root.recording) {
                    return root._accentRed
                }
                if (root.accentBlue) {
                    return root._accentBlue
                }
                if (pressArea.pressed || root.selected) {
                    return qgcPal.buttonHighlight
                }
                return Qt.rgba(1, 1, 1, 0.55)
            }

            Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }

            QGCColoredImage {
                anchors.centerIn: parent
                width: parent.width * (photoFlashAnim.running ? 0.42 : 0.52)
                height: width
                source: root.iconSource
                sourceSize.height: height
                color: root._glyphColor
                fillMode: Image.PreserveAspectFit
                visible: root.iconSource !== ""

                Behavior on width { NumberAnimation { duration: 90 } }
                Behavior on height { NumberAnimation { duration: 90 } }
            }
        }

        QGCLabel {
            id: labelItem
            width: parent.width
            text: root.label
            color: root._labelColor
            font.pointSize: ScreenTools.smallFontPointSize
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            opacity: 1
        }
    }

    SequentialAnimation {
        id: photoFlashAnim
        property real opacityValue: 0
        NumberAnimation {
            target: photoFlashAnim
            property: "opacityValue"
            from: 1
            to: 0
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        enabled: root.enabled
        preventStealing: true
        onPressed: root.pressed()
        onReleased: root.released()
        onCanceled: root.released()
        onClicked: root.clicked()
    }
}
