#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QmlObjectListModel.h"

class QGCFileDownload;
class QNetworkAccessManager;
class QNetworkReply;

/// One media file entry from UniPod MT11 Web Server getmedialist.
class UnipodMt11MediaFile : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(bool isVideo READ isVideo CONSTANT)

public:
    explicit UnipodMt11MediaFile(const QString &name, const QString &url, bool isVideo, QObject *parent = nullptr);

    QString name() const { return _name; }
    QString url() const { return _url; }
    bool isVideo() const { return _isVideo; }

private:
    QString _name;
    QString _url;
    bool _isVideo = false;
};

/// HTTP client for UniPod MT11 Web Server media browse / download.
/// Real device: http://<host>:82/cgi-bin/media.cgi/api/v1/... (not port 80 root).
class UnipodMt11MediaClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")

    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(int mediaType READ mediaType NOTIFY mediaTypeChanged)
    Q_PROPERTY(QmlObjectListModel *files READ files CONSTANT)
    Q_PROPERTY(bool downloading READ isDownloading NOTIFY downloadingChanged)
    Q_PROPERTY(qreal downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)

public:
    enum MediaType {
        Photo = 0,
        Video = 1,
    };
    Q_ENUM(MediaType)

    static constexpr int kPageSize = 24;
    static constexpr int kHttpPort = 82;
    static constexpr const char *kApiRootPath = "/cgi-bin/media.cgi";

    explicit UnipodMt11MediaClient(QObject *parent = nullptr);
    ~UnipodMt11MediaClient() override;

    bool isReady() const { return _ready; }
    bool isLoading() const { return _loading; }
    bool hasMore() const { return _hasMore; }
    QString errorString() const { return _errorString; }
    int mediaType() const { return _mediaType; }
    QmlObjectListModel *files() { return &_files; }
    bool isDownloading() const { return _downloading; }
    qreal downloadProgress() const { return _downloadProgress; }

    /// Base URL e.g. http://192.168.144.25:82/cgi-bin/media.cgi — derived from RTSP host.
    QUrl baseUrl() const;

    // --- Pure helpers (unit-tested) ---
    static QUrl buildMediaListUrl(const QUrl &base, int mediaType, const QString &path, int start, int count);
    static QUrl buildMediaCountUrl(const QUrl &base, int mediaType, const QString &path);
    static QUrl buildDirectoriesUrl(const QUrl &base, int mediaType);
    static QString joinApiPath(const QUrl &base, const QString &endpoint);
    static bool parseMediaListResponse(const QByteArray &json, QString *errorOut,
                                       QList<QPair<QString, QString>> *listOut);
    static bool parseDirectoriesResponse(const QByteArray &json, QString *errorOut, QStringList *pathsOut);
    static QString joinSavePath(const QString &directory, const QString &fileName);
    static QUrl defaultBaseUrlFromRtsp(const QString &rtspUrl);

    Q_INVOKABLE void refresh(int mediaType);
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void download(UnipodMt11MediaFile *file);
    Q_INVOKABLE void cancelDownload();

public slots:
    void setReady(bool ready);

signals:
    void readyChanged();
    void loadingChanged();
    void hasMoreChanged();
    void errorStringChanged();
    void mediaTypeChanged();
    void downloadingChanged();
    void downloadProgressChanged();
    void downloadFinished(bool success, const QString &localPath, const QString &error);

private slots:
    void _onDirectoriesFinished();
    void _onListFinished();
    void _onDownloadFinished(bool success, const QString &localPath, const QString &error);
    void _onDownloadProgress(qreal progress);

private:
    void _setLoading(bool loading);
    void _setHasMore(bool hasMore);
    void _setErrorString(const QString &error);
    void _setDownloading(bool downloading);
    void _setDownloadProgress(qreal progress);
    void _clearFiles();
    void _fetchDirectories();
    void _fetchPage(int start);
    void _abortListRequest();
    void _advanceAfterPage(int itemCount);

    QNetworkAccessManager *_networkManager = nullptr;
    QPointer<QNetworkReply> _listReply;
    QPointer<QGCFileDownload> _downloader;
    QmlObjectListModel _files;
    QStringList _directoryPaths;
    int _directoryIndex = 0;
    int _mediaType = Photo;
    int _nextStart = 0;
    bool _ready = false;
    bool _loading = false;
    bool _hasMore = false;
    bool _downloading = false;
    qreal _downloadProgress = 0.0;
    QString _errorString;
};
