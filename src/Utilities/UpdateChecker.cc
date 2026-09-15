#include "UpdateChecker.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

#include "AppSettings.h"
#include "MultiVehicleManager.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(UpdateCheckerLog, "Utilities.UpdateChecker")

Q_APPLICATION_STATIC(UpdateChecker, _updateCheckerInstance);

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent)
{
    if (MultiVehicleManager* const multiVehicleManager = MultiVehicleManager::instance()) {
        connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this,
                &UpdateChecker::_onActiveVehicleChanged);
        _bindVehicle(multiVehicleManager->activeVehicle());
    }
}

UpdateChecker* UpdateChecker::instance()
{
    return _updateCheckerInstance();
}

std::optional<UpdateManifest> UpdateChecker::parseManifest(const QByteArray& json)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }

    const QJsonObject root = document.object();

    if (!root.value(QStringLiteral("version")).isString() || !root.value(QStringLiteral("notes")).isString()) {
        return std::nullopt;
    }

    const QString version = root.value(QStringLiteral("version")).toString();
    int major = 0;
    int minor = 0;
    int patch = 0;
    if (!parseVersion(version, major, minor, patch)) {
        return std::nullopt;
    }

    UpdateManifest manifest;
    manifest.version = version;
    manifest.notes = root.value(QStringLiteral("notes")).toString();

    if (root.value(QStringLiteral("android")).isObject()) {
        manifest.android = _parsePackage(root.value(QStringLiteral("android")).toObject());
    }

    if (root.value(QStringLiteral("macos")).isObject()) {
        manifest.macos = _parsePackage(root.value(QStringLiteral("macos")).toObject());
    }

    return manifest;
}

bool UpdateChecker::parseVersion(const QString& versionString, int& major, int& minor, int& patch)
{
    static const QRegularExpression regExp(QStringLiteral("v(\\d+)\\.(\\d+)\\.(\\d+)"));
    const QRegularExpressionMatch match = regExp.match(versionString);
    if (match.hasMatch() && match.lastCapturedIndex() == 3) {
        major = match.captured(1).toInt();
        minor = match.captured(2).toInt();
        patch = match.captured(3).toInt();
        return true;
    }

    return false;
}

bool UpdateChecker::isNewerThan(const QString& remoteVersion, const QString& localVersion)
{
    int remoteMajor = 0;
    int remoteMinor = 0;
    int remotePatch = 0;
    int localMajor = 0;
    int localMinor = 0;
    int localPatch = 0;

    if (!parseVersion(remoteVersion, remoteMajor, remoteMinor, remotePatch)) {
        return false;
    }

    if (!parseVersion(localVersion, localMajor, localMinor, localPatch)) {
        return false;
    }

    if (remoteMajor != localMajor) {
        return remoteMajor > localMajor;
    }

    if (remoteMinor != localMinor) {
        return remoteMinor > localMinor;
    }

    return remotePatch > localPatch;
}

QString UpdateChecker::currentPlatformKey()
{
#if defined(Q_OS_ANDROID)
    return QStringLiteral("android");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#else
    return QString();
#endif
}

std::optional<UpdatePackage> UpdateChecker::packageForPlatform(const UpdateManifest& manifest,
                                                               const QString& platformKey)
{
    if (platformKey == QStringLiteral("android")) {
        return manifest.android;
    }

    if (platformKey == QStringLiteral("macos")) {
        return manifest.macos;
    }

    return std::nullopt;
}

bool UpdateChecker::shouldPrompt(bool updateAvailable, bool flying, bool sessionDismissed)
{
    return updateAvailable && !flying && !sessionDismissed;
}

QString UpdateChecker::installerDestinationPath(const QString& packageUrl)
{
    const QUrl url = QUrl::fromUserInput(packageUrl);
    QString fileName = url.fileName();
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("miduo-update");
    }

    QString directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (directory.isEmpty()) {
        directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }

    return QDir(directory).filePath(fileName);
}

std::optional<UpdatePackage> UpdateChecker::_parsePackage(const QJsonObject& object)
{
    if (!object.value(QStringLiteral("url")).isString() || !object.value(QStringLiteral("sha256")).isString()) {
        return std::nullopt;
    }

    const QString url = object.value(QStringLiteral("url")).toString();
    const QString sha256 = object.value(QStringLiteral("sha256")).toString();

    if (url.isEmpty() || sha256.length() != 64) {
        return std::nullopt;
    }

    const QUrl parsedUrl(url);
    if (!parsedUrl.isValid() || parsedUrl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        return std::nullopt;
    }

    UpdatePackage package;
    package.url = url;
    package.sha256 = sha256;

    if (object.value(QStringLiteral("size")).isDouble()) {
        package.size = static_cast<qint64>(object.value(QStringLiteral("size")).toDouble());
    } else {
        package.size = -1;
    }

    return package;
}

void UpdateChecker::checkOnStartup()
{
    SettingsManager* const settingsManager = SettingsManager::instance();
    if (!settingsManager) {
        return;
    }

    AppSettings* const appSettings = settingsManager->appSettings();
    if (!appSettings) {
        return;
    }

    Fact* const urlFact = appSettings->updateManifestUrl();
    if (!urlFact) {
        return;
    }

    const QString url = urlFact->rawValue().toString().trimmed();
    const QUrl manifestUrl(url);
    if (url.isEmpty() || !manifestUrl.isValid() ||
        manifestUrl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        qCDebug(UpdateCheckerLog) << "updateManifestUrl empty or not https; skipping update check";
        return;
    }

    if (_manifestDownload) {
        QGCFileDownload* const stale = _manifestDownload;
        _manifestDownload = nullptr;
        disconnect(stale, nullptr, this, nullptr);
        stale->cancel();
        stale->deleteLater();
    }

    _manifestDownload = new QGCFileDownload(this);
    connect(_manifestDownload, &QGCFileDownload::finished, this, &UpdateChecker::_onManifestFinished);
    if (!_manifestDownload->start(url)) {
        qCDebug(UpdateCheckerLog) << "Manifest download failed to start" << _manifestDownload->errorString();
        _manifestDownload->deleteLater();
        _manifestDownload = nullptr;
    }
}

void UpdateChecker::showDialogIfIdle()
{
    maybeShowDialog(_isFlying());
}

void UpdateChecker::maybeShowDialog(bool flying)
{
    _setDialogVisible(shouldPrompt(_updateAvailable, flying, _sessionDismissed));
}

bool UpdateChecker::applyManifestJson(const QByteArray& json, const QString& localVersion, const QString& platformKey)
{
    const std::optional<UpdateManifest> manifest = parseManifest(json);
    if (!manifest.has_value()) {
        return false;
    }

    if (!isNewerThan(manifest->version, localVersion)) {
        return false;
    }

    _setRemoteVersion(manifest->version);
    _setNotes(manifest->notes);
    _setUpdateAvailable(true);

    const std::optional<UpdatePackage> package = packageForPlatform(*manifest, platformKey);
    if (package.has_value()) {
        _package = *package;
    } else {
        _package = UpdatePackage{};
    }
    _setDownloadAvailable(!_package.url.isEmpty());

    return true;
}

void UpdateChecker::dismiss()
{
    _sessionDismissed = true;
    _setDialogVisible(false);
}

void UpdateChecker::download()
{
    if (!_downloadAvailable || _downloading) {
        return;
    }

    _setErrorString(QString());
    _setDownloadProgress(0.0);
    _setDownloadProgressKnown(_package.size > 0);
    _setLocalInstallerPath(QString());

    if (_packageDownload) {
        QGCFileDownload* const stale = _packageDownload;
        _packageDownload = nullptr;
        disconnect(stale, nullptr, this, nullptr);
        stale->cancel();
        stale->deleteLater();
    }

    _packageDownload = new QGCFileDownload(this);
    _packageDownload->setOutputPath(installerDestinationPath(_package.url));
    _packageDownload->setExpectedHash(_package.sha256);
    _packageDownload->setAutoDecompress(false);
    connect(_packageDownload, &QGCFileDownload::downloadProgress, this, &UpdateChecker::_onPackageDownloadProgress);
    connect(_packageDownload, &QGCFileDownload::finished, this, &UpdateChecker::_onPackageFinished);

    _setDownloading(true);
    if (!_packageDownload->start(_package.url)) {
        _setDownloading(false);
        _setErrorString(_packageDownload->errorString());
        _packageDownload->deleteLater();
        _packageDownload = nullptr;
    }
}

void UpdateChecker::cancelDownload()
{
    if (!_packageDownload) {
        return;
    }

    const QString incompletePath = !_packageDownload->localPath().isEmpty() ? _packageDownload->localPath()
                                                                            : installerDestinationPath(_package.url);
    _packageDownload->cancel();
    if (!incompletePath.isEmpty()) {
        QFile::remove(incompletePath);
    }
}

void UpdateChecker::openInstaller()
{
    if (_localInstallerPath.isEmpty()) {
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(_localInstallerPath))) {
        _setErrorString(tr("Saved to %1. Install it from Files.").arg(_localInstallerPath));
    }
}

void UpdateChecker::_onPackageDownloadProgress(qint64 bytesReceived, qint64 totalBytes)
{
    qint64 total = totalBytes;
    if (total <= 0 && _package.size > 0) {
        total = _package.size;
    }

    if (total > 0) {
        _setDownloadProgressKnown(true);
        _setDownloadProgress(static_cast<qreal>(bytesReceived) / static_cast<qreal>(total));
        return;
    }

    _setDownloadProgressKnown(false);
}

void UpdateChecker::_onManifestFinished(bool success, const QString& localFile, const QString& errorMsg)
{
    QGCFileDownload* const download = qobject_cast<QGCFileDownload*>(sender());
    if (!download || download != _manifestDownload) {
        if (download) {
            download->deleteLater();
        }
        return;
    }

    _manifestDownload = nullptr;
    download->deleteLater();

    if (!success) {
        qCDebug(UpdateCheckerLog) << "Manifest download failed" << errorMsg;
        return;
    }

    QFile file(localFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(UpdateCheckerLog) << "Failed to read manifest" << localFile;
        return;
    }

    applyManifestJson(file.readAll(), QCoreApplication::applicationVersion(), currentPlatformKey());
    showDialogIfIdle();
}

void UpdateChecker::_onPackageFinished(bool success, const QString& localFile, const QString& errorMsg)
{
    QGCFileDownload* const download = qobject_cast<QGCFileDownload*>(sender());
    if (!download || download != _packageDownload) {
        if (download) {
            download->deleteLater();
        }
        return;
    }

    _packageDownload = nullptr;
    download->deleteLater();

    _setDownloading(false);

    if (success) {
        _setLocalInstallerPath(localFile);
        _setErrorString(QString());
        return;
    }

    _setErrorString(errorMsg);
    const QString incompletePath = !localFile.isEmpty() ? localFile : installerDestinationPath(_package.url);
    if (!incompletePath.isEmpty()) {
        QFile::remove(incompletePath);
    }
}

void UpdateChecker::_onActiveVehicleChanged(Vehicle* vehicle)
{
    _bindVehicle(vehicle);
    if (_updateAvailable && !_sessionDismissed) {
        maybeShowDialog(_isFlying());
    }
}

void UpdateChecker::_onFlyingChanged(bool flying)
{
    Q_UNUSED(flying);
    if (_updateAvailable && !_sessionDismissed) {
        maybeShowDialog(_isFlying());
    }
}

bool UpdateChecker::_isFlying() const
{
    MultiVehicleManager* const multiVehicleManager = MultiVehicleManager::instance();
    if (!multiVehicleManager) {
        return false;
    }

    Vehicle* const vehicle = multiVehicleManager->activeVehicle();
    return vehicle && vehicle->flying();
}

void UpdateChecker::_bindVehicle(Vehicle* vehicle)
{
    if (_trackedVehicle) {
        disconnect(_trackedVehicle, &Vehicle::flyingChanged, this, &UpdateChecker::_onFlyingChanged);
    }

    _trackedVehicle = vehicle;
    if (!_trackedVehicle) {
        return;
    }

    connect(_trackedVehicle, &Vehicle::flyingChanged, this, &UpdateChecker::_onFlyingChanged);
}

void UpdateChecker::_setDialogVisible(bool visible)
{
    if (_dialogVisible == visible) {
        return;
    }
    _dialogVisible = visible;
    emit dialogVisibleChanged();
}

void UpdateChecker::_setErrorString(const QString& error)
{
    if (_errorString == error) {
        return;
    }
    _errorString = error;
    emit errorStringChanged();
}

void UpdateChecker::_setUpdateAvailable(bool available)
{
    if (_updateAvailable == available) {
        return;
    }
    _updateAvailable = available;
    emit updateAvailableChanged();
}

void UpdateChecker::_setRemoteVersion(const QString& version)
{
    if (_remoteVersion == version) {
        return;
    }
    _remoteVersion = version;
    emit remoteVersionChanged();
}

void UpdateChecker::_setNotes(const QString& notes)
{
    if (_notes == notes) {
        return;
    }
    _notes = notes;
    emit notesChanged();
}

void UpdateChecker::_setDownloadAvailable(bool available)
{
    if (_downloadAvailable == available) {
        return;
    }
    _downloadAvailable = available;
    emit downloadAvailableChanged();
}

void UpdateChecker::_setDownloadProgress(qreal progress)
{
    if (qFuzzyCompare(_downloadProgress, progress)) {
        return;
    }
    _downloadProgress = progress;
    emit downloadProgressChanged();
}

void UpdateChecker::_setDownloadProgressKnown(bool known)
{
    if (_downloadProgressKnown == known) {
        return;
    }
    _downloadProgressKnown = known;
    emit downloadProgressKnownChanged();
}

void UpdateChecker::_setDownloading(bool downloading)
{
    if (_downloading == downloading) {
        return;
    }
    _downloading = downloading;
    emit downloadingChanged();
}

void UpdateChecker::_setLocalInstallerPath(const QString& path)
{
    if (_localInstallerPath == path) {
        return;
    }
    _localInstallerPath = path;
    emit localInstallerPathChanged();
}
