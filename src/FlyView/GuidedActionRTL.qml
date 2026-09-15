import QGroundControl
import QGroundControl.FlyView

GuidedToolStripAction {
    text:       _guidedController.rtlTitle
    iconSource: "/res/rtl.svg"
    visible:    !forceHidden
    enabled:    _guidedController.showRTL
    actionID:   _guidedController.actionRTL

    property bool forceHidden: false
}
