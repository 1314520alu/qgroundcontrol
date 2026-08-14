import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

TextArea {
    id:                     messageText
    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 50
    height:                 contentHeight
    readOnly:               true
    textFormat:             TextEdit.RichText
    color:                  qgcPal.text
    placeholderText:        qsTr("No Messages")
    placeholderTextColor:   qgcPal.text
    padding:                0
    wrapMode:               TextEdit.Wrap

    property bool noMessages: messageText.length === 0
    property var  qgcPal:     QGroundControl.globalPalette
    property var  _fact:      null

    function _severityFontCss(colorValue) {
        return "color: " + colorValue + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;"
    }

    function _colorizeSeverities(message) {
        message = message.replace(new RegExp("<#E>", "g"), _severityFontCss(qgcPal.colorRed))
        message = message.replace(new RegExp("<#I>", "g"), _severityFontCss(qgcPal.colorOrange))
        message = message.replace(new RegExp("<#N>", "g"), _severityFontCss(qgcPal.text))
        return message
    }

    /// Light zebra: odd rows get a soft windowShade background (even rows stay transparent).
    function _applyZebra(html) {
        const parts = html.split(/<br\s*\/?>/i).filter(part => part.trim().length > 0)
        let result = ""
        for (let i = 0; i < parts.length; i++) {
            const bg = (i % 2 === 1) ? qgcPal.windowShade : "transparent"
            result += "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"4\"><tr bgcolor=\""
                    + bg + "\"><td style=\"padding: 2px 4px;\">" + parts[i] + "</td></tr></table>"
        }
        return result
    }

    function formatMessage(message) {
        return _applyZebra(_colorizeSeverities(message))
    }

    function _reloadAllMessages() {
        if (!_activeVehicle) {
            messageText.text = ""
            return
        }
        messageText.text = formatMessage(_activeVehicle.formattedMessages)
    }

    Component.onCompleted: {
        _reloadAllMessages()
        if (_activeVehicle) {
            _activeVehicle.resetAllMessages()
        }
    }

    Connections {
        target: _activeVehicle
        // Rebuild so zebra odd/even stays correct after prepend
        function onNewFormattedMessage(formattedMessage) { messageText._reloadAllMessages() }
    }

    FactPanelController {
        id: controller
    }

    onLinkActivated: (link) => {
        if (link.startsWith('param://')) {
            var paramName = link.substr(8);
            _fact = controller.getParameterFact(-1, paramName, true)
            if (_fact != null) {
                paramEditorDialogFactory.open()
            }
        } else {
            Qt.openUrlExternally(link);
        }
    }

    QGCPopupDialogFactory {
        id: paramEditorDialogFactory

        dialogComponent: paramEditorDialogComponent
    }

    Component {
        id: paramEditorDialogComponent

        ParameterEditorDialog {
            title:          qsTr("Edit Parameter")
            fact:           messageText._fact
            destroyOnClose: true
        }
    }

    Rectangle {
        anchors.right:   parent.right
        anchors.top:     parent.top
        width:                      ScreenTools.defaultFontPixelHeight * 1.25
        height:                     width
        radius:                     width / 2
        color:                      QGroundControl.globalPalette.button
        border.color:               QGroundControl.globalPalette.buttonText
        visible:                    !noMessages

        QGCColoredImage {
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.25
            anchors.centerIn:   parent
            anchors.fill:       parent
            sourceSize.height:  height
            source:             "/res/TrashDelete.svg"
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            color:              qgcPal.text
        }

        QGCMouseArea {
            fillItem: parent
            onClicked: {
                _activeVehicle.clearMessages()
                messageText.text = ""
                mainWindow.closeIndicatorDrawer()
            }
        }
    }
}
