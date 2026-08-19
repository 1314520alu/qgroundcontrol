#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class VideoSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    VideoSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(videoSource)
    DEFINE_SETTINGFACT(udpUrl)
    DEFINE_SETTINGFACT(tcpUrl)
    DEFINE_SETTINGFACT(rtspUrl)
    DEFINE_SETTINGFACT(aspectRatio)
    DEFINE_SETTINGFACT(videoFit)
    DEFINE_SETTINGFACT(gridLines)
    DEFINE_SETTINGFACT(showRecControl)
    DEFINE_SETTINGFACT(recordingFormat)
    DEFINE_SETTINGFACT(maxVideoSize)
    DEFINE_SETTINGFACT(enableStorageLimit)
    DEFINE_SETTINGFACT(rtspTimeout)
    DEFINE_SETTINGFACT(streamEnabled)
    DEFINE_SETTINGFACT(disableWhenDisarmed)
    DEFINE_SETTINGFACT(lowLatencyMode)
    DEFINE_SETTINGFACT(rtpJitterLatencyMs)
    DEFINE_SETTINGFACT(rtspAutoReconnect)
    DEFINE_SETTINGFACT(forceVideoDecoder)
    DEFINE_SETTINGFACT(forceCpuVideoPath)
    DEFINE_SETTINGFACT(videoConversionElement)
    DEFINE_SETTINGFACT(disablePixelAspectRatio)

    Q_PROPERTY(bool     streamConfigured        READ streamConfigured       NOTIFY streamConfiguredChanged)
    Q_PROPERTY(QString  rtspVideoSource         READ rtspVideoSource        CONSTANT)
    Q_PROPERTY(QString  udp264VideoSource       READ udp264VideoSource      CONSTANT)
    Q_PROPERTY(QString  udp265VideoSource       READ udp265VideoSource      CONSTANT)
    Q_PROPERTY(QString  tcpVideoSource          READ tcpVideoSource         CONSTANT)
    Q_PROPERTY(QString  mpegtsVideoSource       READ mpegtsVideoSource      CONSTANT)
    Q_PROPERTY(QString  disabledVideoSource     READ disabledVideoSource    CONSTANT)
    Q_PROPERTY(QString  unipodMT11VideoSource   READ unipodMT11VideoSource  CONSTANT)
    Q_PROPERTY(QString  siyiR1MVideoSource      READ siyiR1MVideoSource     CONSTANT)
    Q_PROPERTY(QString  siyiA8MiniVideoSource   READ siyiA8MiniVideoSource  CONSTANT)
    Q_PROPERTY(QString  topotekTq10NVideoSource  READ topotekTq10NVideoSource CONSTANT)

    bool     streamConfigured       ();
    QString  rtspVideoSource        () { return videoSourceRTSP; }
    QString  udp264VideoSource      () { return videoSourceUDPH264; }
    QString  udp265VideoSource      () { return videoSourceUDPH265; }
    QString  tcpVideoSource         () { return videoSourceTCP; }
    QString  mpegtsVideoSource      () { return videoSourceMPEGTS; }
    QString  disabledVideoSource    () { return videoDisabled; }
    QString  unipodMT11VideoSource  () { return videoSourceUnipodMT11; }
    QString  siyiR1MVideoSource     () { return videoSourceSiyiR1M; }
    QString  siyiA8MiniVideoSource  () { return videoSourceSiyiA8Mini; }
    QString  topotekTq10NVideoSource () { return videoSourceTopotekTq10N; }

    /// Remove hardware forced-decoder options absent from the running GStreamer registry, and
    /// reset the active choice to Default if it was pruned. Call after the video backend has
    /// initialized (the registry is empty until then).
    void pruneUnavailableDecoders();

    static constexpr const char* videoSourceNoVideo           = QT_TRANSLATE_NOOP("VideoSettings", "No Video Available");
    static constexpr const char* videoDisabled                = QT_TRANSLATE_NOOP("VideoSettings", "Video Stream Disabled");
    static constexpr const char* videoSourceRTSP              = QT_TRANSLATE_NOOP("VideoSettings", "RTSP Video Stream");
    static constexpr const char* videoSourceUDPH264           = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.264 Video Stream");
    static constexpr const char* videoSourceUDPH265           = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.265 Video Stream");
    static constexpr const char* videoSourceTCP               = QT_TRANSLATE_NOOP("VideoSettings", "TCP-MPEG2 Video Stream");
    static constexpr const char* videoSourceMPEGTS            = QT_TRANSLATE_NOOP("VideoSettings", "MPEG-TS Video Stream");
    static constexpr const char* videoSource3DRSolo           = QT_TRANSLATE_NOOP("VideoSettings", "3DR Solo (requires restart)");
    static constexpr const char* videoSourceParrotDiscovery   = QT_TRANSLATE_NOOP("VideoSettings", "Parrot Discovery");
    static constexpr const char* videoSourceYuneecMantisG     = QT_TRANSLATE_NOOP("VideoSettings", "Yuneec Mantis G");
    static constexpr const char* videoSourceHerelinkAirUnit   = QT_TRANSLATE_NOOP("VideoSettings", "Herelink AirUnit");
    static constexpr const char* videoSourceHerelinkHotspot   = QT_TRANSLATE_NOOP("VideoSettings", "Herelink Hotspot");
    static constexpr const char* videoSourceUnipodMT11        = QT_TRANSLATE_NOOP("VideoSettings", "UniPod MT11");
    static constexpr const char* videoSourceSiyiR1M           = QT_TRANSLATE_NOOP("VideoSettings", "SIYI R1M");
    static constexpr const char* videoSourceSiyiA8Mini        = QT_TRANSLATE_NOOP("VideoSettings", "SIYI A8 Mini");
    static constexpr const char* videoSourceTopotekTq10N      = QT_TRANSLATE_NOOP("VideoSettings", "Topotek TQ10N");

    /// Default primary RTSP URL from UniPod MT11 manual (192.168.144.25 / video1).
    static constexpr const char* unipodMT11RtspUrl = "rtsp://192.168.144.25:8554/video1";
    /// Default RTSP URL from SIYI R1 / R1M recording camera manual.
    static constexpr const char* siyiR1MRtspUrl = "rtsp://192.168.144.25:8554/main.264";
    /// Pre-ZT30 gimbal RTSP (A8 mini / ZR10 / …) — same path as R1M per A8 mini User Manual §4.6.
    static constexpr const char* siyiA8MiniRtspUrl = "rtsp://192.168.144.25:8554/main.264";
    /// Default RTSP URL from Topotek QGC/VLC guide (TQ10N on 192.168.144.108).
    static constexpr const char* topotekTq10NRtspUrl = "rtsp://192.168.144.108:554/stream=0";

    /// True for preset sources that live on the SIYI radio ethernet (192.168.144.x).
    static bool usesSiyiRadioEthernet(const QString &source);

signals:
    void streamConfiguredChanged    (bool configured);

private slots:
    void _configChanged             (QVariant value);

private:
    void _setDefaults               ();
    void _setForceVideoDecodeList();

private:
    bool _noVideo = false;

};
