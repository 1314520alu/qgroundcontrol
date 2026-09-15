#include "Xf200TetheredPowerVisualTest.h"

#include <QtCore/QtNumeric>

#include "MAVLinkLib.h"
#include "Xf200TetheredPowerVisual.h"

void Xf200TetheredPowerVisualTest::_kind_data()
{
    QTest::addColumn<double>("voltage");
    QTest::addColumn<int>("chargeState");
    QTest::addColumn<int>("expected");

    const int ok = static_cast<int>(MAV_BATTERY_CHARGE_STATE_OK);
    const int undefined = static_cast<int>(MAV_BATTERY_CHARGE_STATE_UNDEFINED);
    const int charging = static_cast<int>(MAV_BATTERY_CHARGE_STATE_CHARGING);
    const int low = static_cast<int>(MAV_BATTERY_CHARGE_STATE_LOW);
    const int critical = static_cast<int>(MAV_BATTERY_CHARGE_STATE_CRITICAL);
    const int emergency = static_cast<int>(MAV_BATTERY_CHARGE_STATE_EMERGENCY);
    const int failed = static_cast<int>(MAV_BATTERY_CHARGE_STATE_FAILED);
    const int unhealthy = static_cast<int>(MAV_BATTERY_CHARGE_STATE_UNHEALTHY);

    QTest::newRow("nan undefined") << qQNaN() << undefined << static_cast<int>(Xf200TetheredPowerVisual::Empty);
    QTest::newRow("nan ok") << qQNaN() << ok << static_cast<int>(Xf200TetheredPowerVisual::Empty);
    QTest::newRow("120 ok") << 120.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Normal);
    QTest::newRow("119 ok") << 119.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Normal);
    QTest::newRow("121 ok") << 121.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Normal);
    QTest::newRow("120 undefined") << 120.0 << undefined << static_cast<int>(Xf200TetheredPowerVisual::Normal);
    QTest::newRow("120 charging") << 120.0 << charging << static_cast<int>(Xf200TetheredPowerVisual::Normal);
    QTest::newRow("117 warn") << 117.0 << undefined << static_cast<int>(Xf200TetheredPowerVisual::Warn);
    QTest::newRow("115 warn") << 115.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Warn);
    QTest::newRow("125 warn") << 125.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Warn);
    QTest::newRow("130 warn") << 130.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Warn);
    QTest::newRow("114 critical") << 114.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Critical);
    QTest::newRow("131 critical") << 131.0 << ok << static_cast<int>(Xf200TetheredPowerVisual::Critical);
    QTest::newRow("low overrides voltage") << 120.0 << low << static_cast<int>(Xf200TetheredPowerVisual::Low);
    QTest::newRow("critical charge") << 120.0 << critical << static_cast<int>(Xf200TetheredPowerVisual::Critical);
    QTest::newRow("emergency") << 120.0 << emergency << static_cast<int>(Xf200TetheredPowerVisual::Emergency);
    QTest::newRow("failed") << 120.0 << failed << static_cast<int>(Xf200TetheredPowerVisual::Emergency);
    QTest::newRow("unhealthy") << qQNaN() << unhealthy << static_cast<int>(Xf200TetheredPowerVisual::Emergency);
}

void Xf200TetheredPowerVisualTest::_kind()
{
    QFETCH(double, voltage);
    QFETCH(int, chargeState);
    QFETCH(int, expected);

    QCOMPARE(static_cast<int>(Xf200TetheredPowerVisual::kind(voltage, chargeState)), expected);
}

void Xf200TetheredPowerVisualTest::_batterySvgMatchesKind()
{
    QCOMPARE(Xf200TetheredPowerVisual::batterySvg(Xf200TetheredPowerVisual::Empty),
             QStringLiteral("/qmlimages/Battery.svg"));
    QCOMPARE(Xf200TetheredPowerVisual::batterySvg(Xf200TetheredPowerVisual::Normal),
             QStringLiteral("/qmlimages/BatteryGreen.svg"));
    QCOMPARE(Xf200TetheredPowerVisual::batterySvg(Xf200TetheredPowerVisual::Warn),
             QStringLiteral("/qmlimages/BatteryYellow.svg"));
    QCOMPARE(Xf200TetheredPowerVisual::batterySvg(Xf200TetheredPowerVisual::Low),
             QStringLiteral("/qmlimages/BatteryOrange.svg"));
    QCOMPARE(Xf200TetheredPowerVisual::batterySvg(Xf200TetheredPowerVisual::Critical),
             QStringLiteral("/qmlimages/BatteryCritical.svg"));
    QCOMPARE(Xf200TetheredPowerVisual::batterySvg(Xf200TetheredPowerVisual::Emergency),
             QStringLiteral("/qmlimages/BatteryEMERGENCY.svg"));
}

void Xf200TetheredPowerVisualTest::_psuSvgIsPowerSupply()
{
    QCOMPARE(Xf200TetheredPowerVisual::psuSvg(Xf200TetheredPowerVisual::Normal),
             QStringLiteral("/qmlimages/PowerSupply.svg"));
    QCOMPARE(Xf200TetheredPowerVisual::psuSvg(Xf200TetheredPowerVisual::Emergency),
             QStringLiteral("/qmlimages/PowerSupply.svg"));
}

void Xf200TetheredPowerVisualTest::_aircraftModelConstantIsXf200Tethered()
{
    QCOMPARE(Xf200TetheredPowerVisual::kAircraftModelZyXf200Tethered, 3);
}

void Xf200TetheredPowerVisualTest::_mavlinkIdsAreZeroBased()
{
    QCOMPARE(Xf200TetheredPowerVisual::mavlinkBatteryId(1), 0);
    QCOMPARE(Xf200TetheredPowerVisual::mavlinkBatteryId(4), 3);
    QCOMPARE(Xf200TetheredPowerVisual::mavlinkBatteryId(5), 4);
}

UT_REGISTER_TEST(Xf200TetheredPowerVisualTest, TestLabel::Unit)
