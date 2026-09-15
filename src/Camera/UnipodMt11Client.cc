#include "UnipodMt11Client.h"

#include <QtCore/QtMath>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "QGCLoggingCategory.h"
#include "QmlControls/ScreenToolsController.h"
#include "Settings/SettingsManager.h"
#include "Settings/VideoSettings.h"
#include "UnipodMt11Protocol.h"

QGC_LOGGING_CATEGORY(UnipodMt11ClientLog, "Camera.UnipodMt11Client")

UnipodMt11Client::UnipodMt11Client(QObject* parent) : QObject(parent)
{
    _pollTimer = new QTimer(this);
    _pollTimer->setInterval(1000);
    (void) connect(_pollTimer, &QTimer::timeout, this, &UnipodMt11Client::_onPollTimeout);

    _zoomHoldTimer = new QTimer(this);
    _zoomHoldTimer->setInterval(UnipodMt11Protocol::kCurrentZoomPollIntervalMs);
    (void) connect(_zoomHoldTimer, &QTimer::timeout, this, &UnipodMt11Client::_onZoomHoldTimeout);
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
    _zoomHoldTimer->start();
    _pollSystemInfo();
    requestCurrentZoom();

    qCDebug(UnipodMt11ClientLog) << "Started on local port" << _socket->localPort()
                                 << "ethernet:" << ScreenToolsController::siyiRadioEthernetAddress();
}

void UnipodMt11Client::stop()
{
    if (_pollTimer) {
        _pollTimer->stop();
    }
    if (_zoomHoldTimer) {
        _zoomHoldTimer->stop();
    }
    _stopZoomHold();

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
        emit sendFailed(tr("SIYI UDP camera not ready"));
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
        emit sendFailed(tr("SIYI UDP camera not ready"));
        return;
    }

    const QByteArray frame = UnipodMt11Protocol::buildRecordToggleCommand(++_seq);
    if (!_sendDatagram(frame)) {
        emit sendFailed(tr("Failed to send record toggle command"));
    }
}

void UnipodMt11Client::startZoom(int direction)
{
    if (!isReady()) {
        return;
    }

    const int holdDirection = (direction > 0) ? 1 : ((direction < 0) ? -1 : 0);
    if (holdDirection == 0) {
        stopZoom();
        return;
    }
    if (_zoomHoldDirection == holdDirection) {
        return;
    }

    _zoomHoldDirection = holdDirection;
    _zoomHoldMotorStopped = false;
    const qint8 zoom = static_cast<qint8>(holdDirection);
    (void) _sendDatagram(UnipodMt11Protocol::buildZoomCommand(++_seq, zoom));
    requestCurrentZoom();
    if (_zoomHoldTimer) {
        _zoomHoldTimer->setInterval(UnipodMt11Protocol::kCurrentZoomHoldPollIntervalMs);
        if (!_zoomHoldTimer->isActive()) {
            _zoomHoldTimer->start();
        }
    }
}

void UnipodMt11Client::stopZoom()
{
    if (!isReady()) {
        _stopZoomHold();
        return;
    }

    _stopZoomHold();
    (void) _sendDatagram(UnipodMt11Protocol::buildZoomCommand(++_seq, 0));
    requestCurrentZoom();
}

void UnipodMt11Client::requestCurrentZoom()
{
    if (!isReady()) {
        return;
    }

    (void) _sendDatagram(UnipodMt11Protocol::buildCurrentZoomRequest(++_seq));
}

void UnipodMt11Client::startFocus(int direction)
{
    if (!isReady()) {
        return;
    }

    const qint8 focus = (direction > 0) ? 1 : ((direction < 0) ? -1 : 0);
    (void) _sendDatagram(UnipodMt11Protocol::buildFocusCommand(++_seq, focus));
}

void UnipodMt11Client::stopFocus()
{
    if (!isReady()) {
        return;
    }

    (void) _sendDatagram(UnipodMt11Protocol::buildFocusCommand(++_seq, 0));
}

void UnipodMt11Client::ptzStart(int direction)
{
    if (!isReady()) {
        return;
    }

    static constexpr qint8 kSpeed = 20;
    qint8 yaw = 0;
    qint8 pitch = 0;
    switch (direction) {
        case 1:  // up
            pitch = kSpeed;
            break;
        case 2:  // down
            pitch = -kSpeed;
            break;
        case 3:  // left
            yaw = -kSpeed;
            break;
        case 4:  // right
            yaw = kSpeed;
            break;
        default:
            return;
    }

    (void) _sendDatagram(UnipodMt11Protocol::buildGimbalSpeedCommand(++_seq, yaw, pitch));
}

void UnipodMt11Client::ptzStop()
{
    if (!isReady()) {
        return;
    }

    (void) _sendDatagram(UnipodMt11Protocol::buildGimbalSpeedCommand(++_seq, 0, 0));
}

void UnipodMt11Client::ptzCenter(quint8 mode)
{
    if (!isReady()) {
        return;
    }
    if (mode < 1 || mode > 4) {
        return;
    }
    (void) _sendDatagram(UnipodMt11Protocol::buildCenterCommand(++_seq, mode));
}

void UnipodMt11Client::ptzHome()
{
    ptzCenter(1);
}

void UnipodMt11Client::setVideoLayout(quint8 mainMode, quint8 secondaryMode)
{
    if (!isReady()) {
        emit sendFailed(tr("SIYI UDP camera not ready"));
        return;
    }

    if (!_sendDatagram(UnipodMt11Protocol::buildSetVideoLayoutCommand(++_seq, mainMode, secondaryMode))) {
        emit sendFailed(tr("Failed to send video layout command"));
    }
}

void UnipodMt11Client::setLaserEnabled(bool enabled)
{
    if (!isReady()) {
        emit sendFailed(tr("SIYI UDP camera not ready"));
        return;
    }

    if (!_sendDatagram(UnipodMt11Protocol::buildSetLaserStateCommand(++_seq, enabled))) {
        emit sendFailed(tr("Failed to send laser command"));
        return;
    }

    if (_laserEnabled != enabled) {
        _laserEnabled = enabled;
        emit laserEnabledChanged();
    }
    if (!enabled) {
        _laserDistanceMeters = qQNaN();
        emit laserDistanceChanged();
    }
}

void UnipodMt11Client::requestLaserDistance()
{
    if (!isReady()) {
        return;
    }

    if (!_laserEnabled) {
        setLaserEnabled(true);
    }
    (void) _sendDatagram(UnipodMt11Protocol::buildLaserDistanceRequest(++_seq));
}

void UnipodMt11Client::setAiRecognitionEnabled(bool enabled)
{
    if (!isReady()) {
        emit sendFailed(tr("SIYI UDP camera not ready"));
        return;
    }

    if (!_sendDatagram(UnipodMt11Protocol::buildSetAiTrackModeCommand(++_seq, enabled))) {
        emit sendFailed(tr("Failed to send AI recognition command"));
        return;
    }

    if (_aiRecognitionEnabled != enabled) {
        _aiRecognitionEnabled = enabled;
        emit aiRecognitionEnabledChanged();
    }
}

bool UnipodMt11Client::_canStart()
{
    VideoSettings* videoSettings = SettingsManager::instance()->videoSettings();
    if (!videoSettings) {
        return false;
    }

    const QString source = videoSettings->videoSource()->rawValue().toString();
    if (source != VideoSettings::videoSourceUnipodMT11 && source != VideoSettings::videoSourceSiyiA8Mini &&
        source != VideoSettings::videoSourceSiyiZr10) {
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

bool UnipodMt11Client::_sendDatagram(const QByteArray& frame)
{
    if (!_socket) {
        qCWarning(UnipodMt11ClientLog) << "Send failed: socket not open";
        return false;
    }

    qCDebug(UnipodMt11ClientLog) << "Send" << frame.toHex(' ');

    const qint64 bytes =
        _socket->writeDatagram(frame, QHostAddress(UnipodMt11Protocol::kDefaultHost), UnipodMt11Protocol::kDefaultPort);
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

void UnipodMt11Client::_stopZoomHold()
{
    _zoomHoldDirection = 0;
    _zoomHoldMotorStopped = false;
    if (_zoomHoldTimer) {
        _zoomHoldTimer->setInterval(UnipodMt11Protocol::kCurrentZoomPollIntervalMs);
    }
}

void UnipodMt11Client::_maybeStopHoldAtOpticalLimit()
{
    if (_zoomHoldDirection == 0 || _zoomHoldMotorStopped || !isReady()) {
        return;
    }
    if (!UnipodMt11Protocol::holdZoomShouldStopMotor(_zoomHoldDirection, _zoomLevel, UnipodMt11Protocol::kHoldZoomMin,
                                                     UnipodMt11Protocol::kHoldZoomMaxDefault)) {
        return;
    }

    _zoomHoldMotorStopped = true;
    (void) _sendDatagram(UnipodMt11Protocol::buildZoomCommand(++_seq, 0));
    requestCurrentZoom();
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

void UnipodMt11Client::_onZoomHoldTimeout()
{
    if (!isReady()) {
        return;
    }

    if (_zoomHoldDirection != 0) {
        _maybeStopHoldAtOpticalLimit();
    }
    if (UnipodMt11Protocol::shouldRequestCurrentZoomOnPoll(_zoomHoldDirection, _zoomHoldMotorStopped)) {
        requestCurrentZoom();
    }
}

void UnipodMt11Client::_setZoomLevel(double zoomLevel)
{
    if (qIsNaN(zoomLevel) || zoomLevel <= 0.0) {
        return;
    }
    _zoomLevelStamp.restart();
    if (!qIsNaN(_zoomLevel) && qFuzzyCompare(_zoomLevel, zoomLevel)) {
        _maybeStopHoldAtOpticalLimit();
        return;
    }
    _zoomLevel = zoomLevel;
    emit zoomLevelChanged();
    _maybeStopHoldAtOpticalLimit();
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

        QByteArray remaining = datagram;
        bool parsedAny = false;
        while (remaining.size() >= 10) {
            quint8 ctrl = 0;
            quint16 seq = 0;
            quint8 cmd = 0;
            QByteArray payload;
            if (!UnipodMt11Protocol::parseFrame(remaining, &ctrl, &seq, &cmd, &payload)) {
                if (!parsedAny) {
                    qCWarning(UnipodMt11ClientLog) << "Dropped invalid frame:" << datagram.toHex(' ');
                }
                break;
            }
            parsedAny = true;

            qCDebug(UnipodMt11ClientLog) << "Recv cmd" << Qt::hex << cmd << "seq" << seq << "payload"
                                         << payload.toHex(' ') << Qt::dec;

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
            } else if (cmd == 0x05) {
                double zoom = 0.0;
                if (UnipodMt11Protocol::parseZoomMultipleAck(payload, &zoom)) {
                    _setZoomLevel(zoom);
                }
            } else if (cmd == 0x18) {
                double zoom = 0.0;
                if (UnipodMt11Protocol::parseCurrentZoomAck(payload, &zoom)) {
                    _setZoomLevel(zoom);
                }
            } else if (cmd == 0x15) {
                quint16 distanceDm = 0;
                if (UnipodMt11Protocol::parseLaserDistanceAck(payload, &distanceDm)) {
                    _laserDistanceMeters = static_cast<double>(distanceDm) / 10.0;
                    emit laserDistanceChanged();
                }
            }

            remaining = remaining.mid(10 + payload.size());
        }
    }
}
