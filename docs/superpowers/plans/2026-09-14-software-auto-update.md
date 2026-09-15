# 米多地面站 Software Update Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** On launch, fetch an Aliyun OSS `latest.json` and, when a newer Android/macOS build exists and the vehicle is not flying, show a compact dialog that can download the installer for the user to install.

**Architecture:** Replace `QGCApplication::_checkForNewVersion()` (AWS / qgroundcontrol.com) with `UpdateChecker`. Static parse/compare helpers are unit-tested without HTTP. The singleton fetches the manifest, gates the dialog on `Vehicle::flying()`, and downloads through existing `QGCFileDownload` (progress, cancel, SHA-256). QML on `MainWindow` shows the dialog. Manifest URL is `AppSettings.updateManifestUrl` (empty = off).

**Tech Stack:** Qt 6, QGC Fact settings, `QGCFileDownload`, QML `QGCPopupDialog`, CTest `UnitTest`.

## Global Constraints

- Landscape remotes first (Skydroid G20 / SIYI MK32): compact dialog, `ScreenTools` sizes, `QGCPalette` / QGC controls.
- No `Q_ASSERT`; null-check `activeVehicle()` before `flying()`.
- No live OSS in unit tests. No Windows/Linux/iOS packages. No silent install. No Gitee.
- Do not commit unless the user asks.
- Version compare uses `v(\d+)\.(\d+)\.(\d+)`; git suffix ignored.
- Check every launch except unit tests, including daily builds.
- User-visible strings use `qsTr()`; Chinese in `translations/qgc_source_zh_CN.ts` and `translations/qgc_json_zh_CN.ts`.

**Spec:** `docs/superpowers/specs/2026-09-14-software-auto-update-design.md`

---

### Task 1: Manifest parse, version compare, prompt gate (TDD)

**Files:**
- Create: `src/Utilities/UpdateChecker.h`
- Create: `src/Utilities/UpdateChecker.cc`
- Create: `test/Utilities/UpdateCheckerTest.h`
- Create: `test/Utilities/UpdateCheckerTest.cc`
- Modify: `src/Utilities/CMakeLists.txt` (add `UpdateChecker.cc/.h` to `target_sources`)
- Modify: `test/Utilities/CMakeLists.txt` (add test sources + `add_qgc_test`)

**Interfaces:**
- Consumes: Qt JSON, `QRegularExpression`
- Produces: `UpdateChecker::parseManifest`, `parseVersion`, `isNewerThan`, `currentPlatformKey`, `packageForPlatform`, `shouldPrompt`, `installerDestinationPath`; structs `UpdatePackage`, `UpdateManifest`

- [ ] **Step 1: Write the failing test**

`test/Utilities/UpdateCheckerTest.h`:

```cpp
#pragma once

#include "UnitTest.h"

class UpdateCheckerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseValidManifest();
    void _parseRejectsBadJsonAndMissingFields();
    void _isNewerThan();
    void _packageForPlatform();
    void _shouldPrompt();
    void _installerDestinationPathUsesBasename();
};
```

`test/Utilities/UpdateCheckerTest.cc` (full file):

```cpp
#include "UpdateCheckerTest.h"

#include "UpdateChecker.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>

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
    const std::optional<UpdatePackage> android =
        UpdateChecker::packageForPlatform(manifest, QStringLiteral("android"));
    QVERIFY(android.has_value());
    QCOMPARE(android->url.contains(QStringLiteral("apk")), true);
    QVERIFY(UpdateChecker::packageForPlatform(manifest, QStringLiteral("macos")).has_value());
    QVERIFY(!UpdateChecker::packageForPlatform(manifest, QStringLiteral("windows")).has_value());
    QVERIFY(!UpdateChecker::packageForPlatform(manifest, QString()).has_value());

    const UpdateManifest androidOnly = UpdateChecker::parseManifest(QByteArrayLiteral(
        R"({"version":"v1.0.0","notes":"n","android":{"url":"https://e/a.apk","sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}})")).value();
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
```

Add to `src/Utilities/CMakeLists.txt` `target_sources` list:

```cmake
        UpdateChecker.cc
        UpdateChecker.h
```

Add to `test/Utilities/CMakeLists.txt` `target_sources` and:

```cmake
add_qgc_test(UpdateCheckerTest LABELS Unit Utilities)
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest -R UpdateCheckerTest --output-on-failure`  
Expected: FAIL (type not linked / methods missing)

- [ ] **Step 3: Implement parse helpers in `UpdateChecker`**

`src/Utilities/UpdateChecker.h` (this task only needs the static API; leave instance methods for Task 3 as declarations if the header is otherwise incomplete — prefer adding only what Task 1 tests):

```cpp
#pragma once

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class UpdateCheckerTest;
class Vehicle;
class QGCFileDownload;

struct UpdatePackage
{
    QString url;
    QString sha256;
    qint64 size = -1;
};

struct UpdateManifest
{
    QString version;
    QString notes;
    std::optional<UpdatePackage> android;
    std::optional<UpdatePackage> macos;
};

class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    friend class UpdateCheckerTest;

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    static UpdateChecker *instance();

    [[nodiscard]] static std::optional<UpdateManifest> parseManifest(const QByteArray &json);
    [[nodiscard]] static bool parseVersion(const QString &versionString, int &major, int &minor, int &patch);
    [[nodiscard]] static bool isNewerThan(const QString &remoteVersion, const QString &localVersion);
    [[nodiscard]] static QString currentPlatformKey();
    [[nodiscard]] static std::optional<UpdatePackage> packageForPlatform(const UpdateManifest &manifest,
                                                                         const QString &platformKey);
    [[nodiscard]] static bool shouldPrompt(bool updateAvailable, bool flying, bool sessionDismissed);
    [[nodiscard]] static QString installerDestinationPath(const QString &packageUrl);

private:
    [[nodiscard]] static std::optional<UpdatePackage> _parsePackage(const QJsonObject &object);
};
```

Implementation notes for `UpdateChecker.cc`:

- Logging: `QGC_LOGGING_CATEGORY(UpdateCheckerLog, "Utilities.UpdateChecker")`
- `parseVersion`: same regex as old `QGCApplication::_parseVersionText`: `v(\\d+)\\.(\\d+)\\.(\\d+)`
- `isNewerThan`: parse both; if either fails, return false; then major/minor/patch lexicographic greater
- `parseManifest`: `QJsonDocument::fromJson`; require object with string `version` that `parseVersion` accepts and string `notes`; optional `android`/`macos` objects via `_parsePackage`
- `_parsePackage`: require non-empty `url` and `sha256` length 64; `size` optional number else -1; invalid object → nullopt (that platform omitted, not whole manifest)
- `currentPlatformKey`: `Q_OS_ANDROID` → `"android"`; `Q_OS_MACOS` → `"macos"`; else empty
- `packageForPlatform`: `android` / `macos` match the optional fields; anything else nullopt
- `shouldPrompt`: `updateAvailable && !flying && !sessionDismissed`
- `installerDestinationPath`: `QUrl::fromUserInput` / `QUrl(packageUrl).fileName()`; directory `DownloadLocation` if non-empty else `AppLocalDataLocation`; empty basename → `miduo-update`
- `instance()`: `Q_APPLICATION_STATIC(UpdateChecker, _updateCheckerInstance)` + return `_updateCheckerInstance()`
- Constructor can be empty this task

- [ ] **Step 4: Run tests and make sure they pass**

Run: `ctest -R UpdateCheckerTest --output-on-failure`  
Expected: PASS

- [ ] **Step 5: Commit** — skip unless the user asks.

---

### Task 2: Manifest URL setting

**Files:**
- Modify: `src/Settings/App.SettingsGroup.json` (add fact after `aircraftModel`)
- Modify: `src/Settings/AppSettings.h` (`DEFINE_SETTINGFACT(updateManifestUrl)` after `aircraftModel`)
- Modify: `src/Settings/AppSettings.cc` (`DECLARE_SETTINGSFACT(AppSettings, updateManifestUrl)` after `aircraftModel`)
- Modify: `src/AppSettings/pages/General.SettingsUI.json` (new group after Aircraft Model)
- Modify: `test/Settings/AppSettingsTest.h` / `.cc`
- Modify: `translations/qgc_json_zh_CN.ts` (labels)

**Interfaces:**
- Consumes: existing Fact / settings QML generator
- Produces: `appSettings.updateManifestUrl` string Fact, default `""`

- [ ] **Step 1: Write the failing test**

Add to `AppSettingsTest.h`:

```cpp
    void _updateManifestUrlDefaultEmpty();
```

Add to `AppSettingsTest.cc`:

```cpp
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
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest -R AppSettingsTest --output-on-failure`  
Expected: FAIL (`updateManifestUrl` missing)

- [ ] **Step 3: Add Fact + General UI**

`App.SettingsGroup.json` fact:

```json
        {
            "name": "updateManifestUrl",
            "shortDesc": "HTTPS URL of latest.json on Aliyun OSS. Empty disables update checks.",
            "longDesc": "Full URL to the software-update manifest. Leave empty to skip checking. Android and macOS installer URLs are inside that JSON.",
            "type": "string",
            "default": "",
            "label": "Update Manifest URL",
            "keywords": "update,oss,latest.json,software update"
        },
```

`General.SettingsUI.json` — insert after the Aircraft Model group:

```json
        {
            "heading": "Software Update",
            "keywords": ["update", "oss", "latest.json", "software update", "更新"],
            "controls": [
                {
                    "setting": "appSettings.updateManifestUrl",
                    "control": "textfield"
                }
            ]
        },
```

Header/cc: `DEFINE_SETTINGFACT(updateManifestUrl)` and `DECLARE_SETTINGSFACT(AppSettings, updateManifestUrl)`.

Rebuild so the settings QML generator regenerates General. Chinese JSON strings:

- `Update Manifest URL` → `更新清单地址`
- `Software Update` → `软件更新`

- [ ] **Step 4: Run tests**

Run: `ctest -R AppSettingsTest --output-on-failure`  
Expected: PASS

- [ ] **Step 5: Commit** — skip unless the user asks.

---

### Task 3: UpdateChecker instance — apply manifest, dialog flags, download wiring

**Files:**
- Modify: `src/Utilities/UpdateChecker.h`
- Modify: `src/Utilities/UpdateChecker.cc`
- Modify: `test/Utilities/UpdateCheckerTest.h`
- Modify: `test/Utilities/UpdateCheckerTest.cc`

**Interfaces:**
- Consumes: Task 1 statics, Task 2 `updateManifestUrl`, `QGCFileDownload`, `MultiVehicleManager`, `Vehicle::flying()`
- Produces: QML properties `updateAvailable`, `remoteVersion`, `notes`, `downloadAvailable`, `downloadProgress`, `downloading`, `errorString`, `localInstallerPath`, `dialogVisible`; methods `checkOnStartup()`, `showDialogIfIdle()`, `maybeShowDialog(bool flying)`, `download()`, `cancelDownload()`, `dismiss()`, `openInstaller()`, `applyManifestJson(...)`

- [ ] **Step 1: Write the failing tests**

Add slots:

```cpp
    void _checkOnStartupEmptyUrlDoesNothing();
    void _applyNewerAndroidSetsDownload();
    void _applyNewerWithoutPlatformHidesDownload();
    void _maybeShowDialogRespectsFlyingAndDismiss();
```

```cpp
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
```

Need `#include "AppSettings.h"` and `#include "SettingsManager.h"` plus `QScopeGuard`.

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest -R UpdateCheckerTest --output-on-failure`  
Expected: FAIL (instance API missing)

- [ ] **Step 3: Implement instance API**

Add to the class (header):

```cpp
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString remoteVersion READ remoteVersion NOTIFY remoteVersionChanged)
    Q_PROPERTY(QString notes READ notes NOTIFY notesChanged)
    Q_PROPERTY(bool downloadAvailable READ downloadAvailable NOTIFY downloadAvailableChanged)
    Q_PROPERTY(qreal downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString localInstallerPath READ localInstallerPath NOTIFY localInstallerPathChanged)
    Q_PROPERTY(bool dialogVisible READ dialogVisible NOTIFY dialogVisibleChanged)

    bool updateAvailable() const { return _updateAvailable; }
    QString remoteVersion() const { return _remoteVersion; }
    QString notes() const { return _notes; }
    bool downloadAvailable() const { return _downloadAvailable; }
    qreal downloadProgress() const { return _downloadProgress; }
    bool downloading() const { return _downloading; }
    QString errorString() const { return _errorString; }
    QString localInstallerPath() const { return _localInstallerPath; }
    bool dialogVisible() const { return _dialogVisible; }

    Q_INVOKABLE void checkOnStartup();
    Q_INVOKABLE void showDialogIfIdle();
    Q_INVOKABLE void download();
    Q_INVOKABLE void cancelDownload();
    Q_INVOKABLE void dismiss();
    Q_INVOKABLE void openInstaller();
    void maybeShowDialog(bool flying);
    bool applyManifestJson(const QByteArray &json, const QString &localVersion, const QString &platformKey);

signals:
    void updateAvailableChanged();
    void remoteVersionChanged();
    void notesChanged();
    void downloadAvailableChanged();
    void downloadProgressChanged();
    void downloadingChanged();
    void errorStringChanged();
    void localInstallerPathChanged();
    void dialogVisibleChanged();

private slots:
    void _onManifestFinished(bool success, const QString &localFile, const QString &errorMsg);
    void _onPackageFinished(bool success, const QString &localFile, const QString &errorMsg);
    void _onActiveVehicleChanged(Vehicle *vehicle);
    void _onFlyingChanged(bool flying);

private:
    void _setDialogVisible(bool visible);
    void _setErrorString(const QString &error);
    [[nodiscard]] bool _isFlying() const;
    void _bindVehicle(Vehicle *vehicle);

    bool _updateAvailable = false;
    bool _downloadAvailable = false;
    bool _downloading = false;
    bool _dialogVisible = false;
    bool _sessionDismissed = false;
    qreal _downloadProgress = 0.0;
    QString _remoteVersion;
    QString _notes;
    QString _errorString;
    QString _localInstallerPath;
    UpdatePackage _package;
    QPointer<Vehicle> _trackedVehicle;
    QGCFileDownload *_manifestDownload = nullptr;
    QGCFileDownload *_packageDownload = nullptr;
```

Behavior:

- `checkOnStartup`: if `updateManifestUrl` trimmed empty → return (log debug). Else start `QGCFileDownload` GET of that URL (no expected hash). Connect `finished` → `_onManifestFinished`.
- `_onManifestFinished`: on failure log only (`qCDebug`), do not set `updateAvailable`. On success read file bytes, `applyManifestJson(..., QCoreApplication::applicationVersion(), currentPlatformKey())`, then `showDialogIfIdle()`.
- `showDialogIfIdle()`: `maybeShowDialog(_isFlying())`.
- `applyManifestJson`: parse; if nullopt return false; if `!isNewerThan(manifest.version, localVersion)` return false (leave flags false); else set version/notes, `updateAvailable=true`, set `_package` from `packageForPlatform`, `downloadAvailable = _package.url` non-empty. Return true.
- `maybeShowDialog(flying)`: `_setDialogVisible(shouldPrompt(_updateAvailable, flying, _sessionDismissed))`. Used by tests to inject flying without a `Vehicle`.
- `_isFlying`: `Vehicle* v = MultiVehicleManager::instance()->activeVehicle(); return v && v->flying();`
- `_bindVehicle`: disconnect previous; null-check; connect `flyingChanged` and `MultiVehicleManager::activeVehicleChanged` once from constructor to `_onActiveVehicleChanged` / `_onFlyingChanged`; those call `maybeShowDialog(_isFlying())` only when `_updateAvailable && !_sessionDismissed`.
- `dismiss`: `_sessionDismissed = true`; `_setDialogVisible(false)`.
- `download`: if `!_downloadAvailable` or `_downloading` return; clear error; create `QGCFileDownload`; `setOutputPath(installerDestinationPath(_package.url))`; `setExpectedHash(_package.sha256)`; `setAutoDecompress(false)`; connect progress and finished; `start(_package.url)`. Set `_downloading`.
- `_onPackageFinished`: `_downloading=false`; success → `_localInstallerPath`, `_setErrorString({})`, keep dialog visible; failure → `_setErrorString(errorMsg)`, `QFile::remove` incomplete path if any.
- `cancelDownload`: `_packageDownload->cancel()`; delete incomplete file.
- `openInstaller`: if path empty return; `QDesktopServices::openUrl(QUrl::fromLocalFile(_localInstallerPath))`; if that returns false, `_setErrorString` with the path.
- Property setters only emit when the value changes.
- Constructor: connect `MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged`.

Include `QGCFileDownload.h`, `MultiVehicleManager.h`, `Vehicle.h`, `SettingsManager.h`, `AppSettings.h`. Null-check `SettingsManager::instance()` / `appSettings()` / fact before use.

- [ ] **Step 4: Run tests**

Run: `ctest -R UpdateCheckerTest --output-on-failure`  
Expected: PASS

- [ ] **Step 5: Commit** — skip unless the user asks.

---

### Task 4: Replace upstream version check and expose to QML

**Files:**
- Modify: `src/QGCApplication.cc` (call `UpdateChecker::instance()->checkOnStartup()`; delete `_checkForNewVersion`, `_qgcCurrentStableVersionDownloadComplete`, `_parseVersionText` and their uses)
- Modify: `src/QGCApplication.h` (remove those methods and `_majorVersion` / `_minorVersion` / `_buildVersion`)
- Modify: `src/QmlControls/QGroundControlQmlGlobal.h`
- Modify: `src/QmlControls/QGroundControlQmlGlobal.cc`
- Do **not** remove `QGCCorePlugin::stableVersionCheckFileUrl()` / `stableDownloadLocation()`

**Interfaces:**
- Consumes: `UpdateChecker::instance()`, `checkOnStartup()`
- Produces: `QGroundControl.updateChecker` in QML

- [ ] **Step 1: Swap the startup call**

In `QGCApplication` init, replace:

```cpp
#ifndef QGC_DAILY_BUILD
    _checkForNewVersion();
#endif
```

with:

```cpp
    if (!_runningUnitTests) {
        UpdateChecker::instance()->checkOnStartup();
    }
```

`#include "UpdateChecker.h"`. Delete the three old methods and the three version member fields from `.cc`/`.h`.

- [ ] **Step 2: Expose on `QGroundControl`**

Header: `#include` / `Q_MOC_INCLUDE("UpdateChecker.h")`, forward-declare `class UpdateChecker;`, add:

```cpp
    Q_PROPERTY(UpdateChecker* updateChecker READ updateChecker CONSTANT)
    UpdateChecker* updateChecker() { return _updateChecker; }
```

and member `UpdateChecker* _updateChecker = nullptr;`

`.cc` constructor: `_updateChecker(UpdateChecker::instance())` (add to the initializer list like `_linkManager`). `#include "UpdateChecker.h"`.

- [ ] **Step 3: Build**

Run: `just build` (or the existing incremental CMake build)  
Expected: links; unit tests still pass

Run: `ctest -R "UpdateCheckerTest|AppSettingsTest" --output-on-failure`  
Expected: PASS

- [ ] **Step 4: Commit** — skip unless the user asks.

---

### Task 5: Dialog, Android install permission, translations

**Files:**
- Create: `src/QmlControls/UpdateAvailableDialog.qml`
- Modify: `src/QmlControls/CMakeLists.txt` (`QML_FILES` add `UpdateAvailableDialog.qml`)
- Modify: `src/MainWindow/MainWindow.qml`
- Modify: `cmake/platform/Android.cmake` (`REQUEST_INSTALL_PACKAGES`)
- Modify: `translations/qgc_source_zh_CN.ts`

**Interfaces:**
- Consumes: `QGroundControl.updateChecker` properties/methods from Task 3
- Produces: compact landscape dialog; Android can open the downloaded APK

- [ ] **Step 1: Dialog QML**

`UpdateAvailableDialog.qml` — `QGCPopupDialog`, `buttons: 0` (actions in the body). Bind to `QGroundControl.updateChecker`.

Layout (short landscape): title `qsTr("New Version Available")`; `QGCLabel` remote version; `QGCFlickable` + wrapped notes (`ScreenTools.defaultFontPixelHeight * 6` max height); error label if `errorString` non-empty; `QGCButton` row:

- Not downloading, download available, no local path yet: `qsTr("Download")` → `updateChecker.download()`; `qsTr("Later")` → `updateChecker.dismiss(); close()`
- Downloading: `QProgressBar` or `QGC` progress using `downloadProgress`; `qsTr("Cancel")` → `updateChecker.cancelDownload()`
- Failed: `qsTr("Retry")` → `download()`; `qsTr("Later")` → dismiss + close
- Success (`localInstallerPath` non-empty): `qsTr("Install")` → `updateChecker.openInstaller()`; `qsTr("Later")` → dismiss + close

`onClosed`: `updateChecker.dismiss()` (idempotent). `destroyOnClose: true`.

Widths: `Layout.preferredWidth: Math.min(mainWindow.width * 0.7, ScreenTools.defaultFontPixelWidth * 60)` so G20/MK32 keep Later/Download on-screen.

- [ ] **Step 2: Host on `MainWindow.qml`**

After the simple-message-dialog block, add `QGCPopupDialogFactory` + `Component` for `UpdateAvailableDialog`, and:

```qml
    Connections {
        target: QGroundControl.updateChecker

        function onDialogVisibleChanged() {
            if (QGroundControl.updateChecker.dialogVisible) {
                updateAvailableDialogFactory.open()
            }
        }
    }
```

Guard against stacking: if a dialog is already open, do not `open()` again (factory creates a new instance). Track `property var _updateDialog: null` and only open when `_updateDialog` is null; on closed, set it null. The factory `open()` return value can be stored.

- [ ] **Step 3: Android permission**

In `cmake/platform/Android.cmake` next to `INTERNET`:

```cmake
qt_add_android_permission(${CMAKE_PROJECT_NAME} NAME android.permission.REQUEST_INSTALL_PACKAGES)
```

- [ ] **Step 4: Translations**

`qgc_source_zh_CN.ts` context `UpdateAvailableDialog`:

| source | zh_CN |
| --- | --- |
| New Version Available | 发现新版本 |
| Later | 稍后 |
| Download | 下载 |
| Retry | 重试 |
| Cancel | 取消 |
| Install | 安装 |

If `openInstaller` failure string is in C++ (`tr("Saved to %1. Install it from Files.")`) add that under `UpdateChecker`.

- [ ] **Step 5: Build and unit tests**

Run: `just build` then `ctest -R "UpdateCheckerTest|AppSettingsTest" --output-on-failure`  
Expected: PASS. QML loads (qmllint via `just lint` if the environment has it).

Manual after OSS URL exists: ground launch shows dialog; flying does not until land; Download writes APK/DMG and Install/open works.

- [ ] **Step 6: Commit** — skip unless the user asks.

---

## Self-review (spec coverage)

| Spec item | Task |
| --- | --- |
| OSS `latest.json` fields / parse failures silent | 1, 3 |
| `vX.Y.Z` compare, git suffix | 1 |
| Empty URL skips check | 2, 3 |
| Flying defers; land/disconnect prompts; Later session-dismiss | 3 |
| Download on tap; sha256; delete incomplete | 3 (`QGCFileDownload::setExpectedHash`, cancel/fail remove) |
| Android APK / macOS DMG only | 1 `packageForPlatform` |
| Stop AWS `_checkForNewVersion` | 4 |
| Keep `QGCCorePlugin` version-check API | 4 |
| `QGroundControl.updateChecker` | 4 |
| Compact dialog + zh copy | 5 |
| `REQUEST_INSTALL_PACKAGES` | 5 |
| Unit tests no live OSS | 1, 2, 3 |
| Daily builds still check | 4 (removed `QGC_DAILY_BUILD` guard) |
| Settings Software Update group | 2 |
