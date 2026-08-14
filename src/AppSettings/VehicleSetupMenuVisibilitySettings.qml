import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    id:                 _root
    Layout.fillWidth:   true
    // Use AppSettings context — same as C++ catalog tr() — so Android/desktop qm lookups match.
    heading:            qsTranslate("AppSettings", "Vehicle Setup Menu")
    headingDescription: qsTranslate("AppSettings", "Choose which Vehicle Setup sidebar pages to show. Summary is always visible.")

    property var _appSettings: QGroundControl.settingsManager.appSettings

    // Force checkbox bindings to refresh when the Fact string changes.
    property int _visibilityRevision: 0

    Connections {
        target: _appSettings.vehicleSetupVisibleComponents
        function onRawValueChanged(value) { _root._visibilityRevision++ }
    }

    GridLayout {
        Layout.fillWidth:   true
        columns:            2
        columnSpacing:      ScreenTools.defaultFontPixelWidth
        rowSpacing:         ScreenTools.defaultFontPixelHeight / 4

        Repeater {
            // Labels come from AppSettings::vehicleSetupMenuCatalog() (AppSettings::tr).
            model: _appSettings.vehicleSetupMenuCatalog()

            QGCCheckBoxSlider {
                required property var modelData

                Layout.fillWidth:   true
                text:               modelData.label
                checked: {
                    void _root._visibilityRevision
                    return _appSettings.isVehicleSetupComponentVisible(modelData.id)
                }
                onClicked: _appSettings.setVehicleSetupComponentVisible(modelData.id, !_appSettings.isVehicleSetupComponentVisible(modelData.id))
            }
        }
    }

    QGCButton {
        Layout.fillWidth:   true
        text:               qsTranslate("AppSettings", "Restore Defaults")
        onClicked:          _appSettings.resetVehicleSetupVisibleComponents()
    }
}
