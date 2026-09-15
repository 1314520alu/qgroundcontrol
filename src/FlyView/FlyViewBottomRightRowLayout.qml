import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Item {
    id: root

    // Telemetry lives bottom-center; this row only hosts the instrument panel.
    width: instrumentPanel.visible ? instrumentPanel.width : 0
    height: instrumentPanel.visible ? instrumentPanel.height : 0

    FlyViewInstrumentPanel {
        id:                 instrumentPanel
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        visible:            QGroundControl.corePlugin.options.flyView.showInstrumentPanel && _showSingleVehicleUI
    }
}
