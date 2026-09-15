import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Button {
    id:             control
    padding:        ScreenTools.defaultFontPixelWidth * 0.75
    hoverEnabled:   !ScreenTools.isMobile
    autoExclusive:  true
    icon.color:     textColor

    property color textColor: checked || pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
    property bool expandable: false
    property bool expanded:   false
    // Brand rasters (e.g. Miduo PNG) keep original colors. Monochrome PNG glyphs still go
    // through QGCColoredImage — otherwise they vanish on the dark Configure sidebar.
    property bool logo: false
    property bool _iconIsRaster: {
        var src = String(control.icon.source)
        return src.indexOf(".png") !== -1 || src.indexOf(".jpg") !== -1
               || src.indexOf(".jpeg") !== -1 || src.indexOf(".webp") !== -1
    }
    property bool _keepOriginalColors: logo && _iconIsRaster

    signal toggleExpand()

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  control.enabled
    }

    background: Rectangle {
        color:      qgcPal.buttonHighlight
        opacity:    checked || pressed ? 1 : enabled && hovered ? .2 : 0
        radius:     ScreenTools.defaultFontPixelWidth / 2
    }

    contentItem: RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        Image {
            visible:               control._keepOriginalColors
            source:                visible ? control.icon.source : ""
            Layout.preferredWidth: ScreenTools.defaultFontPixelHeight
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight
            Layout.maximumWidth:   ScreenTools.defaultFontPixelHeight
            Layout.maximumHeight:  ScreenTools.defaultFontPixelHeight
            sourceSize.width:      ScreenTools.defaultFontPixelHeight * 2
            sourceSize.height:     ScreenTools.defaultFontPixelHeight * 2
            fillMode:              Image.PreserveAspectFit
            mipmap:                true
            asynchronous:          true
        }

        QGCColoredImage {
            visible:               !control._keepOriginalColors
            source:                visible ? control.icon.source : ""
            color:                 control.icon.color
            fillMode:              Image.PreserveAspectFit
            Layout.preferredWidth: ScreenTools.defaultFontPixelHeight
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight
            Layout.maximumWidth:   ScreenTools.defaultFontPixelHeight
            Layout.maximumHeight:  ScreenTools.defaultFontPixelHeight
            width:                 ScreenTools.defaultFontPixelHeight
            height:                ScreenTools.defaultFontPixelHeight
        }

        QGCLabel {
            id:                     displayText
            Layout.fillWidth:       true
            text:                   control.text
            color:                  control.textColor
            horizontalAlignment:    QGCLabel.AlignLeft
            elide:                  Text.ElideRight
        }

        QGCColoredImage {
            visible:    control.expandable
            source:     "/InstrumentValueIcons/cheveron-right.svg"
            color:      control.textColor
            width:      ScreenTools.defaultFontPixelHeight * 0.75
            height:     width
            rotation:   control.expanded ? 90 : 0

            MouseArea {
                anchors.fill: parent
                anchors.margins: -ScreenTools.defaultFontPixelWidth
                onClicked: control.toggleExpand()
            }
        }
    }
}
