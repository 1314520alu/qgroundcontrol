#include "Xf200TetheredPowerVisual.h"

#include <QtCore/QtNumeric>

#include "MAVLinkLib.h"

Xf200TetheredPowerVisual::Xf200TetheredPowerVisual(QObject* parent) : QObject(parent) {}

int Xf200TetheredPowerVisual::aircraftModelZyXf200Tethered() const
{
    return kAircraftModelZyXf200Tethered;
}

Xf200TetheredPowerVisual::Kind Xf200TetheredPowerVisual::kind(double voltage, int chargeState)
{
    switch (chargeState) {
        case MAV_BATTERY_CHARGE_STATE_LOW:
            return Low;
        case MAV_BATTERY_CHARGE_STATE_CRITICAL:
            return Critical;
        case MAV_BATTERY_CHARGE_STATE_EMERGENCY:
        case MAV_BATTERY_CHARGE_STATE_FAILED:
        case MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
            return Emergency;
        default:
            break;
    }

    if (qIsNaN(voltage)) {
        return Empty;
    }

    if (voltage >= 119.0 && voltage <= 121.0) {
        return Normal;
    }
    if ((voltage >= 115.0 && voltage < 119.0) || (voltage > 121.0 && voltage <= 130.0)) {
        return Warn;
    }
    return Critical;
}

QString Xf200TetheredPowerVisual::batterySvg(Kind kind)
{
    switch (kind) {
        case Normal:
            return QStringLiteral("/qmlimages/BatteryGreen.svg");
        case Warn:
            return QStringLiteral("/qmlimages/BatteryYellow.svg");
        case Low:
            return QStringLiteral("/qmlimages/BatteryOrange.svg");
        case Critical:
            return QStringLiteral("/qmlimages/BatteryCritical.svg");
        case Emergency:
            return QStringLiteral("/qmlimages/BatteryEMERGENCY.svg");
        case Empty:
        default:
            return QStringLiteral("/qmlimages/Battery.svg");
    }
}

QString Xf200TetheredPowerVisual::psuSvg(Kind kind)
{
    Q_UNUSED(kind);
    return QStringLiteral("/qmlimages/PowerSupply.svg");
}

int Xf200TetheredPowerVisual::mavlinkBatteryId(int slotId)
{
    return slotId - 1;
}
