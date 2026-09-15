import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ToolStrip {
    id: _root

    // Relocate takeoff / land / RTL into payload overlay left strip
    property alias hidePrimaryGuided: flyViewToolStripActionList.hidePrimaryGuided

    signal displayPreFlightChecklist

    FlyViewToolStripActionList {
        id: flyViewToolStripActionList

        onDisplayPreFlightChecklist: _root.displayPreFlightChecklist()
    }

    model: flyViewToolStripActionList.model
}
