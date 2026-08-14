#include "APMSafetyComponent.h"
#include "Vehicle.h"
#include "QGCMAVLink.h"

APMSafetyComponent::APMSafetyComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownSafetyVehicleComponent, parent)
{

}

QString APMSafetyComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMSafety.VehicleConfig.json");
}

QString APMSafetyComponent::description() const
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

QUrl APMSafetyComponent::setupSource() const
{
    if (_vehicle->sub()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSafetyComponentSub.qml"));
    }
    if (_vehicle->multiRotor() || _vehicle->fixedWing() || _vehicle->rover()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSafetyComponent.qml"));
    }
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMNotSupported.qml"));
}

QUrl APMSafetyComponent::summaryQmlSource() const
{
    if (_vehicle->sub()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSafetyComponentSummarySub.qml"));
    }
    if (_vehicle->multiRotor() || _vehicle->fixedWing() || _vehicle->rover()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSafetyComponentSummary.qml"));
    }
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMNotSupported.qml"));
}
