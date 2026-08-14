#pragma once

#include "UnitTest.h"

class FlightModeNameTranslatorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testEnglishUnchanged();
    void _testChineseCopterModesFromFirmware();
    void _testChineseCopterModeAliases();
    void _testChinesePlaneLoiter();
    void _testChineseUnknownLeftInEnglish();
    void _testChineseDuplicateSuffixPreserved();
};
