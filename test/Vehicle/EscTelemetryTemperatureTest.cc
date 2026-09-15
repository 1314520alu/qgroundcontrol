#include "EscTelemetryTemperatureTest.h"

#include "EscStatusFactGroupListModel.h"

void EscTelemetryTemperatureTest::_celsiusAsKelvinRecover_data()
{
    QTest::addColumn<int>("mavlinkTemp");
    QTest::addColumn<float>("expectedDegC");

    // Bus Status.temperature = degC (mis-typed as Kelvin) → ESC_TELEMETRY uint8 = degC - 17
    QTest::newRow("26C_from_9") << 9 << 26.0f;
    QTest::newRow("28C_from_11") << 11 << 28.0f;
    QTest::newRow("30C_from_13") << 13 << 30.0f;
    QTest::newRow("20C_from_3") << 3 << 20.0f;
}

void EscTelemetryTemperatureTest::_celsiusAsKelvinRecover()
{
    QFETCH(int, mavlinkTemp);
    QFETCH(float, expectedDegC);

    QCOMPARE(escTelemetryDegCFromCelsiusAsKelvinMavlink(static_cast<uint8_t>(mavlinkTemp)), expectedDegC);
}

UT_REGISTER_TEST(EscTelemetryTemperatureTest, TestLabel::Unit)
