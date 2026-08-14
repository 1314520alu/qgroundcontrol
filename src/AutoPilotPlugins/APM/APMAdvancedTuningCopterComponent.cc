#include "APMAdvancedTuningCopterComponent.h"
#include "Vehicle.h"

APMAdvancedTuningCopterComponent::APMAdvancedTuningCopterComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{
}

QUrl APMAdvancedTuningCopterComponent::setupSource() const
{
    if (_vehicle->multiRotor()) {
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMAdvancedTuningCopterComponent.qml");
    }
    return QUrl::fromUserInput(QString());
}
