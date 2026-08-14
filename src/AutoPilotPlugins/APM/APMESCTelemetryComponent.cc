#include "APMESCTelemetryComponent.h"

APMESCTelemetryComponent::APMESCTelemetryComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{
}
