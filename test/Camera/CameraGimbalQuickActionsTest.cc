#include "CameraGimbalQuickActionsTest.h"

#include "SimulatedCameraControl.h"
#include "SiyiA8MiniCameraControl.h"
#include "TopotekTq10CameraControl.h"
#include "TopotekTq10Client.h"
#include "UnipodMt11CameraControl.h"
#include "UnipodMt11Client.h"

void CameraGimbalQuickActionsTest::testSimulatedDefaultsFalse()
{
    SimulatedCameraControl cam(nullptr, this);
    QCOMPARE(cam.hasGimbalRecenter(), false);
    QCOMPARE(cam.hasGimbalLookDown(), false);
    QCOMPARE(cam.hasGimbalYawRecenter(), false);
    QCOMPARE(cam.hasGimbalPitchDown(), false);
}

void CameraGimbalQuickActionsTest::testTopotekOnlyRecenter()
{
    TopotekTq10Client client(this);
    TopotekTq10CameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.hasGimbalRecenter(), true);
    QCOMPARE(cam.hasGimbalLookDown(), false);
    QCOMPARE(cam.hasGimbalYawRecenter(), false);
    QCOMPARE(cam.hasGimbalPitchDown(), false);
}

void CameraGimbalQuickActionsTest::testUnipodHasAllFour()
{
    UnipodMt11Client client(this);
    UnipodMt11CameraControl cam(nullptr, &client, this);
    QVERIFY(cam.hasGimbalPad());
    QCOMPARE(cam.hasGimbalRecenter(), true);
    QCOMPARE(cam.hasGimbalLookDown(), true);
    QCOMPARE(cam.hasGimbalYawRecenter(), true);
    QCOMPARE(cam.hasGimbalPitchDown(), true);
}

void CameraGimbalQuickActionsTest::testA8HasAllFour()
{
    UnipodMt11Client client(this);
    SiyiA8MiniCameraControl cam(nullptr, &client, this);
    QVERIFY(cam.hasGimbalPad());
    QCOMPARE(cam.hasGimbalRecenter(), true);
    QCOMPARE(cam.hasGimbalLookDown(), true);
    QCOMPARE(cam.hasGimbalYawRecenter(), true);
    QCOMPARE(cam.hasGimbalPitchDown(), true);
}

UT_REGISTER_TEST(CameraGimbalQuickActionsTest, TestLabel::Unit)
