#include "MyAircraftSettingsTest.h"

#include <QtCore/QScopeGuard>

#include "MyAircraftSettings.h"
#include "SettingsManager.h"

void MyAircraftSettingsTest::_defaults()
{
    MyAircraftSettings* const settings = SettingsManager::instance()->myAircraftSettings();
    QVERIFY(settings);

    Fact* const enabled = settings->weitongAircraftEnabled();
    Fact* const host = settings->satcomHost();
    Fact* const port = settings->satcomPort();
    QVERIFY(enabled);
    QVERIFY(host);
    QVERIFY(port);

    QCOMPARE(enabled->rawValue().toBool(), false);
    QCOMPARE(host->rawValue().toString(), QStringLiteral("192.168.0.200"));
    QCOMPARE(port->rawValue().toUInt(), 23u);
}

void MyAircraftSettingsTest::_enableToggle()
{
    MyAircraftSettings* const settings = SettingsManager::instance()->myAircraftSettings();
    QVERIFY(settings);

    Fact* const enabled = settings->weitongAircraftEnabled();
    const QVariant saved = enabled->rawValue();
    const auto guard = qScopeGuard([enabled, saved] { enabled->setRawValue(saved); });

    enabled->setRawValue(true);
    QCOMPARE(enabled->rawValue().toBool(), true);
    enabled->setRawValue(false);
    QCOMPARE(enabled->rawValue().toBool(), false);
}

UT_REGISTER_TEST(MyAircraftSettingsTest, TestLabel::Unit)
