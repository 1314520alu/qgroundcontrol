#include "TopotekTq10Client.h"

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "QGCLoggingCategory.h"
#include "QmlControls/ScreenToolsController.h"
#include "Settings/SettingsManager.h"
#include "Settings/VideoSettings.h"
#include "TopotekTq10Protocol.h"

QGC_LOGGING_CATEGORY(TopotekTq10ClientLog, "Camera.TopotekTq10Client")

TopotekTq10Client::TopotekTq10Client(QObject *parent)
    : QObject(parent)
{
    _pollTimer = new QTimer(this);
    _pollTimer->setInterval(1000);
    (void) connect(_pollTimer, &QTimer::timeout, this, &TopotekTq10Client::_onPollTimeout);
}

TopotekTq10Client::~TopotekTq10Client()
{
    stop();
}

void TopotekTq10Client::setActive(bool active)
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

void TopotekTq10Client::start()
{
    if (_running) {
        return;
    }

    if (!_canStart()) {
        qCDebug(TopotekTq10ClientLog) << "Start deferred: video source or ethernet not ready";
        _setReady(false);
        return;
    }

    _socket = new QUdpSocket(this);
    if (!_socket->bind(QHostAddress::AnyIPv4, TopotekTq10Protocol::kLocalPort)) {
        qCWarning(TopotekTq10ClientLog) << "Failed to bind UDP socket on port"
                                          << TopotekTq10Protocol::kLocalPort << ":" << _socket->errorString();
        _socket->deleteLater();
        _socket = nullptr;
        _setReady(false);
        return;
    }

    (void) connect(_socket, &QUdpSocket::readyRead, this, &TopotekTq10Client::_onReadyRead);

    _running = true;
    _setReady(true);
    _pollTimer->start();
    _pollRecordState();

    qCDebug(TopotekTq10ClientLog) << "Started on local port" << _socket->localPort()
                                    << "ethernet:" << ScreenToolsController::siyiRadioEthernetAddress();
}

void TopotekTq10Client::stop()
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

    qCDebug(TopotekTq10ClientLog) << "Stopped";
}

void TopotekTq10Client::takePhoto()
{
    if (!isReady()) {
        emit sendFailed(tr("Topotek TQ10N not ready"));
        return;
    }

    const QByteArray frame = TopotekTq10Protocol::buildCap();
    if (!_sendDatagram(frame)) {
        emit sendFailed(tr("Failed to send photo command"));
    }
}

void TopotekTq10Client::toggleRecording()
{
    if (!isReady()) {
        emit sendFailed(tr("Topotek TQ10N not ready"));
        return;
    }

    const QByteArray frame = TopotekTq10Protocol::buildRecToggle();
    if (!_sendDatagram(frame)) {
        emit sendFailed(tr("Failed to send record toggle command"));
    }
}

void TopotekTq10Client::startZoom(int direction)
{
    if (!isReady()) {
        return;
    }

    const char *data = (direction > 0) ? "02" : "01";
    (void) _sendDatagram(TopotekTq10Protocol::buildZoom(data));
}

void TopotekTq10Client::stopZoom()
{
    if (!isReady()) {
        return;
    }

    (void) _sendDatagram(TopotekTq10Protocol::buildZoom("00"));
}

void TopotekTq10Client::ptzStart(int direction)
{
    if (!isReady()) {
        return;
    }

    const char *data = "00";
    switch (direction) {
    case 1: data = "01"; break;
    case 2: data = "02"; break;
    case 3: data = "03"; break;
    case 4: data = "04"; break;
    default: return;
    }

    (void) _sendDatagram(TopotekTq10Protocol::buildPtz(data));
}

void TopotekTq10Client::ptzStop()
{
    if (!isReady()) {
        return;
    }

    (void) _sendDatagram(TopotekTq10Protocol::buildPtz("00"));
}

void TopotekTq10Client::ptzHome()
{
    if (!isReady()) {
        emit sendFailed(tr("Topotek TQ10N not ready"));
        return;
    }

    if (!_sendDatagram(TopotekTq10Protocol::buildPtz("05"))) {
        emit sendFailed(tr("Failed to send gimbal home command"));
    }
}

bool TopotekTq10Client::_canStart()
{
    VideoSettings *videoSettings = SettingsManager::instance()->videoSettings();
    if (!videoSettings) {
        return false;
    }

    const QString source = videoSettings->videoSource()->rawValue().toString();
    if (source != VideoSettings::videoSourceTopotekTq10N) {
        return false;
    }

    return _isEthernetReady();
}

bool TopotekTq10Client::_isEthernetReady() const
{
    if (ScreenToolsController::isSiyiRadioEthernetReady()) {
        return true;
    }

    const QString address = ScreenToolsController::siyiRadioEthernetAddress();
    return !address.isEmpty() && address.startsWith(QStringLiteral("192.168.144."));
}

void TopotekTq10Client::_setReady(bool ready)
{
    if (_ready == ready) {
        return;
    }

    _ready = ready;
    emit readyChanged();
}

void TopotekTq10Client::_setRecordSta(quint8 recordSta)
{
    if (_recordSta == recordSta) {
        return;
    }

    _recordSta = recordSta;
    emit recordStaChanged(_recordSta);
}

bool TopotekTq10Client::_sendDatagram(const QByteArray &frame)
{
    if (!_socket) {
        qCWarning(TopotekTq10ClientLog) << "Send failed: socket not open";
        return false;
    }

    qCDebug(TopotekTq10ClientLog) << "Send" << frame;

    const qint64 bytes = _socket->writeDatagram(
        frame,
        QHostAddress(TopotekTq10Protocol::kDefaultHost),
        TopotekTq10Protocol::kDefaultPort);
    if (bytes != frame.size()) {
        qCWarning(TopotekTq10ClientLog) << "Send failed:" << _socket->errorString();
        return false;
    }

    return true;
}

void TopotekTq10Client::_pollRecordState()
{
    if (!_running || !_socket) {
        return;
    }

    (void) _sendDatagram(TopotekTq10Protocol::buildRecQuery());
}

void TopotekTq10Client::_onPollTimeout()
{
    if (!_isEthernetReady()) {
        qCWarning(TopotekTq10ClientLog) << "Ethernet lost; stopping client";
        stop();
        return;
    }

    _pollRecordState();
}

void TopotekTq10Client::_onReadyRead()
{
    if (!_socket) {
        return;
    }

    while (_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(_socket->pendingDatagramSize()));
        (void) _socket->readDatagram(datagram.data(), datagram.size());

        qCDebug(TopotekTq10ClientLog) << "Recv" << datagram;

        quint8 recordSta = 0;
        if (TopotekTq10Protocol::parseRecordState(datagram, &recordSta)) {
            _setRecordSta(recordSta);
        }
    }
}
