#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QtNumeric>

class QUdpSocket;

/// UDP client for UniPod MT11 onboard photo/record control (192.168.144.25:37260).
class UnipodMt11Client : public QObject
{
    Q_OBJECT

public:
    explicit UnipodMt11Client(QObject* parent = nullptr);
    ~UnipodMt11Client() override;

    bool isReady() const { return _ready; }

    quint8 recordSta() const { return _recordSta; }

    Q_INVOKABLE void start();
    void stop();

    void takePhoto();
    void toggleRecording();
    void startZoom(int direction);
    void stopZoom();
    void startFocus(int direction);
    void stopFocus();
    /// direction: 1=up, 2=down, 3=left, 4=right (matches Topotek pad / overlay).
    void ptzStart(int direction);
    void ptzStop();
    void ptzHome();
    /// main/secondary stream layout (CMD 0x11). Common MT11 presets: zoom+IR, IR+zoom, PIP+IR.
    void setVideoLayout(quint8 mainMode, quint8 secondaryMode);
    void setLaserEnabled(bool enabled);
    void requestLaserDistance();
    void setAiRecognitionEnabled(bool enabled);

    bool laserEnabled() const { return _laserEnabled; }

    bool aiRecognitionEnabled() const { return _aiRecognitionEnabled; }

    /// Last laser range in meters; NaN if unknown.
    double laserDistanceMeters() const { return _laserDistanceMeters; }

public slots:
    void setActive(bool active);

signals:
    void readyChanged();
    void recordStaChanged(quint8 recordSta);
    void funcFeedback(quint8 infoType);
    void sendFailed(const QString& reason);
    void laserEnabledChanged();
    void aiRecognitionEnabledChanged();
    void laserDistanceChanged();

private slots:
    void _onPollTimeout();
    void _onReadyRead();

private:
    bool _canStart();
    bool _isEthernetReady() const;
    void _setReady(bool ready);
    void _setRecordSta(quint8 recordSta);
    bool _sendDatagram(const QByteArray& frame);
    void _pollSystemInfo();

    QUdpSocket* _socket = nullptr;
    QTimer* _pollTimer = nullptr;
    quint16 _seq = 0;
    quint8 _recordSta = 0;
    bool _ready = false;
    bool _active = false;
    bool _running = false;
    bool _laserEnabled = false;
    bool _aiRecognitionEnabled = false;
    double _laserDistanceMeters = qQNaN();
};
