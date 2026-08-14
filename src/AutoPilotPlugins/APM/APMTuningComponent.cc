#include "APMTuningComponent.h"
#include "Vehicle.h"

APMTuningComponent::APMTuningComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}

QString APMTuningComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMTuningCopter.VehicleConfig.json");
}

QUrl APMTuningComponent::setupSource() const
{
    if (_vehicle->multiRotor()) {
        // Generated from APMTuningCopter.VehicleConfig.json
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMTuningCopterComponent.qml");
    }
    return QUrl::fromUserInput(QString());
}
