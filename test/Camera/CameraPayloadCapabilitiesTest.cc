#include "CameraPayloadCapabilitiesTest.h"

#include <QtTest/QSignalSpy>

#include "MavlinkCameraControlInterface.h"
#include "PayloadCapabilityCatalog.h"
#include "SimulatedCameraControl.h"
#include "SiyiA8MiniCameraControl.h"
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

void CameraPayloadCapabilitiesTest::testCatalogPhase1Sets()
{
    auto& catalog = PayloadCapabilityCatalog::instance();
    QVERIFY(catalog.isLoaded());

    const auto* mt11 = catalog.byModelName(QStringLiteral("UniPod MT11"));
    QVERIFY(mt11);
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("gimbal")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("lens")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("laser")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("ai")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("follow")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("exposure_auto")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("zoom")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("focus")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(mt11, QStringLiteral("media_library")));

    const auto* a8 = catalog.byVideoSource(QStringLiteral("SIYI A8 Mini"));
    QVERIFY(a8);
    QVERIFY(PayloadCapabilityCatalog::overlayHas(a8, QStringLiteral("gimbal")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(a8, QStringLiteral("exposure_auto")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(a8, QStringLiteral("zoom")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(a8, QStringLiteral("focus")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(a8, QStringLiteral("media_library")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(a8, QStringLiteral("ai")));

    const auto* tq10 = catalog.byModelName(QStringLiteral("Topotek TQ10N"));
    QVERIFY(tq10);
    QVERIFY(PayloadCapabilityCatalog::overlayHas(tq10, QStringLiteral("gimbal")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(tq10, QStringLiteral("exposure_auto")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(tq10, QStringLiteral("zoom")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(tq10, QStringLiteral("focus")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(tq10, QStringLiteral("laser")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(tq10, QStringLiteral("media_library")));
}

void CameraPayloadCapabilitiesTest::testZr10CatalogPhase1()
{
    auto& catalog = PayloadCapabilityCatalog::instance();
    QVERIFY(catalog.isLoaded());

    const auto* zr10 = catalog.byVideoSource(QStringLiteral("SIYI ZR10"));
    QVERIFY(zr10);
    QCOMPARE(zr10->modelName, QStringLiteral("SIYI ZR10"));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("gimbal")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("exposure_auto")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("photo")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("video")));
    QVERIFY(PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("zoom")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("focus")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("media_library")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("laser")));
    QVERIFY(!PayloadCapabilityCatalog::overlayHas(zr10, QStringLiteral("ai")));
}

void CameraPayloadCapabilitiesTest::testTopotekHasGimbalPad()
{
    TopotekTq10Client client(this);
    TopotekTq10CameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.hasGimbalPad(), true);
    QCOMPARE(cam.hasZoom(), true);
    QCOMPARE(cam.hasFocus(), false);
    QCOMPARE(cam.hasLensSwitch(), false);
    QCOMPARE(cam.hasLaserRange(), false);
    QCOMPARE(cam.hasMediaLibrary(), false);
    QCOMPARE(cam.hasExposureAuto(), false);
}

void CameraPayloadCapabilitiesTest::testUnipodPhase1Flags()
{
    UnipodMt11Client client(this);
    UnipodMt11CameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.hasGimbalPad(), true);
    QCOMPARE(cam.hasLensSwitch(), true);
    QCOMPARE(cam.hasLaserRange(), true);
    QCOMPARE(cam.hasAiRecognition(), true);
    QCOMPARE(cam.hasFollowFlight(), true);
    QCOMPARE(cam.hasExposureAuto(), false);
    QCOMPARE(cam.hasZoom(), true);
    QCOMPARE(cam.hasFocus(), true);
    QCOMPARE(cam.hasMediaLibrary(), false);
}

void CameraPayloadCapabilitiesTest::testA8Phase1Flags()
{
    UnipodMt11Client client(this);
    SiyiA8MiniCameraControl cam(nullptr, &client, this);
    QCOMPARE(cam.modelName(), QStringLiteral("SIYI A8 Mini"));
    QCOMPARE(cam.hasGimbalPad(), true);
    QCOMPARE(cam.hasZoom(), true);
    QCOMPARE(cam.hasFocus(), false);
    QCOMPARE(cam.hasMediaLibrary(), false);
    QCOMPARE(cam.hasExposureAuto(), false);
    QCOMPARE(cam.capturesPhotos(), true);
    QCOMPARE(cam.capturesVideo(), true);
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
