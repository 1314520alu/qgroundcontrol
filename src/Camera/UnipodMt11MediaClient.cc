#include "UnipodMt11MediaClient.h"

#include "AppSettings.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

#include <algorithm>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

QGC_LOGGING_CATEGORY(UnipodMt11MediaClientLog, "Camera.UnipodMt11MediaClient")

UnipodMt11MediaFile::UnipodMt11MediaFile(const QString &name, const QString &url, bool isVideo, QObject *parent)
    : QObject(parent)
    , _name(name)
    , _url(url)
    , _isVideo(isVideo)
{
}

UnipodMt11MediaClient::UnipodMt11MediaClient(QObject *parent)
    : QObject(parent)
    , _networkManager(new QNetworkAccessManager(this))
{
    qCDebug(UnipodMt11MediaClientLog) << this;
}

UnipodMt11MediaClient::~UnipodMt11MediaClient()
{
    _abortListRequest();
    cancelDownload();
    _clearFiles();
    qCDebug(UnipodMt11MediaClientLog) << this;
}

void UnipodMt11MediaClient::setReady(bool ready)
{
    if (_ready == ready) {
        return;
    }
    _ready = ready;
    if (!_ready) {
        _abortListRequest();
        cancelDownload();
        _clearFiles();
        _directoryPaths.clear();
        _directoryIndex = 0;
        _setErrorString(QString());
        _setHasMore(false);
        _nextStart = 0;
    }
    emit readyChanged();
}

QUrl UnipodMt11MediaClient::baseUrl() const
{
    VideoSettings *videoSettings = SettingsManager::instance() ? SettingsManager::instance()->videoSettings() : nullptr;
    const QString rtsp = videoSettings ? QString::fromLatin1(VideoSettings::unipodMT11RtspUrl) : QString();
    return defaultBaseUrlFromRtsp(rtsp);
}

QUrl UnipodMt11MediaClient::defaultBaseUrlFromRtsp(const QString &rtspUrl)
{
    const QUrl rtsp(rtspUrl);
    QUrl http;
    http.setScheme(QStringLiteral("http"));
    http.setHost(rtsp.host().isEmpty() ? QStringLiteral("192.168.144.25") : rtsp.host());
    http.setPort(kHttpPort);
    http.setPath(QString::fromLatin1(kApiRootPath));
    return http;
}

QString UnipodMt11MediaClient::joinApiPath(const QUrl &base, const QString &endpoint)
{
    QString root = base.path();
    while (root.endsWith(QLatin1Char('/'))) {
        root.chop(1);
    }
    QString ep = endpoint;
    if (!ep.startsWith(QLatin1Char('/'))) {
        ep.prepend(QLatin1Char('/'));
    }
    return root + ep;
}

QUrl UnipodMt11MediaClient::buildMediaListUrl(const QUrl &base, int mediaType, const QString &path, int start, int count)
{
    QUrl url(base);
    url.setPath(joinApiPath(base, QStringLiteral("/api/v1/getmedialist")));
    // Multi-arg QString::arg so path characters like '%' are not re-substituted.
    // Keep empty path as "path=" (QUrlQuery would drop the '=').
    url.setQuery(QStringLiteral("media_type=%1&path=%2&start=%3&count=%4")
                     .arg(QString::number(mediaType), path, QString::number(start), QString::number(count)));
    return url;
}

QUrl UnipodMt11MediaClient::buildMediaCountUrl(const QUrl &base, int mediaType, const QString &path)
{
    QUrl url(base);
    url.setPath(joinApiPath(base, QStringLiteral("/api/v1/getmediacount")));
    url.setQuery(QStringLiteral("media_type=%1&path=%2").arg(QString::number(mediaType), path));
    return url;
}

QUrl UnipodMt11MediaClient::buildDirectoriesUrl(const QUrl &base, int mediaType)
{
    QUrl url(base);
    url.setPath(joinApiPath(base, QStringLiteral("/api/v1/getdirectories")));
    url.setQuery(QStringLiteral("media_type=%1").arg(mediaType));
    return url;
}

bool UnipodMt11MediaClient::parseMediaListResponse(const QByteArray &json, QString *errorOut,
                                                   QList<QPair<QString, QString>> *listOut)
{
    if (errorOut) {
        errorOut->clear();
    }
    if (listOut) {
        listOut->clear();
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Invalid JSON");
        }
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("success")).toBool(true) == false
        || root.value(QStringLiteral("code")).toInt(200) != 200) {
        if (errorOut) {
            *errorOut = root.value(QStringLiteral("message")).toString(QStringLiteral("Request failed"));
        }
        return false;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QJsonArray list = data.value(QStringLiteral("list")).toArray();
    if (listOut) {
        for (const QJsonValue &value : list) {
            const QJsonObject item = value.toObject();
            const QString name = item.value(QStringLiteral("name")).toString();
            const QString url = item.value(QStringLiteral("url")).toString();
            if (!name.isEmpty() && !url.isEmpty()) {
                listOut->append({name, url});
            }
        }
    }
    return true;
}

bool UnipodMt11MediaClient::parseDirectoriesResponse(const QByteArray &json, QString *errorOut, QStringList *pathsOut)
{
    if (errorOut) {
        errorOut->clear();
    }
    if (pathsOut) {
        pathsOut->clear();
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Invalid JSON");
        }
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("success")).toBool(true) == false
        || root.value(QStringLiteral("code")).toInt(200) != 200) {
        if (errorOut) {
            *errorOut = root.value(QStringLiteral("message")).toString(QStringLiteral("Request failed"));
        }
        return false;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QJsonArray directories = data.value(QStringLiteral("directories")).toArray();
    if (pathsOut) {
        for (const QJsonValue &value : directories) {
            const QJsonObject item = value.toObject();
            QString path = item.value(QStringLiteral("path")).toString();
            if (path.isEmpty()) {
                path = item.value(QStringLiteral("name")).toString();
            }
            if (!path.isEmpty()) {
                pathsOut->append(path);
            }
        }
        // Date folders like 2026-08-17 sort lexicographically; newest last → reverse for UI.
        std::reverse(pathsOut->begin(), pathsOut->end());
    }
    return true;
}

QString UnipodMt11MediaClient::joinSavePath(const QString &directory, const QString &fileName)
{
    if (directory.isEmpty() || fileName.isEmpty()) {
        return {};
    }
    return QDir(directory).filePath(QFileInfo(fileName).fileName());
}

void UnipodMt11MediaClient::refresh(int mediaType)
{
    if (!_ready) {
        return;
    }
    _abortListRequest();
    cancelDownload();
    _clearFiles();
    _directoryPaths.clear();
    _directoryIndex = 0;
    if (_mediaType != mediaType) {
        _mediaType = mediaType;
        emit mediaTypeChanged();
    }
    _nextStart = 0;
    _setHasMore(false);
    _setErrorString(QString());
    _fetchDirectories();
}

void UnipodMt11MediaClient::loadMore()
{
    if (!_ready || _loading || !_hasMore) {
        return;
    }
    _fetchPage(_nextStart);
}

void UnipodMt11MediaClient::download(UnipodMt11MediaFile *file)
{
    if (!file || file->url().isEmpty()) {
        emit downloadFinished(false, QString(), tr("Invalid file"));
        return;
    }
    if (_downloading) {
        emit downloadFinished(false, QString(), tr("Download already in progress"));
        return;
    }

    AppSettings *appSettings = SettingsManager::instance() ? SettingsManager::instance()->appSettings() : nullptr;
    if (!appSettings) {
        emit downloadFinished(false, QString(), tr("Settings unavailable"));
        return;
    }

    const QString directory = file->isVideo() ? appSettings->videoSavePath() : appSettings->photoSavePath();
    const QString outputPath = joinSavePath(directory, file->name());
    if (outputPath.isEmpty()) {
        emit downloadFinished(false, QString(), tr("Invalid save path"));
        return;
    }

    QDir().mkpath(directory);

    if (!_downloader) {
        _downloader = new QGCFileDownload(this);
        (void) connect(_downloader, &QGCFileDownload::finished, this, &UnipodMt11MediaClient::_onDownloadFinished);
        (void) connect(_downloader, &QGCFileDownload::progressChanged, this, &UnipodMt11MediaClient::_onDownloadProgress);
    }

    _downloader->setAutoDecompress(false);
    _downloader->setOutputPath(outputPath);
    _setDownloadProgress(0.0);
    _setDownloading(true);

    if (!_downloader->start(file->url())) {
        _setDownloading(false);
        emit downloadFinished(false, QString(), tr("Failed to start download"));
    }
}

void UnipodMt11MediaClient::cancelDownload()
{
    if (_downloader && _downloader->isRunning()) {
        _downloader->cancel();
    }
    _setDownloading(false);
    _setDownloadProgress(0.0);
}

void UnipodMt11MediaClient::_fetchDirectories()
{
    const QUrl url = buildDirectoriesUrl(baseUrl(), _mediaType);
    qCInfo(UnipodMt11MediaClientLog) << "GET" << url;

    QNetworkRequest request(url);
    request.setTransferTimeout(8000);

    _listReply = _networkManager->get(request);
    if (!_listReply) {
        _setErrorString(tr("Network request failed"));
        return;
    }

    _setLoading(true);
    (void) connect(_listReply, &QNetworkReply::finished, this, &UnipodMt11MediaClient::_onDirectoriesFinished);
}

void UnipodMt11MediaClient::_fetchPage(int start)
{
    if (_directoryIndex < 0 || _directoryIndex >= _directoryPaths.size()) {
        _setHasMore(false);
        _setLoading(false);
        return;
    }

    const QString path = _directoryPaths.at(_directoryIndex);
    const QUrl url = buildMediaListUrl(baseUrl(), _mediaType, path, start, kPageSize);
    qCInfo(UnipodMt11MediaClientLog) << "GET" << url;

    QNetworkRequest request(url);
    request.setTransferTimeout(8000);

    _listReply = _networkManager->get(request);
    if (!_listReply) {
        _setErrorString(tr("Network request failed"));
        return;
    }

    _setLoading(true);
    (void) connect(_listReply, &QNetworkReply::finished, this, &UnipodMt11MediaClient::_onListFinished);
}

void UnipodMt11MediaClient::_onDirectoriesFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        _setLoading(false);
        return;
    }
    reply->deleteLater();
    if (_listReply == reply) {
        _listReply.clear();
    }

    if (reply->error() != QNetworkReply::NoError) {
        _setLoading(false);
        QString message = reply->errorString();
        if (reply->error() == QNetworkReply::ConnectionRefusedError) {
            message = tr("Camera media web server unreachable at %1. "
                         "RTSP may still work; confirm MT11 HTTP media service on port %2.")
                          .arg(baseUrl().toString(), QString::number(kHttpPort));
        } else if (reply->error() == QNetworkReply::HostNotFoundError
                   || reply->error() == QNetworkReply::TimeoutError
                   || reply->error() == QNetworkReply::OperationCanceledError) {
            message = tr("Cannot reach camera web server (%1): %2")
                          .arg(baseUrl().toString(), reply->errorString());
        }
        qCWarning(UnipodMt11MediaClientLog) << "Directories failed" << reply->url() << reply->error() << message;
        _setErrorString(message);
        return;
    }

    QString parseError;
    QStringList paths;
    if (!parseDirectoriesResponse(reply->readAll(), &parseError, &paths)) {
        _setLoading(false);
        _setErrorString(parseError.isEmpty() ? tr("Failed to parse directories") : parseError);
        return;
    }

    _directoryPaths = paths;
    _directoryIndex = 0;
    _nextStart = 0;
    _setErrorString(QString());

    if (_directoryPaths.isEmpty()) {
        _setLoading(false);
        _setHasMore(false);
        return;
    }

    _fetchPage(0);
}

void UnipodMt11MediaClient::_onListFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        _setLoading(false);
        return;
    }
    reply->deleteLater();
    if (_listReply == reply) {
        _listReply.clear();
    }

    _setLoading(false);

    if (reply->error() != QNetworkReply::NoError) {
        QString message = reply->errorString();
        if (reply->error() == QNetworkReply::ConnectionRefusedError) {
            message = tr("Camera media web server unreachable at %1. "
                         "RTSP may still work; confirm MT11 HTTP media service on port %2.")
                          .arg(baseUrl().toString(), QString::number(kHttpPort));
        } else if (reply->error() == QNetworkReply::HostNotFoundError
                   || reply->error() == QNetworkReply::TimeoutError
                   || reply->error() == QNetworkReply::OperationCanceledError) {
            message = tr("Cannot reach camera web server (%1): %2")
                          .arg(baseUrl().toString(), reply->errorString());
        }
        qCWarning(UnipodMt11MediaClientLog) << "List failed" << reply->url() << reply->error() << message;
        _setErrorString(message);
        return;
    }

    QString parseError;
    QList<QPair<QString, QString>> items;
    if (!parseMediaListResponse(reply->readAll(), &parseError, &items)) {
        _setErrorString(parseError.isEmpty() ? tr("Failed to parse media list") : parseError);
        return;
    }

    _setErrorString(QString());
    const bool isVideo = (_mediaType == Video);
    for (const auto &item : items) {
        auto *file = new UnipodMt11MediaFile(item.first, item.second, isVideo, this);
        _files.append(file);
    }

    _advanceAfterPage(items.size());

    // Skip empty date folders without waiting for manual Load more.
    if (items.isEmpty() && _hasMore) {
        _fetchPage(_nextStart);
    }
}

void UnipodMt11MediaClient::_advanceAfterPage(int itemCount)
{
    if (itemCount >= kPageSize) {
        _nextStart += itemCount;
        _setHasMore(true);
        return;
    }

    // Finished current directory — move to next folder if any.
    ++_directoryIndex;
    _nextStart = 0;
    _setHasMore(_directoryIndex < _directoryPaths.size());
}

void UnipodMt11MediaClient::_onDownloadFinished(bool success, const QString &localPath, const QString &error)
{
    _setDownloading(false);
    if (!success) {
        _setDownloadProgress(0.0);
    } else {
        _setDownloadProgress(1.0);
    }
    emit downloadFinished(success, localPath, error);
}

void UnipodMt11MediaClient::_onDownloadProgress(qreal progress)
{
    _setDownloadProgress(progress);
}

void UnipodMt11MediaClient::_setLoading(bool loading)
{
    if (_loading == loading) {
        return;
    }
    _loading = loading;
    emit loadingChanged();
}

void UnipodMt11MediaClient::_setHasMore(bool hasMore)
{
    if (_hasMore == hasMore) {
        return;
    }
    _hasMore = hasMore;
    emit hasMoreChanged();
}

void UnipodMt11MediaClient::_setErrorString(const QString &error)
{
    if (_errorString == error) {
        return;
    }
    _errorString = error;
    emit errorStringChanged();
}

void UnipodMt11MediaClient::_setDownloading(bool downloading)
{
    if (_downloading == downloading) {
        return;
    }
    _downloading = downloading;
    emit downloadingChanged();
}

void UnipodMt11MediaClient::_setDownloadProgress(qreal progress)
{
    if (qFuzzyCompare(_downloadProgress, progress)) {
        return;
    }
    _downloadProgress = progress;
    emit downloadProgressChanged();
}

void UnipodMt11MediaClient::_clearFiles()
{
    _files.clearAndDeleteContents();
}

void UnipodMt11MediaClient::_abortListRequest()
{
    if (_listReply) {
        _listReply->abort();
        _listReply->deleteLater();
        _listReply.clear();
    }
    _setLoading(false);
}
