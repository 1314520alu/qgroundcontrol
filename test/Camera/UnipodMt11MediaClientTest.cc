#include "UnipodMt11MediaClientTest.h"

#include "UnipodMt11MediaClient.h"

#include <QtCore/QUrl>

void UnipodMt11MediaClientTest::testDefaultBaseUrlFromRtsp()
{
    const QUrl base = UnipodMt11MediaClient::defaultBaseUrlFromRtsp(
        QStringLiteral("rtsp://192.168.144.25:8554/video1"));
    QCOMPARE(base.scheme(), QStringLiteral("http"));
    QCOMPARE(base.host(), QStringLiteral("192.168.144.25"));
    QCOMPARE(base.port(), UnipodMt11MediaClient::kHttpPort);
    QCOMPARE(base.path(), QString::fromLatin1(UnipodMt11MediaClient::kApiRootPath));

    const QUrl fallback = UnipodMt11MediaClient::defaultBaseUrlFromRtsp(QString());
    QCOMPARE(fallback.host(), QStringLiteral("192.168.144.25"));
    QCOMPARE(fallback.port(), UnipodMt11MediaClient::kHttpPort);
    QCOMPARE(fallback.path(), QString::fromLatin1(UnipodMt11MediaClient::kApiRootPath));
}

void UnipodMt11MediaClientTest::testBuildMediaListUrl()
{
    const QUrl base = UnipodMt11MediaClient::defaultBaseUrlFromRtsp(
        QStringLiteral("rtsp://192.168.144.25:8554/video1"));

    const QUrl url = UnipodMt11MediaClient::buildMediaListUrl(base, 0, QString(), 0, 24);
    QCOMPARE(url.port(), 82);
    QCOMPARE(url.path(), QStringLiteral("/cgi-bin/media.cgi/api/v1/getmedialist"));
    QCOMPARE(url.query(), QStringLiteral("media_type=0&path=&start=0&count=24"));

    const QUrl paged = UnipodMt11MediaClient::buildMediaListUrl(base, 1, QStringLiteral("2026-08-17"), 24, 24);
    QCOMPARE(paged.query(), QStringLiteral("media_type=1&path=2026-08-17&start=24&count=24"));
}

void UnipodMt11MediaClientTest::testBuildMediaCountAndDirectoriesUrls()
{
    const QUrl base = UnipodMt11MediaClient::defaultBaseUrlFromRtsp(
        QStringLiteral("rtsp://192.168.144.25:8554/video1"));

    const QUrl countUrl = UnipodMt11MediaClient::buildMediaCountUrl(base, 0, QStringLiteral("2026-08-17"));
    QCOMPARE(countUrl.path(), QStringLiteral("/cgi-bin/media.cgi/api/v1/getmediacount"));
    QCOMPARE(countUrl.query(), QStringLiteral("media_type=0&path=2026-08-17"));

    const QUrl dirsUrl = UnipodMt11MediaClient::buildDirectoriesUrl(base, 1);
    QCOMPARE(dirsUrl.path(), QStringLiteral("/cgi-bin/media.cgi/api/v1/getdirectories"));
    QCOMPARE(dirsUrl.query(), QStringLiteral("media_type=1"));
}

void UnipodMt11MediaClientTest::testParseMediaListSuccess()
{
    const QByteArray json = R"({
        "code": 200,
        "data": {
            "media_type": 0,
            "path": "2026-08-17",
            "list": [
                {"name": "aa.jpg", "url": "http://192.168.144.25:82/photo/aa.jpg"},
                {"name": "bb.jpg", "url": "http://192.168.144.25:82/photo/bb.jpg"}
            ]
        },
        "success": true
    })";

    QString error;
    QList<QPair<QString, QString>> list;
    QVERIFY(UnipodMt11MediaClient::parseMediaListResponse(json, &error, &list));
    QVERIFY(error.isEmpty());
    QCOMPARE(list.size(), 2);
    QCOMPARE(list[0].first, QStringLiteral("aa.jpg"));
    QCOMPARE(list[0].second, QStringLiteral("http://192.168.144.25:82/photo/aa.jpg"));
    QCOMPARE(list[1].first, QStringLiteral("bb.jpg"));
}

void UnipodMt11MediaClientTest::testParseMediaListError()
{
    const QByteArray json = R"({
        "code": 400,
        "message": "path not exist",
        "success": false
    })";

    QString error;
    QList<QPair<QString, QString>> list;
    QVERIFY(!UnipodMt11MediaClient::parseMediaListResponse(json, &error, &list));
    QCOMPARE(error, QStringLiteral("path not exist"));
    QVERIFY(list.isEmpty());
}

void UnipodMt11MediaClientTest::testParseMediaListInvalidJson()
{
    QString error;
    QList<QPair<QString, QString>> list;
    QVERIFY(!UnipodMt11MediaClient::parseMediaListResponse(QByteArrayLiteral("{not-json"), &error, &list));
    QCOMPARE(error, QStringLiteral("Invalid JSON"));
}

void UnipodMt11MediaClientTest::testParseDirectoriesResponse()
{
    const QByteArray json = R"({
        "code": 200,
        "data": {
            "media_type": 0,
            "directories": [
                {"name": "1970-01-01", "path": "1970-01-01"},
                {"name": "2026-08-17", "path": "2026-08-17"}
            ]
        },
        "success": true,
        "message": ""
    })";

    QString error;
    QStringList paths;
    QVERIFY(UnipodMt11MediaClient::parseDirectoriesResponse(json, &error, &paths));
    QVERIFY(error.isEmpty());
    QCOMPARE(paths.size(), 2);
    // Newest-first after reverse
    QCOMPARE(paths[0], QStringLiteral("2026-08-17"));
    QCOMPARE(paths[1], QStringLiteral("1970-01-01"));
}

void UnipodMt11MediaClientTest::testJoinSavePath()
{
    QCOMPARE(UnipodMt11MediaClient::joinSavePath(QStringLiteral("/tmp/photos"), QStringLiteral("shot.jpg")),
             QStringLiteral("/tmp/photos/shot.jpg"));
    QCOMPARE(UnipodMt11MediaClient::joinSavePath(QStringLiteral("/tmp/photos"), QStringLiteral("/remote/path/shot.jpg")),
             QStringLiteral("/tmp/photos/shot.jpg"));
}

UT_REGISTER_TEST(UnipodMt11MediaClientTest, TestLabel::Unit)
