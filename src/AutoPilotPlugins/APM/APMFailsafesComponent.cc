#include "APMFailsafesComponent.h"
#include "Vehicle.h"
#include "QGCMAVLink.h"

APMFailsafesComponent::APMFailsafesComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}

QString APMFailsafesComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMFailsafes.VehicleConfig.json");
}

QString APMFailsafesComponent::description() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_SUBMARINE:
        return tr("Configure failsafe actions and leak detection.");
    case MAV_TYPE_GROUND_ROVER:
        return tr("Configure battery, GCS, throttle, and EKF failsafes.");
    case MAV_TYPE_FIXED_WING:
        return tr("Configure battery, GCS, and throttle failsafes.");
    default:
        return tr("Configure battery, GCS, RC, throttle, EKF, and dead reckoning failsafes.");
    }
}

QUrl APMFailsafesComponent::setupSource() const
{
    if (_vehicle->sub() || _vehicle->fixedWing() || _vehicle->multiRotor() || _vehicle->rover()) {
        // Generated from APMFailsafes.VehicleConfig.json
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFailsafesComponent.qml"));
    }
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMNotSupported.qml"));
}

QUrl APMFailsafesComponent::summaryQmlSource() const
{
    if (_vehicle->sub()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFailsafesComponentSummarySub.qml"));
    }
    if (_vehicle->fixedWing() || _vehicle->multiRotor() || _vehicle->rover()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFailsafesComponentSummary.qml"));
    }
    return QUrl();
}
