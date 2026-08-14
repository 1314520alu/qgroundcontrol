#include "MotorComponentController.h"

#include "Fact.h"
#include "MotorLayoutBuilder.h"
#include "ParameterManager.h"
#include "Vehicle.h"

MotorComponentController::MotorComponentController(QObject *parent)
    : FactPanelController(parent)
{
    if (_vehicle) {
        (void) connect(_vehicle, &Vehicle::vehicleTypeChanged, this, &MotorComponentController::_rebuildLayout);
        if (_vehicle->parameterManager()) {
            (void) connect(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged,
                           this, &MotorComponentController::_rebuildLayout);
        }
    }
    _rebuildLayout();
}

int MotorComponentController::clampThrottle(int percent) const
{
    return qBound(0, percent, maxThrottle());
}

void MotorComponentController::refreshLayout()
{
    _rebuildLayout();
}

void MotorComponentController::_rebuildLayout()
{
    _motors.clear();
    _topologyName.clear();
    _vehicleName.clear();
    _spatialLayout = false;
    _motorCount = 0;

    if (!_vehicle) {
        emit layoutChanged();
        return;
    }

    _vehicleName = _vehicle->vehicleTypeString();
    const int motorCount = _vehicle->motorCount();
    _motorCount = motorCount;

    int frameClass = -1;
    int frameType = -1;
    if (parameterExists(ParameterManager::defaultComponentId, QStringLiteral("FRAME_CLASS"))) {
        Fact *const frameClassFact = getParameterFact(ParameterManager::defaultComponentId, QStringLiteral("FRAME_CLASS"), false);
        if (frameClassFact) {
            frameClass = frameClassFact->rawValue().toInt();
            (void) connect(frameClassFact, &Fact::rawValueChanged, this, &MotorComponentController::_rebuildLayout, Qt::UniqueConnection);
        }
    }
    if (parameterExists(ParameterManager::defaultComponentId, QStringLiteral("FRAME_TYPE"))) {
        Fact *const frameTypeFact = getParameterFact(ParameterManager::defaultComponentId, QStringLiteral("FRAME_TYPE"), false);
        if (frameTypeFact) {
            frameType = frameTypeFact->rawValue().toInt();
            (void) connect(frameTypeFact, &Fact::rawValueChanged, this, &MotorComponentController::_rebuildLayout, Qt::UniqueConnection);
        }
    }

    const MotorLayoutBuilder::Result layout = MotorLayoutBuilder::build(frameClass, frameType, motorCount);
    _topologyName = layout.topologyName;
    _spatialLayout = layout.spatial;

    for (const MotorLayoutBuilder::Entry &entry : layout.motors) {
        QVariantMap m;
        m.insert(QStringLiteral("letter"), entry.letter);
        m.insert(QStringLiteral("motorIndex"), entry.motorIndex);
        m.insert(QStringLiteral("angleDeg"), entry.angleDeg);
        QString dir = QStringLiteral("Unknown");
        if (entry.dir == MotorLayoutBuilder::SpinDir::CW) {
            dir = QStringLiteral("CW");
        } else if (entry.dir == MotorLayoutBuilder::SpinDir::CCW) {
            dir = QStringLiteral("CCW");
        }
        m.insert(QStringLiteral("dir"), dir);
        QString layer = QStringLiteral("single");
        if (entry.layer == MotorLayoutBuilder::Layer::Top) {
            layer = QStringLiteral("top");
        } else if (entry.layer == MotorLayoutBuilder::Layer::Bottom) {
            layer = QStringLiteral("bottom");
        }
        m.insert(QStringLiteral("layer"), layer);
        _motors.append(m);
    }

    emit layoutChanged();
}
