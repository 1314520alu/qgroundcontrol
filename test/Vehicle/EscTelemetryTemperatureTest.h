#pragma once

#include "UnitTest.h"

class EscTelemetryTemperatureTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _celsiusAsKelvinRecover_data();
    void _celsiusAsKelvinRecover();
};
