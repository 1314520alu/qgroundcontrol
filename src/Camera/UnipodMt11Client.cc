#include "UnipodMt11Client.h"

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "QGCLoggingCategory.h"
#include "QmlControls/ScreenToolsController.h"
#include "Settings/SettingsManager.h"
#include "Settings/VideoSettings.h"
#include "UnipodMt11Protocol.h"

QGC_LOGGING_CATEGORY(UnipodMt11ClientLog, "Camera.UnipodMt11Client")

UnipodMt11Client::UnipodMt11Client(QObject *parent)
    : QObject(parent)
{
    _pollTimer = new QTimer(this);
    _pollTimer->setInterval(1000);
    (void) connect(_pollTimer, &QTimer::timeout, this, &UnipodMt11Client::_onPollTimeout);
}

UnipodMt11Client::~UnipodMt11Client()
{
    stop();
}

void UnipodMt11Client::setActive(bool active)
{
    if (_active == active) {
        return;
    }

    _active = active;
    if (_active) {
        start();
    } else {
        stop();
    }
}

void UnipodMt11Client::start()
{
    if (_running) {
        return;
    }

    if (!_canStart()) {
        qCDebug(UnipodMt11ClientLog) << "Start deferred: video source or ethernet not ready";
        _setReady(false);
        return;
    }

    _socket = new QUdpSocket(this);
    if (!_socket->bind(QHostAddress::AnyIPv4, 0)) {
        qCWarning(UnipodMt11ClientLog) << "Failed to bind UDP socket:" << _socket->errorString();
        _socket->deleteLater();
        _socket = nullptr;
        _setReady(false);
        return;
    }

    (void) connect(_socket, &QUdpSocket::readyRead, this, &UnipodMt11Client::_onReadyRead);

    _running = true;
    _setReady(true);
    _pollTimer->start();
    _pollSystemInfo();

    qCDebug(UnipodMt11ClientLog) << "Started on local port" << _socket->localPort()
                                 << "ethernet:" << ScreenToolsController::siyiRadioEthernetAddress();
}

void UnipodMt11Client::stop()
{
    if (_pollTimer) {
        _pollTimer->stop();
    }

    if (_socket) {
        _socket->close();
        _socket->deleteLater();
        _socket = nullptr;
    }

    _running = false;
    _setReady(false);
    _setRecordSta(0);

    qCDebug(UnipodMt11ClientLog) << "Stopped";
}

void UnipodMt11Client::takePhoto()
{
    if (!isReady()) {
        emit sendFailed(tr("UniPod MT11 not ready"));
        return;
    }

    const QByteArray frame = UnipodMt11Protocol::buildPhotoCommand(++_seq);
    if (!_sendDatagram(frame)) {
        emit sendFailed(tr("Failed to send photo command"));
    }
}

void UnipodMt11Client::toggleRecording()
{
    if (!isReady()) {
        emit sendFailed(tr("UniPod MT11 not ready"));
        return;
    }

    const QByteArray frame = UnipodMt11Protocol::buildRecordToggleCommand(++_seq);
    if (!_sendDatagram(frame)) {
        emit sendFailed(tr("Failed to send record toggle command"));
    }
}

bool UnipodMt11Client::_canStart()
{
    VideoSettings *videoSettings = SettingsManager::instance()->videoSettings();
    if (!videoSettings) {
        return false;
    }

    const QString source = videoSettings->videoSource()->rawValue().toString();
    if (source != VideoSettings::videoSourceUnipodMT11) {
        return false;
    }

    return _isEthernetReady();
}

bool UnipodMt11Client::_isEthernetReady() const
{
    if (ScreenToolsController::isSiyiRadioEthernetReady()) {
        return true;
    }

    const QString address = ScreenToolsController::siyiRadioEthernetAddress();
    return !address.isEmpty() && address.startsWith(QStringLiteral("192.168.144."));
}

void UnipodMt11Client::_setReady(bool ready)
{
    if (_ready == ready) {
        return;
    }

    _ready = ready;
    emit readyChanged();
}

void UnipodMt11Client::_setRecordSta(quint8 recordSta)
{
    if (_recordSta == recordSta) {
        return;
    }

    _recordSta = recordSta;
    emit recordStaChanged(_recordSta);
}

bool UnipodMt11Client::_sendDatagram(const QByteArray &frame)
{
    if (!_socket) {
        qCWarning(UnipodMt11ClientLog) << "Send failed: socket not open";
        return false;
    }

    qCDebug(UnipodMt11ClientLog) << "Send" << frame.toHex(' ');

    const qint64 bytes = _socket->writeDatagram(
        frame,
        QHostAddress(UnipodMt11Protocol::kDefaultHost),
        UnipodMt11Protocol::kDefaultPort);
    if (bytes != frame.size()) {
        qCWarning(UnipodMt11ClientLog) << "Send failed:" << _socket->errorString();
        return false;
    }

    return true;
}

void UnipodMt11Client::_pollSystemInfo()
{
    if (!_running || !_socket) {
        return;
    }

    const QByteArray frame = UnipodMt11Protocol::buildSystemInfoRequest(++_seq);
    (void) _sendDatagram(frame);
}

void UnipodMt11Client::_onPollTimeout()
{
    if (!_isEthernetReady()) {
        qCWarning(UnipodMt11ClientLog) << "Ethernet lost; stopping client";
        stop();
        return;
    }

    _pollSystemInfo();
}

void UnipodMt11Client::_onReadyRead()
{
    if (!_socket) {
        return;
    }

    while (_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(_socket->pendingDatagramSize()));
        (void) _socket->readDatagram(datagram.data(), datagram.size());

        quint8 ctrl = 0;
        quint16 seq = 0;
        quint8 cmd = 0;
        QByteArray payload;
        if (!UnipodMt11Protocol::parseFrame(datagram, &ctrl, &seq, &cmd, &payload)) {
            qCDebug(UnipodMt11ClientLog) << "Dropped invalid frame:" << datagram.toHex(' ');
            continue;
        }

        qCDebug(UnipodMt11ClientLog) << "Recv cmd" << Qt::hex << cmd << "seq" << seq
                                     << "payload" << payload.toHex(' ') << Qt::dec;

        if (cmd == 0x0A) {
            UnipodMt11Protocol::SystemInfoAck ack;
            if (UnipodMt11Protocol::parseSystemInfoAck(payload, &ack)) {
                _setRecordSta(ack.recordSta);
            }
        } else if (cmd == 0x0B) {
            quint8 infoType = 0;
            if (UnipodMt11Protocol::parseFuncFeedback(payload, &infoType)) {
                using UnipodMt11Protocol::FuncFeedback;
                if (infoType == static_cast<quint8>(FuncFeedback::RecordStart)) {
                    _setRecordSta(1);
                } else if (infoType == static_cast<quint8>(FuncFeedback::RecordEnd)) {
                    _setRecordSta(0);
                }
                emit funcFeedback(infoType);
            }
        }
    }
}
