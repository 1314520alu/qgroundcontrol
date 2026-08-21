#include "CameraPayloadCapabilitiesTest.h"

#include <QtTest/QSignalSpy>

#include "MavlinkCameraControlInterface.h"
#include "SimulatedCameraControl.h"
#include "TopotekTq10CameraControl.h"
#include "TopotekTq10Client.h"
#include "UnipodMt11CameraControl.h"
#include "UnipodMt11Client.h"
#include "UnipodMt11MediaClient.h"

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

void CameraPayloadCapabilitiesTest::testUnipodMediaLibraryFollowsReady()
{
    UnipodMt11Client client(this);
    UnipodMt11MediaClient media(this);
    UnipodMt11CameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.hasMediaLibrary(), false);

    cam.setMediaClient(&media);
    QCOMPARE(cam.hasMediaLibrary(), false);

    QSignalSpy spy(&cam, &MavlinkCameraControlInterface::infoChanged);
    media.setReady(true);
    QCOMPARE(cam.hasMediaLibrary(), true);
    QVERIFY(spy.count() >= 1);

    media.setReady(false);
    QCOMPARE(cam.hasMediaLibrary(), false);
}

UT_REGISTER_TEST(CameraPayloadCapabilitiesTest, TestLabel::Unit)
