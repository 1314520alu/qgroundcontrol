import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

QGCPopupDialog {
    id: root

    title:          qsTr("New Version Available")
    buttons:        0
    destroyOnClose: true

    readonly property var   _updateChecker:     QGroundControl.updateChecker
    readonly property real  _dialogWidth:       Math.min(mainWindow.width * 0.7, ScreenTools.defaultFontPixelWidth * 60)
    readonly property real  _notesMaxHeight:    ScreenTools.defaultFontPixelHeight * 6
    readonly property real  _buttonMinHeight:   Math.max(ScreenTools.implicitButtonHeight, ScreenTools.minTouchPixels)
    readonly property bool  _downloading:       _updateChecker.downloading
    readonly property bool  _hasInstaller:      _updateChecker.localInstallerPath !== ""
    readonly property bool  _hasError:          _updateChecker.errorString !== ""
    readonly property bool  _showDownload:      !_downloading && !_hasInstaller && _updateChecker.downloadAvailable && !_hasError
    readonly property bool  _showRetry:         !_downloading && !_hasInstaller && _hasError && _updateChecker.downloadAvailable
    readonly property bool  _showInstall:       !_downloading && _hasInstaller
    readonly property bool  _showLater:         !_downloading

    function _dismissAndClose() {
        _updateChecker.dismiss()
        close()
    }

    onClosed: {
        if (_updateChecker.dialogVisible) {
            _updateChecker.dismiss()
        }
    }

    onVisibleChanged: {
        if (visible) {
            closePolicy = Popup.CloseOnEscape
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        spacing: ScreenTools.defaultDialogControlSpacing

        QGCLabel {
            Layout.preferredWidth:  _dialogWidth
            wrapMode:               Text.WordWrap
            text:                   _updateChecker.remoteVersion
        }

        QGCFlickable {
            Layout.preferredWidth:  _dialogWidth
            Layout.preferredHeight: Math.min(notesLabel.implicitHeight, _notesMaxHeight)
            Layout.maximumHeight:   _notesMaxHeight
            contentWidth:           width
            contentHeight:          notesLabel.height
            visible:                notesLabel.text !== ""

            QGCLabel {
                id:         notesLabel
                width:      parent.width
                wrapMode:   Text.WordWrap
                text:       _updateChecker.notes
            }
        }

        QGCLabel {
            Layout.preferredWidth:  _dialogWidth
            wrapMode:               Text.WordWrap
            visible:                _hasError
            color:                  qgcPal.warningText
            text:                   _updateChecker.errorString
        }

        ProgressBar {
            Layout.preferredWidth:  _dialogWidth
            from:                   0
            to:                     1
            value:                  _updateChecker.downloadProgress
            indeterminate:          !_updateChecker.downloadProgressKnown
            visible:                _downloading
        }

        RowLayout {
            Layout.preferredWidth:  _dialogWidth
            spacing:                ScreenTools.defaultFontPixelWidth

            QGCButton {
                Layout.fillWidth:       true
                Layout.minimumWidth:    height * 1.5
                Layout.minimumHeight:   _buttonMinHeight
                text:                   qsTr("Later")
                visible:                _showLater
                onClicked:              root._dismissAndClose()
            }

            QGCButton {
                Layout.fillWidth:       true
                Layout.minimumWidth:    height * 1.5
                Layout.minimumHeight:   _buttonMinHeight
                primary:                true
                text:                   qsTr("Download")
                visible:                _showDownload
                onClicked:              _updateChecker.download()
            }

            QGCButton {
                Layout.fillWidth:       true
                Layout.minimumWidth:    height * 1.5
                Layout.minimumHeight:   _buttonMinHeight
                primary:                true
                text:                   qsTr("Retry")
                visible:                _showRetry
                onClicked:              _updateChecker.download()
            }

            QGCButton {
                Layout.fillWidth:       true
                Layout.minimumWidth:    height * 1.5
                Layout.minimumHeight:   _buttonMinHeight
                primary:                true
                text:                   qsTr("Install")
                visible:                _showInstall
                onClicked:              _updateChecker.openInstaller()
            }

            QGCButton {
                Layout.fillWidth:       true
                Layout.minimumWidth:    height * 1.5
                Layout.minimumHeight:   _buttonMinHeight
                text:                   qsTr("Cancel")
                visible:                _downloading
                onClicked:              _updateChecker.cancelDownload()
            }
        }
    }
}
