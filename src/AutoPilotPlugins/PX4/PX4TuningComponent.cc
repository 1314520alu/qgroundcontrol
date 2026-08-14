#include "PX4TuningComponent.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

PX4TuningComponent::PX4TuningComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("PID Tuning"))
{
}

QString PX4TuningComponent::name(void) const
{
    return _name;
}

QString PX4TuningComponent::description(void) const
{
    return tr("Configure rate, attitude, and velocity controller gains.");
}

QString PX4TuningComponent::iconResource(void) const
{
    return "/qmlimages/TuningComponentIcon.png";
}

bool PX4TuningComponent::requiresSetup(void) const
{
    return false;
}

bool PX4TuningComponent::setupComplete(void) const
{
    return true;
}

QStringList PX4TuningComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl PX4TuningComponent::setupSource(void) const
{
    QString qmlFile;

    if (_vehicle->fixedWing()) {
        qmlFile = "qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentPlane.qml";
    } else if (_vehicle->multiRotor()) {
        qmlFile = "qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentCopter.qml";
    } else if (_vehicle->vtol()) {
        qmlFile = "qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentVTOL.qml";
    } else if (_vehicle->vehicleType() == MAV_TYPE_SPACECRAFT_ORBITER) {
        qmlFile = "qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4TuningComponentSpacecraft.qml";
    }

    return QUrl::fromUserInput(qmlFile);
}

QUrl PX4TuningComponent::summaryQmlSource(void) const
{
    return QUrl();
}
