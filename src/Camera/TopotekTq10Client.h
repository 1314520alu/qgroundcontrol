#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

class QUdpSocket;

/// UDP client for Topotek TQ10N onboard photo/record/PTZ/zoom (192.168.144.108:9003).
class TopotekTq10Client : public QObject
{
    Q_OBJECT

public:
    explicit TopotekTq10Client(QObject *parent = nullptr);
    ~TopotekTq10Client() override;

    bool isReady() const { return _ready; }
    quint8 recordSta() const { return _recordSta; }

    Q_INVOKABLE void start();
    void stop();

    void takePhoto();
    void toggleRecording();
    void startZoom(int direction);
    void stopZoom();
    void ptzStart(int direction);
    void ptzStop();
    void ptzHome();

public slots:
    void setActive(bool active);

signals:
    void readyChanged();
    void recordStaChanged(quint8 recordSta);
    void sendFailed(const QString &reason);

private slots:
    void _onPollTimeout();
    void _onReadyRead();

private:
    bool _canStart();
    bool _isEthernetReady() const;
    void _setReady(bool ready);
    void _setRecordSta(quint8 recordSta);
    bool _sendDatagram(const QByteArray &frame);
    void _pollRecordState();

    QUdpSocket *_socket = nullptr;
    QTimer *_pollTimer = nullptr;
    quint8 _recordSta = 0;
    bool _ready = false;
    bool _active = false;
    bool _running = false;
};
