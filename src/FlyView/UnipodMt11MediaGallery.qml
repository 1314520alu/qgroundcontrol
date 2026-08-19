import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

import QGroundControl
import QGroundControl.Controls

/// Landscape media gallery overlay for UniPod MT11 onboard TF media.
QGCPopupDialog {
    id: root
    title: qsTr("Media Library")
    buttons: Dialog.Close

    property var mediaClient: null
    property var _videoManager: QGroundControl.videoManager
    property var _selectedFile: null
    property bool _liveStoppedForPlayback: false
    property bool _showingPreview: false
    property real _margins: ScreenTools.defaultFontPixelHeight / 2
    property real _cellSize: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelWidth * 8)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function _stopPreviewPlayback() {
        mediaPlayer.stop()
        mediaPlayer.source = ""
        if (_liveStoppedForPlayback && _videoManager) {
            _videoManager.startVideo()
            _liveStoppedForPlayback = false
        }
    }

    function _selectFile(file) {
        _selectedFile = file
        _showingPreview = true
        if (!file) {
            return
        }
        if (file.isVideo) {
            if (_videoManager && !_liveStoppedForPlayback) {
                _videoManager.stopVideo()
                _liveStoppedForPlayback = true
            }
            mediaPlayer.source = file.url
            mediaPlayer.play()
        } else {
            _stopPreviewPlayback()
        }
    }

    function _backToGrid() {
        _stopPreviewPlayback()
        _showingPreview = false
        _selectedFile = null
    }

    onClosed: {
        _stopPreviewPlayback()
        if (mediaClient && mediaClient.downloading) {
            mediaClient.cancelDownload()
        }
    }

    Component.onCompleted: {
        if (mediaClient && mediaClient.ready) {
            mediaClient.refresh(0)
        }
    }

    Connections {
        target: mediaClient
        function onDownloadFinished(success, localPath, error) {
            if (success) {
                QGroundControl.showMessageDialog(root, qsTr("Download"), qsTr("Saved to %1").arg(localPath))
            } else if (error) {
                QGroundControl.showMessageDialog(root, qsTr("Download"), error)
            }
        }
    }

    ColumnLayout {
        width: Math.min(root.maxContentAvailableWidth, ScreenTools.defaultFontPixelWidth * 90)
        height: Math.min(root.maxContentAvailableHeight, ScreenTools.defaultFontPixelHeight * 22)
        spacing: _margins

        RowLayout {
            Layout.fillWidth: true
            spacing: _margins
            visible: !_showingPreview

            QGCButton {
                text: qsTr("Photo")
                checked: mediaClient && mediaClient.mediaType === 0
                checkable: true
                enabled: mediaClient && mediaClient.ready && !mediaClient.loading
                onClicked: {
                    if (mediaClient) {
                        mediaClient.refresh(0)
                    }
                }
            }

            QGCButton {
                text: qsTr("Video")
                checked: mediaClient && mediaClient.mediaType === 1
                checkable: true
                enabled: mediaClient && mediaClient.ready && !mediaClient.loading
                onClicked: {
                    if (mediaClient) {
                        mediaClient.refresh(1)
                    }
                }
            }

            Item { Layout.fillWidth: true }

            QGCButton {
                text: qsTr("Retry")
                visible: mediaClient && mediaClient.errorString.length > 0
                onClicked: {
                    if (mediaClient) {
                        mediaClient.refresh(mediaClient.mediaType)
                    }
                }
            }
        }

        // Grid browse
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !_showingPreview

            GridView {
                id: gridView
                anchors.fill: parent
                clip: true
                cellWidth: _cellSize + _margins
                cellHeight: _cellSize + ScreenTools.defaultFontPixelHeight + _margins
                model: mediaClient ? mediaClient.files : null
                visible: mediaClient && !mediaClient.loading && mediaClient.errorString.length === 0
                         && mediaClient.files && mediaClient.files.count > 0

                delegate: Item {
                    width: gridView.cellWidth
                    height: gridView.cellHeight

                    property var file: object

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: _margins / 2
                        spacing: 2

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: qgcPal.windowShade
                            radius: 4
                            clip: true

                            Image {
                                anchors.fill: parent
                                visible: file && !file.isVideo
                                source: file && !file.isVideo ? file.url : ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                sourceSize.width: _cellSize
                                sourceSize.height: _cellSize
                            }

                            QGCColoredImage {
                                anchors.centerIn: parent
                                visible: file && file.isVideo
                                width: parent.width * 0.4
                                height: width
                                source: "/qmlimages/camera_video.svg"
                                color: qgcPal.text
                                fillMode: Image.PreserveAspectFit
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (file) {
                                        _selectFile(file)
                                    }
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: file ? file.name : ""
                            elide: Text.ElideMiddle
                            font.pointSize: ScreenTools.smallFontPointSize
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                onAtYEndChanged: {
                    if (atYEnd && mediaClient && mediaClient.hasMore && !mediaClient.loading) {
                        mediaClient.loadMore()
                    }
                }
            }

            QGCLabel {
                anchors.centerIn: parent
                text: qsTr("Loading…")
                visible: mediaClient && mediaClient.loading && (!mediaClient.files || mediaClient.files.count === 0)
            }

            QGCLabel {
                anchors.centerIn: parent
                width: parent.width * 0.8
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                text: mediaClient ? mediaClient.errorString : ""
                visible: mediaClient && mediaClient.errorString.length > 0 && !mediaClient.loading
            }

            QGCLabel {
                anchors.centerIn: parent
                text: qsTr("No media files")
                visible: mediaClient && !mediaClient.loading && mediaClient.errorString.length === 0
                         && mediaClient.files && mediaClient.files.count === 0
            }
        }

        // Preview pane
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: _margins
            visible: _showingPreview && _selectedFile

            RowLayout {
                Layout.fillWidth: true
                spacing: _margins

                QGCButton {
                    text: qsTr("Back")
                    onClicked: _backToGrid()
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: _selectedFile ? _selectedFile.name : ""
                    elide: Text.ElideMiddle
                }

                QGCButton {
                    text: mediaClient && mediaClient.downloading ? qsTr("Cancel") : qsTr("Download")
                    enabled: _selectedFile && mediaClient
                    onClicked: {
                        if (!mediaClient || !_selectedFile) {
                            return
                        }
                        if (mediaClient.downloading) {
                            mediaClient.cancelDownload()
                        } else {
                            mediaClient.download(_selectedFile)
                        }
                    }
                }
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: mediaClient ? mediaClient.downloadProgress : 0
                visible: mediaClient && mediaClient.downloading
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Image {
                    anchors.fill: parent
                    visible: _selectedFile && !_selectedFile.isVideo
                    source: _selectedFile && !_selectedFile.isVideo ? _selectedFile.url : ""
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                }

                VideoOutput {
                    id: previewVideo
                    anchors.fill: parent
                    visible: _selectedFile && _selectedFile.isVideo
                    fillMode: VideoOutput.PreserveAspectFit
                }

                MediaPlayer {
                    id: mediaPlayer
                    videoOutput: previewVideo
                    audioOutput: AudioOutput {}
                    onErrorOccurred: function(error, errorString) {
                        QGroundControl.showMessageDialog(
                                    root,
                                    qsTr("Playback"),
                                    qsTr("Unable to play video. You can download it instead.\n%1").arg(errorString))
                    }
                }
            }
        }
    }
}
