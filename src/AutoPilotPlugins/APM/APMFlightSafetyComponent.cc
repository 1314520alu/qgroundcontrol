#include "APMFlightSafetyComponent.h"
#include "Vehicle.h"
#include "QGCMAVLink.h"

APMFlightSafetyComponent::APMFlightSafetyComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownSafetyVehicleComponent, parent)
{

}

QString APMFlightSafetyComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMFlightSafety.VehicleConfig.json");
}

QString APMFlightSafetyComponent::description() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_SUBMARINE:
        return tr("Configure Return to Launch, geofence, and arming checks.");
    case MAV_TYPE_GROUND_ROVER:
        return tr("Configure Return to Launch, geofence, and arming checks.");
    case MAV_TYPE_FIXED_WING:
        return tr("Configure Return to Launch, geofence, and arming checks.");
    default:
        return tr("Configure Return to Launch, geofence, and arming checks.");
    }
}

QUrl APMFlightSafetyComponent::setupSource() const
{
    if (_vehicle->sub()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFlightSafetyComponentSub.qml"));
    }
    if (_vehicle->multiRotor() || _vehicle->fixedWing() || _vehicle->rover()) {
        // Generated from APMFlightSafety.VehicleConfig.json
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFlightSafetyComponent.qml"));
    }
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMNotSupported.qml"));
}

QUrl APMFlightSafetyComponent::summaryQmlSource() const
{
    if (_vehicle->sub()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFlightSafetyComponentSummarySub.qml"));
    }
    if (_vehicle->multiRotor() || _vehicle->fixedWing() || _vehicle->rover()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFlightSafetyComponentSummary.qml"));
    }
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMNotSupported.qml"));
}
