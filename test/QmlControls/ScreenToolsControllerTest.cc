#include "ScreenToolsControllerTest.h"

#include "ScreenToolsController.h"

void ScreenToolsControllerTest::_recommendedUiScaleUnknownPresetIsZero()
{
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QString()), 0);
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("NotARemote")), 0);
}

void ScreenToolsControllerTest::_recommendedUiScaleTenInchClass()
{
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("UniRC 10 Pro")), 100);
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("云卓 H30")), 100);
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("云卓 H16")), 100);
}

void ScreenToolsControllerTest::_recommendedUiScaleSevenInchClass()
{
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("UniRC 7")), 90);
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("MK32")), 90);
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("云卓 G20")), 90);
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("云卓 G16")), 90);
}

void ScreenToolsControllerTest::_recommendedUiScaleMk15()
{
    QCOMPARE(ScreenToolsController::recommendedUiScalePercentForPreset(QStringLiteral("MK15")), 80);
}

UT_REGISTER_TEST(ScreenToolsControllerTest, TestLabel::Unit)
