#pragma once

#include "VehicleComponent.h"

class APMESCTelemetryComponent : public VehicleComponent
{
    Q_OBJECT

public:
    APMESCTelemetryComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final
    {
        return tr("实时查看各路电调 MAVLink 遥测（ESC_TELEMETRY / ESC_INFO / ESC_STATUS）。");
    }
    QString iconResource() const final { return QStringLiteral("/qmlimages/EscIndicator.svg"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final
    {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCTelemetryComponent.qml"));
    }
    QUrl summaryQmlSource() const final
    {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCTelemetryComponentSummary.qml"));
    }
    bool allowSetupWhileArmed() const final { return true; }
    bool allowSetupWhileFlying() const final { return true; }

private:
    const QString _name = tr("电调遥测");
};
