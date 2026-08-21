#include "CameraPayloadCapabilitiesTest.h"

#include "SimulatedCameraControl.h"
#include "TopotekTq10CameraControl.h"
#include "TopotekTq10Client.h"

void CameraPayloadCapabilitiesTest::testSimulatedDefaultsFalse()
{
    SimulatedCameraControl cam(nullptr, this);
    QCOMPARE(cam.hasGimbalPad(), false);
    QCOMPARE(cam.hasLensSwitch(), false);
    QCOMPARE(cam.hasLaserRange(), false);
    QCOMPARE(cam.hasAiRecognition(), false);
    QCOMPARE(cam.hasFollowFlight(), false);
    QCOMPARE(cam.hasMediaLibrary(), false);
}

void CameraPayloadCapabilitiesTest::testTopotekHasGimbalPad()
{
    TopotekTq10Client client(this);
    TopotekTq10CameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.hasGimbalPad(), true);
    QCOMPARE(cam.hasLensSwitch(), false);
    QCOMPARE(cam.hasLaserRange(), false);
    QCOMPARE(cam.hasMediaLibrary(), false);
}

UT_REGISTER_TEST(CameraPayloadCapabilitiesTest, TestLabel::Unit)
