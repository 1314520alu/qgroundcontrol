#pragma once

#include "UnitTest.h"

class VehicleStatusTextTranslatorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testEnglishUnchanged();
    void _testChinesePreArm();
    void _testChineseParameterPreserved();
    void _testChineseProtocolUnchanged();
    void _testChineseFlightEvents();
    void _testChineseUnknownLeftInEnglish();
    void _testHtmlUnchanged();
};
