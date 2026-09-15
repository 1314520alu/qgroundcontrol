#include "VideoSettingsTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "Fact.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

UT_REGISTER_TEST(VideoSettingsTest, TestLabel::Unit)

void VideoSettingsTest::_effectiveRtpJitterLatencyMs_data()
{
    QTest::addColumn<int>("requestedMs");
    QTest::addColumn<bool>("lowLatency");
    QTest::addColumn<QString>("videoSource");
    QTest::addColumn<QString>("rtspUrl");
    QTest::addColumn<int>("expectedMs");

    const QString zr10 = QString::fromLatin1(VideoSettings::videoSourceSiyiZr10);
    const QString rtsp = QString::fromLatin1(VideoSettings::videoSourceRTSP);
    const QString udp = QString::fromLatin1(VideoSettings::videoSourceUDPH264);
    const QString radioUrl = QString::fromLatin1(VideoSettings::siyiZr10RtspUrl);
    const QString lanUrl = QStringLiteral("rtsp://192.168.1.10:554/stream");

    QTest::newRow("zr10-floors-80") << 80 << false << zr10 << radioUrl << 180;
    QTest::newRow("zr10-low-latency-keeps-80") << 80 << true << zr10 << radioUrl << 80;
    QTest::newRow("zr10-keeps-higher-request") << 250 << false << zr10 << radioUrl << 250;
    QTest::newRow("generic-radio-rtsp-floors") << 80 << false << rtsp << radioUrl << 180;
    QTest::newRow("generic-lan-rtsp-keeps-80") << 80 << false << rtsp << lanUrl << 80;
    QTest::newRow("udp-ignores-stale-radio-url") << 80 << false << udp << radioUrl << 80;
}

void VideoSettingsTest::_effectiveRtpJitterLatencyMs()
{
    QFETCH(int, requestedMs);
    QFETCH(bool, lowLatency);
    QFETCH(QString, videoSource);
    QFETCH(QString, rtspUrl);
    QFETCH(int, expectedMs);

    QCOMPARE(VideoSettings::effectiveRtpJitterLatencyMs(requestedMs, lowLatency, videoSource, rtspUrl), expectedMs);
}

void VideoSettingsTest::_rtpJitterLatencyMsBelowMinRejected()
{
    VideoSettings* const settings = SettingsManager::instance()->videoSettings();
    QVERIFY(settings);

    Fact* const source = settings->videoSource();
    Fact* const url = settings->rtspUrl();
    Fact* const lowLatency = settings->lowLatencyMode();
    Fact* const jitter = settings->rtpJitterLatencyMs();
    QVERIFY(source);
    QVERIFY(url);
    QVERIFY(lowLatency);
    QVERIFY(jitter);

    const QVariant savedSource = source->rawValue();
    const QVariant savedUrl = url->rawValue();
    const QVariant savedLowLatency = lowLatency->rawValue();
    const QVariant savedJitter = jitter->rawValue();
    const auto guard =
        qScopeGuard([source, url, lowLatency, jitter, savedSource, savedUrl, savedLowLatency, savedJitter] {
            source->setRawValue(savedSource);
            url->setRawValue(savedUrl);
            lowLatency->setRawValue(savedLowLatency);
            jitter->setRawValue(savedJitter);
        });

    source->setRawValue(QString::fromLatin1(VideoSettings::videoSourceSiyiZr10));
    url->setRawValue(QString::fromLatin1(VideoSettings::siyiZr10RtspUrl));
    lowLatency->setRawValue(false);

    expectAppMessage(QRegularExpression(QStringLiteral("Setting not applied.*at least 180 ms")));
    jitter->setRawValue(80);
    verifyExpectedLogMessage();

    QCOMPARE(jitter->rawValue().toUInt(), 180u);
}

void VideoSettingsTest::_rtpJitterLatencyMsAboveMinAccepted()
{
    VideoSettings* const settings = SettingsManager::instance()->videoSettings();
    QVERIFY(settings);

    Fact* const source = settings->videoSource();
    Fact* const url = settings->rtspUrl();
    Fact* const lowLatency = settings->lowLatencyMode();
    Fact* const jitter = settings->rtpJitterLatencyMs();
    QVERIFY(source);
    QVERIFY(url);
    QVERIFY(lowLatency);
    QVERIFY(jitter);

    const QVariant savedSource = source->rawValue();
    const QVariant savedUrl = url->rawValue();
    const QVariant savedLowLatency = lowLatency->rawValue();
    const QVariant savedJitter = jitter->rawValue();
    const auto guard =
        qScopeGuard([source, url, lowLatency, jitter, savedSource, savedUrl, savedLowLatency, savedJitter] {
            source->setRawValue(savedSource);
            url->setRawValue(savedUrl);
            lowLatency->setRawValue(savedLowLatency);
            jitter->setRawValue(savedJitter);
        });

    source->setRawValue(QString::fromLatin1(VideoSettings::videoSourceSiyiZr10));
    url->setRawValue(QString::fromLatin1(VideoSettings::siyiZr10RtspUrl));
    lowLatency->setRawValue(false);
    jitter->setRawValue(250);

    QCOMPARE(jitter->rawValue().toUInt(), 250u);
}
