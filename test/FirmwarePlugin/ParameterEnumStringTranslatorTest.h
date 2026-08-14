#pragma once

#include "UnitTest.h"

class ParameterEnumStringTranslatorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testEnglishUnchanged();
    void _testChineseBatteryFailsafeActions();
    void _testChineseFlightModeEnums();
    void _testChineseRcOptionEnums();
    void _testChineseUnknownLeftInEnglish();
    void _testChineseTranslateList();
};
