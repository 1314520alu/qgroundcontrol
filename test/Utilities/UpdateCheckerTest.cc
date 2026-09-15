#include "UpdateCheckerTest.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>

#include "AppSettings.h"
#include "SettingsManager.h"
#include "UpdateChecker.h"

UT_REGISTER_TEST(UpdateCheckerTest, TestLabel::Unit)

static const char* kValidJson = R"({
  "version": "v1.2.0",
  "notes": "fix video",
  "android": {
    "url": "https://example.com/miduo-android.apk",
    "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "size": 180000000
  },
  "macos": {
    "url": "https://example.com/miduo.dmg",
    "sha256": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  }
})";

void UpdateCheckerTest::_parseValidManifest()
{
    const std::optional<UpdateManifest> manifest = UpdateChecker::parseManifest(QByteArray(kValidJson));
    QVERIFY(manifest.has_value());
    QCOMPARE(manifest->version, QStringLiteral("v1.2.0"));
    QCOMPARE(manifest->notes, QStringLiteral("fix video"));
    QVERIFY(manifest->android.has_value());
    QCOMPARE(manifest->android->url, QStringLiteral("https://example.com/miduo-android.apk"));
    QCOMPARE(manifest->android->size, 180000000);
    QVERIFY(manifest->macos.has_value());
    QCOMPARE(manifest->macos->size, static_cast<qint64>(-1));
}

void UpdateCheckerTest::_parseRejectsBadJsonAndMissingFields()
{
    QVERIFY(!UpdateChecker::parseManifest(QByteArrayLiteral("{")).has_value());
    QVERIFY(!UpdateChecker::parseManifest(QByteArrayLiteral("{}")).has_value());
    QVERIFY(!UpdateChecker::parseManifest(QByteArrayLiteral(R"({"version":"v1.0.0"})")).has_value());
    QVERIFY(!UpdateChecker::parseManifest(QByteArrayLiteral(R"({"version":"1.0.0","notes":"x"})")).has_value());
    QVERIFY(!UpdateChecker::parseManifest(QByteArrayLiteral(R"({"version":"v1.0","notes":"x"})")).has_value());
}

void UpdateCheckerTest::_parseRejectsHttpPackageUrl()
{
    static const char* const kHttpAndroid = R"({
  "version": "v1.2.0",
  "notes": "fix video",
  "android": {
    "url": "http://example.com/miduo-android.apk",
    "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "size": 180000000
  },
  "macos": {
    "url": "https://example.com/miduo.dmg",
    "sha256": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  }
})";

    const std::optional<UpdateManifest> manifest = UpdateChecker::parseManifest(QByteArray(kHttpAndroid));
    QVERIFY(manifest.has_value());
    QVERIFY(!manifest->android.has_value());
    QVERIFY(manifest->macos.has_value());
    QCOMPARE(manifest->macos->url, QStringLiteral("https://example.com/miduo.dmg"));
}

void UpdateCheckerTest::_isNewerThan()
{
    QVERIFY(UpdateChecker::isNewerThan(QStringLiteral("v1.2.0"), QStringLiteral("v1.1.9")));
    QVERIFY(!UpdateChecker::isNewerThan(QStringLiteral("v1.2.0"), QStringLiteral("v1.2.0")));
    QVERIFY(!UpdateChecker::isNewerThan(QStringLiteral("v1.2.0"), QStringLiteral("v1.2.1")));
    QVERIFY(!UpdateChecker::isNewerThan(QStringLiteral("v1.2.3"), QStringLiteral("v1.2.3-10-gabcdef")));
    QVERIFY(UpdateChecker::isNewerThan(QStringLiteral("v1.2.4"), QStringLiteral("v1.2.3-10-gabcdef")));
    QVERIFY(!UpdateChecker::isNewerThan(QStringLiteral("nope"), QStringLiteral("v1.0.0")));
}

void UpdateCheckerTest::_packageForPlatform()
{
    const UpdateManifest manifest = UpdateChecker::parseManifest(QByteArray(kValidJson)).value();
    const std::optional<UpdatePackage> android = UpdateChecker::packageForPlatform(manifest, QStringLiteral("android"));
    QVERIFY(android.has_value());
    QCOMPARE(android->url.contains(QStringLiteral("apk")), true);
    QVERIFY(UpdateChecker::packageForPlatform(manifest, QStringLiteral("macos")).has_value());
    QVERIFY(!UpdateChecker::packageForPlatform(manifest, QStringLiteral("windows")).has_value());
    QVERIFY(!UpdateChecker::packageForPlatform(manifest, QString()).has_value());

    const UpdateManifest androidOnly =
        UpdateChecker::parseManifest(
            QByteArrayLiteral(
                R"({"version":"v1.0.0","notes":"n","android":{"url":"https://e/a.apk","sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}})"))
            .value();
    QVERIFY(!UpdateChecker::packageForPlatform(androidOnly, QStringLiteral("macos")).has_value());
}

void UpdateCheckerTest::_shouldPrompt()
{
    QVERIFY(UpdateChecker::shouldPrompt(true, false, false));
    QVERIFY(!UpdateChecker::shouldPrompt(true, true, false));
    QVERIFY(!UpdateChecker::shouldPrompt(true, false, true));
    QVERIFY(!UpdateChecker::shouldPrompt(false, false, false));
}

void UpdateCheckerTest::_installerDestinationPathUsesBasename()
{
    const QString path =
        UpdateChecker::installerDestinationPath(QStringLiteral("https://example.com/releases/miduo-android.apk"));
    QCOMPARE(QFileInfo(path).fileName(), QStringLiteral("miduo-android.apk"));
    const QString dir = QFileInfo(path).absolutePath();
    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString appLocal = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QVERIFY(dir == QDir(downloads).absolutePath() || dir == QDir(appLocal).absolutePath());
}

void UpdateCheckerTest::_checkOnStartupEmptyUrlDoesNothing()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    QVERIFY(appSettings);
    Fact* const urlFact = appSettings->updateManifestUrl();
    const QVariant saved = urlFact->rawValue();
    const auto guard = qScopeGuard([urlFact, saved] { urlFact->setRawValue(saved); });
    urlFact->setRawValue(QString());

    UpdateChecker checker;
    checker.checkOnStartup();
    QCOMPARE(checker.updateAvailable(), false);
    QCOMPARE(checker.dialogVisible(), false);
}

void UpdateCheckerTest::_checkOnStartupHttpUrlDoesNothing()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    QVERIFY(appSettings);
    Fact* const urlFact = appSettings->updateManifestUrl();
    const QVariant saved = urlFact->rawValue();
    const auto guard = qScopeGuard([urlFact, saved] { urlFact->setRawValue(saved); });
    urlFact->setRawValue(QStringLiteral("http://example.com/latest.json"));

    UpdateChecker checker;
    checker.checkOnStartup();
    QCOMPARE(checker.updateAvailable(), false);
    QCOMPARE(checker.dialogVisible(), false);
}

void UpdateCheckerTest::_applyNewerAndroidSetsDownload()
{
    UpdateChecker checker;
    QVERIFY(checker.applyManifestJson(QByteArray(kValidJson), QStringLiteral("v1.0.0"), QStringLiteral("android")));
    QCOMPARE(checker.updateAvailable(), true);
    QCOMPARE(checker.remoteVersion(), QStringLiteral("v1.2.0"));
    QCOMPARE(checker.notes(), QStringLiteral("fix video"));
    QCOMPARE(checker.downloadAvailable(), true);
}

void UpdateCheckerTest::_applyNewerWithoutPlatformHidesDownload()
{
    UpdateChecker checker;
    QVERIFY(checker.applyManifestJson(QByteArray(kValidJson), QStringLiteral("v1.0.0"), QStringLiteral("windows")));
    QCOMPARE(checker.updateAvailable(), true);
    QCOMPARE(checker.downloadAvailable(), false);
}

void UpdateCheckerTest::_maybeShowDialogRespectsFlyingAndDismiss()
{
    UpdateChecker checker;
    QVERIFY(checker.applyManifestJson(QByteArray(kValidJson), QStringLiteral("v1.0.0"), QStringLiteral("android")));
    checker.maybeShowDialog(true);
    QCOMPARE(checker.dialogVisible(), false);
    checker.maybeShowDialog(false);
    QCOMPARE(checker.dialogVisible(), true);
    checker.dismiss();
    QCOMPARE(checker.dialogVisible(), false);
    checker.maybeShowDialog(false);
    QCOMPARE(checker.dialogVisible(), false);
}
