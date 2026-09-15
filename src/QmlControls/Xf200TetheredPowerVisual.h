#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

/// Visual kind for ZY-XF200 tethered toolbar PSU / bus icons.
class Xf200TetheredPowerVisual : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    Q_PROPERTY(int aircraftModelZyXf200Tethered READ aircraftModelZyXf200Tethered CONSTANT)

    explicit Xf200TetheredPowerVisual(QObject* parent = nullptr);

    static constexpr int kAircraftModelZyXf200Tethered = 3;

    int aircraftModelZyXf200Tethered() const;

    enum Kind
    {
        Empty = 0,
        Normal,
        Warn,
        Low,
        Critical,
        Emergency,
    };
    Q_ENUM(Kind)

    Q_INVOKABLE static Kind kind(double voltage, int chargeState);
    Q_INVOKABLE static QString batterySvg(Kind kind);
    Q_INVOKABLE static QString psuSvg(Kind kind);

    /// Compact slots are 1-5; MAVLink BATTERY_STATUS.id is 0-based (bus = 4).
    Q_INVOKABLE static int mavlinkBatteryId(int slotId);
};
