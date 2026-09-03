#include "GCSLinkQualityTest.h"

#include "Vehicle.h"

void GCSLinkQualityTest::_noLossIsFullQuality_test()
{
    QCOMPARE(Vehicle::gcsLinkQualityFromLossPercent(0.0f), 100.0f);
}

void GCSLinkQualityTest::_lossRateMapsToRemainingPercent_test()
{
    QCOMPARE(Vehicle::gcsLinkQualityFromLossPercent(10.0f), 90.0f);
}

void GCSLinkQualityTest::_clampsOutOfRange_test()
{
    QCOMPARE(Vehicle::gcsLinkQualityFromLossPercent(-5.0f), 100.0f);
    QCOMPARE(Vehicle::gcsLinkQualityFromLossPercent(140.0f), 0.0f);
}

UT_REGISTER_TEST(GCSLinkQualityTest, TestLabel::Unit, TestLabel::Vehicle)
