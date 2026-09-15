import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Viewer3D

ToolStripActionList {
    id: _root

    // When true, takeoff / land / RTL live in FlyViewPayloadOverlay left bar instead
    property bool hidePrimaryGuided: false

    signal displayPreFlightChecklist

    model: [
        Viewer3DShowAction { },
        PreFlightCheckListShowAction { onTriggered: displayPreFlightChecklist() },
        GuidedActionTakeoff { forceHidden: _root.hidePrimaryGuided },
        GuidedActionLand { forceHidden: _root.hidePrimaryGuided },
        GuidedActionRTL { forceHidden: _root.hidePrimaryGuided },
        GuidedActionPause { },
        FlyViewAdditionalActionsButton { },
        FlyViewGripperButton { }
    ]
}
