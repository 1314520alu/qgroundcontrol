import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: _root
    heading: qsTr("Links")

    property var _linkManager: QGroundControl.linkManager

    // Remote controller one-tap presets (on-device Android QGC).
    // SIYI UniRC: manuals list 192.168.144.20:19856, but that IP only exists when the
    // radio Ethernet interface is up. On-device the SIYI UDP service listens on *:19856
    // (reachable via 127.0.0.1) — UniGCS uses that path when eth .20 is absent.
    // SIYI MK15/MK32: manuals use Port 19856 + 192.168.144.12
    // Skydroid: two MAVLink paths (chosen at apply time via skydroidUsesDirectRadioEthernetTelemetry):
    // - G20/G16 (ar_net0): listen 14551, peer 127.0.0.1:14552 (on-device UDP bridge).
    // - H30/H16 (eth0 on 192.168.144.x): listen 14550, peer 192.168.144.101:14550.
    readonly property var remotePresets: [
        {
            name:       "UniRC 10 Pro",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  0,
            host:       "127.0.0.1:19856"
        },
        {
            name:       "UniRC 7",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  0,
            host:       "127.0.0.1:19856"
        },
        {
            name:       "MK15",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  19856,
            host:       "192.168.144.12"
        },
        {
            name:       "MK32",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  19856,
            host:       "192.168.144.12"
        },
        {
            name:       "云卓 G20",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  14551,
            host:       "127.0.0.1:14552"
        },
        {
            name:       "云卓 G16",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  14551,
            host:       "127.0.0.1:14552"
        },
        {
            name:       "云卓 H16",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  14551,
            host:       "127.0.0.1:14552"
        },
        {
            name:       "云卓 H30",
            linkType:   LinkConfiguration.TypeUdp,
            localPort:  14550,
            host:       "192.168.144.101:14550"
        }
    ]

    property string detectedRemoteName: ""
    property bool   _didAutoApplyDetected: false

    Settings {
        id:         remotePresetSettings
        category:   "RemoteControllerLinkPresets"
        property string selectedName: ""
        // When true, keep the user's manual tile choice across visits.
        property bool   manualOverride: false
    }

    function _findPresetByName(name) {
        for (var i = 0; i < remotePresets.length; i++) {
            if (remotePresets[i].name === name) {
                return remotePresets[i]
            }
        }
        return null
    }

    function _isSkydroidPreset(preset) {
        return preset && preset.name.indexOf("云卓") === 0
    }

    function _skydroidTelemetryParams() {
        if (ScreenToolsController.skydroidUsesDirectRadioEthernetTelemetry()) {
            return { localPort: 14550, host: "192.168.144.101:14550" }
        }
        return { localPort: 14551, host: "127.0.0.1:14552" }
    }

    function _effectivePreset(preset) {
        if (!_isSkydroidPreset(preset)) {
            return preset
        }
        var p = _skydroidTelemetryParams()
        return {
            name:       preset.name,
            linkType:   preset.linkType,
            localPort:  p.localPort,
            host:       p.host,
            extraHosts: preset.extraHosts
        }
    }

    function _findConfigByName(name) {
        var configs = _linkManager.linkConfigurations
        for (var i = 0; i < configs.count; i++) {
            var c = configs.get(i)
            if (c && !c.dynamic && c.name === name) {
                return c
            }
        }
        return null
    }

    function _isRemotePresetName(name) {
        for (var i = 0; i < remotePresets.length; i++) {
            if (remotePresets[i].name === name) {
                return true
            }
        }
        return false
    }

    // Android boots a hidden LinkConfigurationManager (MainWindow) and the Comm Links
    // page creates another. Both auto-apply; removing only the first same-name config
    // leaves a second "UniRC 10 Pro" row above Add New Link.
    function _removeAllConfigsNamed(name) {
        var configs = _linkManager.linkConfigurations
        var toRemove = []
        for (var i = 0; i < configs.count; i++) {
            var c = configs.get(i)
            if (c && !c.dynamic && c.name === name) {
                toRemove.push(c)
            }
        }
        for (var r = 0; r < toRemove.length; r++) {
            _linkManager.removeConfiguration(toRemove[r])
        }
    }

    function _dedupeConfigsNamed(name) {
        var configs = _linkManager.linkConfigurations
        var keep = null
        var toRemove = []
        for (var i = 0; i < configs.count; i++) {
            var c = configs.get(i)
            if (!c || c.dynamic || c.name !== name) {
                continue
            }
            if (!keep) {
                keep = c
            } else if (c.linkActive && !keep.linkActive) {
                toRemove.push(keep)
                keep = c
            } else {
                toRemove.push(c)
            }
        }
        for (var r = 0; r < toRemove.length; r++) {
            _linkManager.removeConfiguration(toRemove[r])
        }
        return keep
    }

    function _hostListContains(config, host) {
        if (!config || !config.hostList) {
            return false
        }
        var list = config.hostList
        for (var i = 0; i < list.length; i++) {
            if (list[i] === host) {
                return true
            }
        }
        return false
    }

    function _presetNeedsRebuild(config, preset) {
        if (config.localPort !== preset.localPort) {
            return true
        }
        if (!_hostListContains(config, preset.host)) {
            return true
        }
        if (!preset.extraHosts) {
            return false
        }
        for (var i = 0; i < preset.extraHosts.length; i++) {
            if (!_hostListContains(config, preset.extraHosts[i])) {
                return true
            }
        }
        return false
    }

    function _addPresetHosts(config, preset) {
        config.addHost(preset.host)
        if (!preset.extraHosts) {
            return
        }
        for (var i = 0; i < preset.extraHosts.length; i++) {
            config.addHost(preset.extraHosts[i])
        }
    }

    function applyRemotePreset(preset, fromManual) {
        preset = _effectivePreset(preset)

        // UniRC / MK presets need the radio ethernet subnet for handbook UDP and RTSP pods.
        if (preset.name.indexOf("UniRC") === 0 || preset.name.indexOf("MK") === 0) {
            ScreenToolsController.ensureSiyiRadioEthernet()
        }

        var configs = _linkManager.linkConfigurations
        var toRemove = []
        for (var i = 0; i < configs.count; i++) {
            var c = configs.get(i)
            if (c && !c.dynamic && _isRemotePresetName(c.name) && c.name !== preset.name) {
                toRemove.push(c)
            }
        }
        for (var r = 0; r < toRemove.length; r++) {
            _linkManager.removeConfiguration(toRemove[r])
        }

        // Boot (hidden MainWindow instance) and opening this page both auto-apply.
        // Tearing down an already-correct UDP link races a second bind on the same
        // port; QGC then treats the duplicate socket as a flapping secondary link.
        var existing = _dedupeConfigsNamed(preset.name)
        if (existing && !_presetNeedsRebuild(existing, preset)) {
            remotePresetSettings.selectedName = preset.name
            if (fromManual) {
                remotePresetSettings.manualOverride = (detectedRemoteName === "" || preset.name !== detectedRemoteName)
            }
            if (!existing.linkActive) {
                _linkManager.createConnectedLink(existing)
            }
            _linkManager.syncUdpAutoConnectLink()
            ScreenTools.applyRemoteUiScaleForPreset(preset.name)
            return
        }

        _removeAllConfigsNamed(preset.name)

        var config = _linkManager.createConfiguration(preset.linkType, preset.name)
        config.dynamic = false
        // autoConnect must be set BEFORE localPort/host: UDPConfiguration::setAutoConnect(true)
        // overwrites localPort with AutoConnectSettings.udpListenPort (default 14550).
        config.autoConnect = true
        config.localPort = preset.localPort
        _addPresetHosts(config, preset)
        _linkManager.endCreateConfiguration(config)
        _dedupeConfigsNamed(preset.name)

        remotePresetSettings.selectedName = preset.name
        if (fromManual) {
            // Picking the auto-detected model clears override so detection stays in charge.
            remotePresetSettings.manualOverride = (detectedRemoteName === "" || preset.name !== detectedRemoteName)
        }

        if (config && !config.linkActive) {
            _linkManager.createConnectedLink(config)
        }

        // Default UDP AutoConnect (14550) would attach the same vehicle as a flaky secondary.
        _linkManager.syncUdpAutoConnectLink()
        ScreenTools.applyRemoteUiScaleForPreset(preset.name)
    }

    function _autoDetectAndApply() {
        if (!ScreenTools.isAndroid) {
            return
        }

        detectedRemoteName = ScreenToolsController.detectRemoteControllerPreset()
        if (detectedRemoteName === "") {
            return
        }

        // Manual tile choice wins until user selects the detected model again.
        if (remotePresetSettings.manualOverride && remotePresetSettings.selectedName !== detectedRemoteName) {
            return
        }

        var preset = _findPresetByName(detectedRemoteName)
        if (!preset) {
            return
        }

        remotePresetSettings.manualOverride = false
        applyRemotePreset(preset, false)
        _didAutoApplyDetected = true
    }

    Component.onCompleted: {
        _autoDetectAndApply()
        if (remotePresetSettings.selectedName !== "") {
            return
        }
        for (var i = 0; i < remotePresets.length; i++) {
            if (_findConfigByName(remotePresets[i].name)) {
                remotePresetSettings.selectedName = remotePresets[i].name
                break
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelHeight / 2

        QGCLabel {
            text: qsTr("Remote Controllers")
            font.bold: true
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            font.pointSize:     ScreenTools.smallFontPointSize
            visible:            detectedRemoteName !== ""
            text:               qsTr("Detected: %1").arg(detectedRemoteName)
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            font.pointSize:     ScreenTools.smallFontPointSize
            text:               qsTr("Tap a remote to auto-configure and connect. Selection is saved until you choose another.")
        }

        Flow {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth

            Repeater {
                model: remotePresets

                Rectangle {
                    id:     presetTile
                    width:  Math.max(ScreenTools.minTouchPixels * 1.1, ScreenTools.defaultFontPixelWidth * 7)
                    height: width
                    radius: ScreenTools.defaultFontPixelHeight / 8
                    color:  selected ? QGroundControl.globalPalette.buttonHighlight : QGroundControl.globalPalette.button
                    border.width: detected ? 2 : 1
                    border.color: selected ? QGroundControl.globalPalette.buttonHighlight
                                           : (detected ? QGroundControl.globalPalette.colorGreen
                                                       : QGroundControl.globalPalette.groupBorder)

                    readonly property bool selected: remotePresetSettings.selectedName === modelData.name
                    readonly property bool detected: _root.detectedRemoteName === modelData.name

                    QGCLabel {
                        anchors.centerIn:   parent
                        width:              parent.width - ScreenTools.defaultFontPixelWidth
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode:           Text.WordWrap
                        text:               modelData.name
                        color:              presetTile.selected ? QGroundControl.globalPalette.buttonHighlightText : QGroundControl.globalPalette.buttonText
                    }

                    QGCMouseArea {
                        fillItem:   parent
                        onClicked:  _root.applyRemotePreset(modelData, true)
                    }
                }
            }
        }
    }

    Repeater {
        model: _linkManager.linkConfigurations

        RowLayout {
            Layout.fillWidth:   true
            visible:            !object.dynamic

            QGCLabel {
                Layout.fillWidth:   true
                text:               object.name
            }
            QGCColoredImage {
                height:                 ScreenTools.minTouchPixels
                width:                  height
                sourceSize.height:      height
                fillMode:               Image.PreserveAspectFit
                mipmap:                 true
                smooth:                 true
                color:                  qgcPalEdit.text
                source:                 "/res/pencil.svg"
                enabled:                !object.link

                QGCPalette {
                    id: qgcPalEdit
                    colorGroupEnabled: parent.enabled
                }

                QGCMouseArea {
                    fillItem: parent
                    onClicked: {
                        var editingConfig = _linkManager.startConfigurationEditing(object)
                        linkDialogFactory.open({ editingConfig: editingConfig, originalConfig: object })
                    }
                }
            }
            QGCColoredImage {
                height:                 ScreenTools.minTouchPixels
                width:                  height
                sourceSize.height:      height
                fillMode:               Image.PreserveAspectFit
                mipmap:                 true
                smooth:                 true
                color:                  qgcPalDelete.text
                source:                 "/res/TrashDelete.svg"

                QGCPalette {
                    id: qgcPalDelete
                    colorGroupEnabled: parent.enabled
                }

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  QGroundControl.showMessageDialog(
                                    _root,
                                    qsTr("Delete Link"),
                                    qsTr("Are you sure you want to delete '%1'?").arg(object.name),
                                    Dialog.Ok | Dialog.Cancel,
                                    function () {
                                        _linkManager.removeConfiguration(object)
                                    })
                }
            }
            QGCButton {
                text:       object.linkActive ? qsTr("Disconnect") : qsTr("Connect")
                onClicked: {
                    if (object.linkActive) {
                        _linkManager.disconnectLinkConfiguration(object)
                    } else {
                        _linkManager.createConnectedLink(object)
                    }
                }
            }
        }
    }

    LabelledButton {
        label:      qsTr("Add New Link")
        buttonText: qsTr("Add")

        onClicked: {
            var editingConfig = _linkManager.createConfiguration(ScreenTools.isSerialAvailable ? LinkConfiguration.TypeSerial : LinkConfiguration.TypeUdp, "")
            linkDialogFactory.open({ editingConfig: editingConfig, originalConfig: null })
        }
    }

    QGCPopupDialogFactory {
        id: linkDialogFactory

        dialogComponent: linkDialogComponent
    }

    Component {
        id: linkDialogComponent

        QGCPopupDialog {
            title:                  originalConfig ? qsTr("Edit Link") : qsTr("Add New Link")
            buttons:                Dialog.Save | Dialog.Cancel
            acceptButtonEnabled:    nameField.text !== ""

            property var originalConfig
            property var editingConfig

            onAccepted: {
                linkSettingsLoader.item.saveSettings()
                editingConfig.name = nameField.text
                if (originalConfig) {
                    _linkManager.endConfigurationEditing(originalConfig, editingConfig)
                } else {
                    editingConfig.dynamic = false
                    _linkManager.endCreateConfiguration(editingConfig)
                }
            }

            onRejected: _linkManager.cancelConfigurationEditing(editingConfig)

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelWidth

                    QGCLabel { text: qsTr("Name") }
                    QGCTextField {
                        id:                 nameField
                        Layout.fillWidth:   true
                        text:               editingConfig.name
                        placeholderText:    qsTr("Enter name")
                    }
                }

                QGCCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("Automatically Connect on Start")
                    checked:            editingConfig.autoConnect
                    onCheckedChanged:   editingConfig.autoConnect = checked
                }

                QGCCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("High Latency")
                    checked:            editingConfig.highLatency
                    onCheckedChanged:   editingConfig.highLatency = checked
                }

                LabelledComboBox {
                    label:                  qsTr("Type")
                    enabled:                originalConfig == null
                    model:                  _linkManager.linkTypeStrings
                    Component.onCompleted:  comboBox.currentIndex = editingConfig.linkType

                    onActivated: (index) => {
                        if (index !== editingConfig.linkType) {
                            var name = nameField.text
                            editingConfig = _linkManager.createConfiguration(index, name)
                        }
                    }
                }

                Loader {
                    id:     linkSettingsLoader
                    source: editingConfig && editingConfig.settingsURL ? editingConfig.settingsURL : ""
                    asynchronous: true

                    property var subEditConfig:         editingConfig
                    property int _firstColumnWidth:     ScreenTools.defaultFontPixelWidth * 12
                    property int _secondColumnWidth:    ScreenTools.defaultFontPixelWidth * 30
                    property int _rowSpacing:           ScreenTools.defaultFontPixelHeight / 2
                    property int _colSpacing:           ScreenTools.defaultFontPixelWidth / 2

                    onStatusChanged: {
                        if (status === Loader.Error) {
                            console.warn("Failed to load link settings page:", source)
                        }
                    }
                }
            }
        }
    }
}
