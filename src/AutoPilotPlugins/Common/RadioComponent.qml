import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.VehicleSetup

SetupPage {
    id: radioPage
    pageComponent: pageComponent
    showPageDescription: false

    Component {
        id: pageComponent

        // Fit the whole Radio UI in the visible viewport (no page scroll on short landscape remotes).
        Item {
            width: availableWidth
            height: availableHeight

            RemoteControlCalibration {
                id: remoteControlCalibration
                anchors.fill: parent
                compactSinglePage: true

                useDeadband: false

                controller: RadioComponentController {
                    statusText: remoteControlCalibration.statusText
                    cancelButton: remoteControlCalibration.cancelButton
                    nextButton: remoteControlCalibration.nextButton
                    joystickMode: false

                    onThrottleReversedCalFailure: QGroundControl.showMessageDialog(radioPage, qsTr("Throttle channel reversed"), qsTr("Calibration failed. The throttle channel on your transmitter is reversed. You must correct this on your transmitter in order to complete calibration."))
                }

                Component.onCompleted: controller.start()

                // Spektrum / CRSF bind and Copy Trims removed (not used on SIYI / Skydroid handheld GCS).
                // PX4 keeps AUX / switch mapping below the main cards.
                additionalSetupComponent: (QGroundControl.multiVehicleManager.activeVehicle && QGroundControl.multiVehicleManager.activeVehicle.px4Firmware)
                                          ? px4AuxSetupComponent
                                          : null

                Component {
                    id: px4AuxSetupComponent

                    ColumnLayout {
                        spacing: ScreenTools.defaultFontPixelHeight / 2

                        QGCLabel {
                            text: qsTr("Switch / Aux Mapping")
                            font.bold: true
                        }

                        Repeater {
                            model: QGroundControl.multiVehicleManager.activeVehicle.multiRotor ?
                                        [ "RC_MAP_AUX1", "RC_MAP_AUX2", "RC_MAP_PARAM1", "RC_MAP_PARAM2", "RC_MAP_PARAM3", "RC_MAP_PAY_SW"] :
                                        [ "RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2", "RC_MAP_PARAM1", "RC_MAP_PARAM2", "RC_MAP_PARAM3", "RC_MAP_PAY_SW"]

                            LabelledFactComboBox {
                                label: fact.shortDescription
                                fact: remoteControlCalibration.controller.getParameterFact(-1, modelData)
                                indexModel: false
                            }
                        }
                    }
                }
            }
        }
    }
}
