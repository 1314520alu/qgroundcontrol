import QGroundControl
import QGroundControl.FlyView

GuidedToolStripAction {
    text:       _guidedController.takeoffTitle
    iconSource: "/res/takeoff.svg"
    // Hidden when Fly View payload overlay hosts takeoff at left-bar bottom
    visible:    (_guidedController.showTakeoff || !_guidedController.showLand) && !forceHidden
    enabled:    _guidedController.showTakeoff
    actionID:   _guidedController.actionTakeoff

    property bool forceHidden: false
}
