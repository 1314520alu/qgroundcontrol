import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

ColumnLayout {
    id: root

    property bool hideWhenPayloadOverlay: false

    spacing: ScreenTools.defaultFontPixelHeight / 2

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: hideWhenPayloadOverlay ? 0 : terrainProgress.implicitHeight
        visible: !root.hideWhenPayloadOverlay
        // Wrapper keeps TerrainProgress from forcing itself visible over the right sidebar
        TerrainProgress {
            id: terrainProgress
            anchors.right: parent.right
        }
    }

    // We use a Loader to load the photoVideoControlComponent only when we have an active vehicle and a camera manager.
    // This make it easier to implement PhotoVideoControl without having to check for the mavlink camera
    // to be null all over the place
    Loader {
        id:                 photoVideoControlLoader
        Layout.alignment:   Qt.AlignRight
        visible:            item ? item.visible : false
        sourceComponent:    globals.activeVehicle && globals.activeVehicle.cameraManager ? photoVideoControlComponent : undefined

        property real rightEdgeCenterInset: visible ? parent.width - x : 0

        Component {
            id: photoVideoControlComponent

            PhotoVideoControl {
                hideWhenPayloadOverlay: root.hideWhenPayloadOverlay
            }
        }
    }

    // Fallback for manual streams (RTSP/UDP) and UniPod when PhotoVideoControl is not up
    // (e.g. no active vehicle yet). Prefer PhotoVideoControl / onboard UniPod when available.
    FlyViewLocalVideoControls {
        Layout.alignment:           Qt.AlignRight
        hideWhenPhotoVideoVisible:  photoVideoControlLoader.visible
        hideWhenPayloadOverlay:     root.hideWhenPayloadOverlay
    }
}
