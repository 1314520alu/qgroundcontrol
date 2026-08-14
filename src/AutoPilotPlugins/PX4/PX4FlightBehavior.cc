#include "PX4FlightBehavior.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

PX4FlightBehavior::PX4FlightBehavior(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Flight Behavior"))
{
}

QString PX4FlightBehavior::name() const
{
    return _name;
}

QString PX4FlightBehavior::description() const
{
    return tr("Configure mission, position hold, and altitude mode settings.");
}

QString PX4FlightBehavior::iconResource() const
{
    return "/qmlimages/TuningComponentIcon.png";
}

bool PX4FlightBehavior::requiresSetup() const
{
    return false;
}

bool PX4FlightBehavior::setupComplete() const
{
    return true;
}

QStringList PX4FlightBehavior::setupCompleteChangedTriggerList() const
{
    return QStringList();
}

QUrl PX4FlightBehavior::setupSource() const
{
    if (_vehicle->multiRotor()) {
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4FlightBehaviorCopter.qml");
    }
    return QUrl();
}

QUrl PX4FlightBehavior::summaryQmlSource() const
{
    return QUrl();
}
