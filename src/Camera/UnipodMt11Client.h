#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

class QUdpSocket;

/// UDP client for UniPod MT11 onboard photo/record control (192.168.144.25:37260).
class UnipodMt11Client : public QObject
{
    Q_OBJECT

public:
    explicit UnipodMt11Client(QObject *parent = nullptr);
    ~UnipodMt11Client() override;

    bool isReady() const { return _ready; }
    quint8 recordSta() const { return _recordSta; }

    Q_INVOKABLE void start();
    void stop();

    void takePhoto();
    void toggleRecording();

public slots:
    void setActive(bool active);

signals:
    void readyChanged();
    void recordStaChanged(quint8 recordSta);
    void funcFeedback(quint8 infoType);
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
    void _pollSystemInfo();

    QUdpSocket *_socket = nullptr;
    QTimer *_pollTimer = nullptr;
    quint16 _seq = 0;
    quint8 _recordSta = 0;
    bool _ready = false;
    bool _active = false;
    bool _running = false;
};
