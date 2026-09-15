#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include <optional>

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

    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString remoteVersion READ remoteVersion NOTIFY remoteVersionChanged)
    Q_PROPERTY(QString notes READ notes NOTIFY notesChanged)
    Q_PROPERTY(bool downloadAvailable READ downloadAvailable NOTIFY downloadAvailableChanged)
    Q_PROPERTY(qreal downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(bool downloadProgressKnown READ downloadProgressKnown NOTIFY downloadProgressKnownChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString localInstallerPath READ localInstallerPath NOTIFY localInstallerPathChanged)
    Q_PROPERTY(bool dialogVisible READ dialogVisible NOTIFY dialogVisibleChanged)

    friend class UpdateCheckerTest;

public:
    explicit UpdateChecker(QObject* parent = nullptr);

    static UpdateChecker* instance();

    [[nodiscard]] static std::optional<UpdateManifest> parseManifest(const QByteArray& json);
    [[nodiscard]] static bool parseVersion(const QString& versionString, int& major, int& minor, int& patch);
    [[nodiscard]] static bool isNewerThan(const QString& remoteVersion, const QString& localVersion);
    [[nodiscard]] static QString currentPlatformKey();
    [[nodiscard]] static std::optional<UpdatePackage> packageForPlatform(const UpdateManifest& manifest,
                                                                         const QString& platformKey);
    [[nodiscard]] static bool shouldPrompt(bool updateAvailable, bool flying, bool sessionDismissed);
    [[nodiscard]] static QString installerDestinationPath(const QString& packageUrl);

    bool updateAvailable() const { return _updateAvailable; }

    QString remoteVersion() const { return _remoteVersion; }

    QString notes() const { return _notes; }

    bool downloadAvailable() const { return _downloadAvailable; }

    qreal downloadProgress() const { return _downloadProgress; }

    bool downloadProgressKnown() const { return _downloadProgressKnown; }

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
    bool applyManifestJson(const QByteArray& json, const QString& localVersion, const QString& platformKey);

signals:
    void updateAvailableChanged();
    void remoteVersionChanged();
    void notesChanged();
    void downloadAvailableChanged();
    void downloadProgressChanged();
    void downloadProgressKnownChanged();
    void downloadingChanged();
    void errorStringChanged();
    void localInstallerPathChanged();
    void dialogVisibleChanged();

private slots:
    void _onManifestFinished(bool success, const QString& localFile, const QString& errorMsg);
    void _onPackageFinished(bool success, const QString& localFile, const QString& errorMsg);
    void _onPackageDownloadProgress(qint64 bytesReceived, qint64 totalBytes);
    void _onActiveVehicleChanged(Vehicle* vehicle);
    void _onFlyingChanged(bool flying);

private:
    [[nodiscard]] static std::optional<UpdatePackage> _parsePackage(const QJsonObject& object);

    void _setDialogVisible(bool visible);
    void _setErrorString(const QString& error);
    void _setUpdateAvailable(bool available);
    void _setRemoteVersion(const QString& version);
    void _setNotes(const QString& notes);
    void _setDownloadAvailable(bool available);
    void _setDownloadProgress(qreal progress);
    void _setDownloadProgressKnown(bool known);
    void _setDownloading(bool downloading);
    void _setLocalInstallerPath(const QString& path);
    [[nodiscard]] bool _isFlying() const;
    void _bindVehicle(Vehicle* vehicle);

    bool _updateAvailable = false;
    bool _downloadAvailable = false;
    bool _downloading = false;
    bool _dialogVisible = false;
    bool _sessionDismissed = false;
    bool _downloadProgressKnown = false;
    qreal _downloadProgress = 0.0;
    QString _remoteVersion;
    QString _notes;
    QString _errorString;
    QString _localInstallerPath;
    UpdatePackage _package;
    QPointer<Vehicle> _trackedVehicle;
    QGCFileDownload* _manifestDownload = nullptr;
    QGCFileDownload* _packageDownload = nullptr;
};
