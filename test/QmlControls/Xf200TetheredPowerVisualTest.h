#pragma once

#include "UnitTest.h"

class Xf200TetheredPowerVisualTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _kind_data();
    void _kind();
    void _batterySvgMatchesKind();
    void _psuSvgIsPowerSupply();
    void _aircraftModelConstantIsXf200Tethered();
    void _mavlinkIdsAreZeroBased();
};
