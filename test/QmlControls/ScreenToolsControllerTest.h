#pragma once

#include "UnitTest.h"

class ScreenToolsControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _recommendedUiScaleUnknownPresetIsZero();
    void _recommendedUiScaleTenInchClass();
    void _recommendedUiScaleSevenInchClass();
    void _recommendedUiScaleMk15();
    void _siyiRadioEthernetReadyMatchesLocal144Net();
};
