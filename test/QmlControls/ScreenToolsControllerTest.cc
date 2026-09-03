#include "ScreenToolsControllerTest.h"

#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

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

void ScreenToolsControllerTest::_siyiRadioEthernetReadyMatchesLocal144Net()
{
    bool has144 = false;
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& addr : addresses) {
        if (addr.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }
        if ((addr.toIPv4Address() & 0xffffff00u) == 0xc0a89000u) {
            has144 = true;
            break;
        }
    }

    QCOMPARE(ScreenToolsController::isSiyiRadioEthernetReady(), has144);
    if (has144) {
        QVERIFY(ScreenToolsController::siyiRadioEthernetAddress().startsWith(QStringLiteral("192.168.144.")));
    } else {
        QVERIFY(ScreenToolsController::siyiRadioEthernetAddress().isEmpty());
    }
}

UT_REGISTER_TEST(ScreenToolsControllerTest, TestLabel::Unit)
