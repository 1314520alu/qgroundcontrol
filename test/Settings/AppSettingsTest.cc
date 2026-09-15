#include "AppSettingsTest.h"

#include <QtCore/QScopeGuard>

#include "AppSettings.h"
#include "FirmwarePluginManager.h"
#include "QGCMAVLink.h"
#include "SettingsManager.h"

UT_REGISTER_TEST(AppSettingsTest, TestLabel::Unit)

void AppSettingsTest::_preferredFirmwareClassEnumFiltered()
{
    _verifyFirmwareClassEnumFiltered(SettingsManager::instance()->appSettings()->preferredFirmwareClass());
}

void AppSettingsTest::_updateManifestUrlDefaultEmpty()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    QVERIFY(appSettings);
    Fact* const fact = appSettings->updateManifestUrl();
    QVERIFY(fact);
    QCOMPARE(fact->rawValue().toString(), QString());
    const QVariant saved = fact->rawValue();
    const auto guard = qScopeGuard([fact, saved] { fact->setRawValue(saved); });
    fact->setRawValue(QStringLiteral("https://example.com/latest.json"));
    QCOMPARE(fact->rawValue().toString(), QStringLiteral("https://example.com/latest.json"));
}

void AppSettingsTest::_offlineEditingFirmwareClassEnumFiltered()
{
    _verifyFirmwareClassEnumFiltered(SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass());
}

void AppSettingsTest::_verifyFirmwareClassEnumFiltered(Fact* fact)
{
    const QList<QGCMAVLink::FirmwareClass_t> supportedClasses =
        FirmwarePluginManager::instance()->supportedFirmwareClasses();

    const QVariantList enumValues = fact->enumValues();
    QCOMPARE(fact->enumStrings().count(), enumValues.count());
    QVERIFY(!enumValues.isEmpty());

    for (const QVariant& enumValue : enumValues) {
        const auto firmwareClass = static_cast<QGCMAVLink::FirmwareClass_t>(enumValue.toUInt());
        QVERIFY2(supportedClasses.contains(firmwareClass),
                 qPrintable(QStringLiteral("%1 enum offers unsupported firmware class %2")
                                .arg(fact->name())
                                .arg(enumValue.toUInt())));
    }

    QVERIFY2(supportedClasses.contains(static_cast<QGCMAVLink::FirmwareClass_t>(fact->rawValue().toUInt())),
             qPrintable(QStringLiteral("%1 value is an unsupported firmware class %2")
                            .arg(fact->name())
                            .arg(fact->rawValue().toUInt())));
}

void AppSettingsTest::_vehicleSetupVisibleComponentsDefault()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    QVERIFY(appSettings);

    appSettings->resetVehicleSetupVisibleComponents();

    const QStringList expected = QString::fromLatin1(AppSettings::vehicleSetupVisibleComponentsDefault)
                                     .split(QLatin1Char(','), Qt::SkipEmptyParts);
    QCOMPARE(appSettings->vehicleSetupVisibleIdList(), expected);
    QVERIFY(appSettings->isVehicleSetupComponentVisible(QStringLiteral("sensors")));
    QVERIFY(appSettings->isVehicleSetupComponentVisible(QStringLiteral("esc")));
    QVERIFY(appSettings->isVehicleSetupComponentVisible(QStringLiteral("escTelemetry")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("frame")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("radio")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("joystick")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("notARealId")));
    QCOMPARE(appSettings->vehicleSetupComponentSortKey(QStringLiteral("frame")), 0);
    QCOMPARE(appSettings->vehicleSetupComponentSortKey(QStringLiteral("sensors")), 1);
    QCOMPARE(appSettings->vehicleSetupComponentSortKey(QStringLiteral("esc")), 5);
    QCOMPARE(appSettings->vehicleSetupComponentSortKey(QStringLiteral("escTelemetry")), 6);
    QCOMPARE(appSettings->vehicleSetupComponentSortKey(QStringLiteral("motors")), 7);
    QVERIFY(appSettings->vehicleSetupComponentSortKey(QStringLiteral("joystick")) >= 1000);
}

void AppSettingsTest::_vehicleSetupVisibleComponentsToggleAndReset()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    QVERIFY(appSettings);

    appSettings->resetVehicleSetupVisibleComponents();
    QVERIFY(appSettings->isVehicleSetupComponentVisible(QStringLiteral("sensors")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("radio")));

    appSettings->setVehicleSetupComponentVisible(QStringLiteral("sensors"), false);
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("sensors")));

    appSettings->setVehicleSetupComponentVisible(QStringLiteral("radio"), true);
    QVERIFY(appSettings->isVehicleSetupComponentVisible(QStringLiteral("radio")));

    appSettings->setVehicleSetupComponentVisible(QStringLiteral("joystick"), true);
    QVERIFY(appSettings->isVehicleSetupComponentVisible(QStringLiteral("joystick")));

    appSettings->setVehicleSetupComponentVisible(QStringLiteral("notARealId"), true);
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("notARealId")));

    appSettings->resetVehicleSetupVisibleComponents();
    QVERIFY(appSettings->isVehicleSetupComponentVisible(QStringLiteral("sensors")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("frame")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("radio")));
    QVERIFY(!appSettings->isVehicleSetupComponentVisible(QStringLiteral("joystick")));
    QCOMPARE(appSettings->vehicleSetupVisibleComponents()->rawValue().toString(),
             QString::fromLatin1(AppSettings::vehicleSetupVisibleComponentsDefault));
}

void AppSettingsTest::_vehicleSetupResolveComponentId()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    QVERIFY(appSettings);

    QCOMPARE(appSettings->resolveVehicleSetupComponentId(
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMRadioComponent.qml"),
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMRadioComponentSummary.qml")),
             QStringLiteral("radio"));
    QCOMPARE(appSettings->resolveVehicleSetupComponentId(
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/JoystickComponent.qml"),
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/JoystickComponentSummary.qml")),
             QStringLiteral("joystick"));
    QCOMPARE(appSettings->resolveVehicleSetupComponentId(
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMAdvancedTuningCopterComponent.qml"),
                 QString()),
             QStringLiteral("tuningAdvanced"));
    QCOMPARE(appSettings->resolveVehicleSetupComponentId(
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/ActuatorComponent.qml"), QString()),
             QStringLiteral("motors"));
    QCOMPARE(appSettings->resolveVehicleSetupComponentId(
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCTelemetryComponent.qml"),
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCTelemetryComponentSummary.qml")),
             QStringLiteral("escTelemetry"));
    QCOMPARE(appSettings->resolveVehicleSetupComponentId(
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCComponent.qml"),
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCComponentSummary.qml")),
             QStringLiteral("esc"));
    QCOMPARE(appSettings->resolveVehicleSetupComponentId(
                 QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/SyslinkComponent.qml"), QString()),
             QString());
}
