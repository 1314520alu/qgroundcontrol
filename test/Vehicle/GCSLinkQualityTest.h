#pragma once

#include "UnitTest.h"

class GCSLinkQualityTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _noLossIsFullQuality_test();
    void _lossRateMapsToRemainingPercent_test();
    void _clampsOutOfRange_test();
};
