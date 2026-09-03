#include "SatcomAntennaController.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QtNumeric>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>

#include "Fact.h"
#include "MyAircraftSettings.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(SatcomAntennaControllerLog, "Satcom.SatcomAntennaController")

Q_APPLICATION_STATIC(SatcomAntennaController, _satcomAntennaControllerInstance);

SatcomAntennaController* SatcomAntennaController::instance()
{
    return _satcomAntennaControllerInstance();
}

SatcomAntennaController::SatcomAntennaController(QObject* parent) : QObject(parent)
{
    _reconnectTimer.setSingleShot(true);
    _reconnectTimer.setInterval(kReconnectMs);
    (void) connect(&_reconnectTimer, &QTimer::timeout, this, &SatcomAntennaController::_connectToAntenna);

    _staleTimer.setSingleShot(true);
    _staleTimer.setInterval(kStaleTelemetryMs);
    (void) connect(&_staleTimer, &QTimer::timeout, this, &SatcomAntennaController::_onStaleTimeout);

    _setConnectionStatusText(tr("Disabled"));
}

SatcomAntennaController::~SatcomAntennaController()
{
    _disconnectFromAntenna();
}

void SatcomAntennaController::init()
{
    _settings = SettingsManager::instance()->myAircraftSettings();
    if (!_settings) {
        qCWarning(SatcomAntennaControllerLog) << "MyAircraftSettings not available";
        return;
    }

    auto bindFact = [this](Fact* fact) {
        if (!fact) {
            return;
        }
        (void) connect(fact, &Fact::rawValueChanged, this, [this](QVariant) { _syncFromSettings(); });
    };
    bindFact(_settings->weitongAircraftEnabled());
    bindFact(_settings->satcomHost());
    bindFact(_settings->satcomPort());

    _syncFromSettings();
}

QString SatcomAntennaController::antennaStateText() const
{
    switch (_antennaState) {
        case static_cast<int>(AntennaState::Searching):
            return tr("Searching");
        case static_cast<int>(AntennaState::Tracking):
            return tr("Tracking");
        case static_cast<int>(AntennaState::Stowed):
            return tr("Stowed");
        case static_cast<int>(AntennaState::Folded):
        case static_cast<int>(AntennaState::FoldedAlt):
            return tr("Folded");
        default:
            return tr("Unknown");
    }
}

QString SatcomAntennaController::networkStateText() const
{
    return _networkState == 1 ? tr("Online") : tr("Offline");
}

SatcomAntennaController::StatusFrame SatcomAntennaController::parseStatusFrame(const QByteArray& line)
{
    StatusFrame frame;
    const QString trimmed = QString::fromLatin1(line).trimmed();
    if (trimmed.isEmpty()) {
        return frame;
    }

    const QStringList parts = trimmed.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (parts.size() != 7) {
        return frame;
    }

    QStringList fields;
    fields.reserve(7);
    for (const QString& part : parts) {
        fields.append(part.trimmed());
    }

    if (fields[0].compare(QStringLiteral("CC"), Qt::CaseInsensitive) != 0) {
        return frame;
    }
    if (fields[6].compare(QStringLiteral("FF"), Qt::CaseInsensitive) != 0) {
        return frame;
    }

    bool okState = false;
    bool okSignal = false;
    bool okNetwork = false;
    bool okAzimuth = false;
    bool okElevation = false;
    const int antennaState = fields[1].toInt(&okState);
    const double signalStrength = fields[2].toDouble(&okSignal);
    const int networkState = fields[3].toInt(&okNetwork);
    const double azimuthDeg = fields[4].toDouble(&okAzimuth);
    const double elevationDeg = fields[5].toDouble(&okElevation);
    if (!okState || !okSignal || !okNetwork || !okAzimuth || !okElevation) {
        return frame;
    }

    frame.antennaState = antennaState;
    frame.signalStrength = signalStrength;
    frame.networkState = networkState;
    frame.azimuthDeg = azimuthDeg;
    frame.elevationDeg = elevationDeg;
    frame.valid = true;
    return frame;
}

QByteArray SatcomAntennaController::commandBytes(Command command)
{
    switch (command) {
        case Command::Unfold:
            return QByteArrayLiteral("ant -open\r\n");
        case Command::Fold:
            return QByteArrayLiteral("ant -fold\r\n");
        case Command::ElevationUp:
            return QByteArrayLiteral("SPU\r\n");
        case Command::ElevationDown:
            return QByteArrayLiteral("SPD\r\n");
        case Command::AzimuthUp:
            return QByteArrayLiteral("SRL\r\n");
        case Command::AzimuthDown:
            return QByteArrayLiteral("SRR\r\n");
        default:
            break;
    }
    return {};
}

void SatcomAntennaController::sendCommand(Command command)
{
    if (!_socket || !_connected) {
        qCWarning(SatcomAntennaControllerLog) << "Dropping command; antenna not connected";
        return;
    }

    const QByteArray bytes = commandBytes(command);
    if (bytes.isEmpty()) {
        return;
    }

    const qint64 written = _socket->write(bytes);
    if (written != bytes.size()) {
        qCWarning(SatcomAntennaControllerLog) << "Failed to write antenna command";
        return;
    }
    (void) _socket->flush();
    qCDebug(SatcomAntennaControllerLog) << "Sent" << bytes.trimmed();
}

void SatcomAntennaController::_syncFromSettings()
{
    if (!_settings) {
        return;
    }

    Fact* const enabledFact = _settings->weitongAircraftEnabled();
    const bool enabled = enabledFact ? enabledFact->rawValue().toBool() : false;
    if (_enabled != enabled) {
        _enabled = enabled;
        emit enabledChanged();
    }

    if (!_enabled) {
        _reconnectTimer.stop();
        _disconnectFromAntenna();
        _setTelemetryValid(false);
        _setConnectionStatusText(tr("Disabled"));
        return;
    }

    _setConnectionStatusText(_connected ? tr("Connected") : tr("Connecting"));
    _connectToAntenna();
}

void SatcomAntennaController::_connectToAntenna()
{
    if (!_enabled || !_settings) {
        return;
    }

    Fact* const hostFact = _settings->satcomHost();
    Fact* const portFact = _settings->satcomPort();
    const QString host = hostFact ? hostFact->rawValue().toString().trimmed() : QString();
    const quint16 port = portFact ? static_cast<quint16>(portFact->rawValue().toUInt()) : 0;
    if (host.isEmpty() || port == 0) {
        _setConnectionStatusText(tr("Invalid host or port"));
        return;
    }

    if (_socket) {
        const QAbstractSocket::SocketState state = _socket->state();
        if (state == QAbstractSocket::ConnectedState || state == QAbstractSocket::ConnectingState) {
            if (_targetHost == host && _targetPort == port) {
                return;
            }
        }
    }

    _targetHost = host;
    _targetPort = port;
    _disconnectFromAntenna();

    _socket = new QTcpSocket(this);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    (void) connect(_socket, &QTcpSocket::connected, this, &SatcomAntennaController::_onSocketConnected);
    (void) connect(_socket, &QTcpSocket::disconnected, this, &SatcomAntennaController::_onSocketDisconnected);
    (void) connect(_socket, &QTcpSocket::readyRead, this, &SatcomAntennaController::_onReadyRead);
    (void) connect(_socket, &QTcpSocket::errorOccurred, this, &SatcomAntennaController::_onSocketError);

    _setConnectionStatusText(tr("Connecting"));
    qCDebug(SatcomAntennaControllerLog) << "Connecting to" << host << port;
    _socket->connectToHost(host, port);
}

void SatcomAntennaController::_disconnectFromAntenna()
{
    _staleTimer.stop();
    _rxBuffer.clear();
    _setTelemetryValid(false);
    _setConnected(false);

    if (!_socket) {
        return;
    }

    _socket->disconnect();
    _socket->abort();
    _socket->deleteLater();
    _socket = nullptr;
}

void SatcomAntennaController::_scheduleReconnect()
{
    if (!_enabled) {
        return;
    }
    if (!_reconnectTimer.isActive()) {
        _reconnectTimer.start();
    }
}

void SatcomAntennaController::_setConnected(bool connected)
{
    if (_connected == connected) {
        return;
    }
    _connected = connected;
    emit connectedChanged();
}

void SatcomAntennaController::_setConnectionStatusText(const QString& text)
{
    if (_connectionStatusText == text) {
        return;
    }
    _connectionStatusText = text;
    emit connectionStatusTextChanged();
}

void SatcomAntennaController::_setTelemetryValid(bool valid)
{
    if (_telemetryValid == valid) {
        return;
    }
    _telemetryValid = valid;
    emit telemetryValidChanged();
}

void SatcomAntennaController::_applyStatus(const StatusFrame& frame)
{
    if (_antennaState != frame.antennaState) {
        _antennaState = frame.antennaState;
        emit antennaStateChanged();
    }
    if (!qFuzzyCompare(_signalStrength + 1.0, frame.signalStrength + 1.0)) {
        _signalStrength = frame.signalStrength;
        emit signalStrengthChanged();
    }
    if (_networkState != frame.networkState) {
        _networkState = frame.networkState;
        emit networkStateChanged();
    }
    if (!qFuzzyCompare(_azimuthDeg + 1.0, frame.azimuthDeg + 1.0)) {
        _azimuthDeg = frame.azimuthDeg;
        emit azimuthDegChanged();
    }
    if (!qFuzzyCompare(_elevationDeg + 1.0, frame.elevationDeg + 1.0)) {
        _elevationDeg = frame.elevationDeg;
        emit elevationDegChanged();
    }
    _setTelemetryValid(true);
    _staleTimer.start();
}

void SatcomAntennaController::_onReadyRead()
{
    if (!_socket) {
        return;
    }

    _rxBuffer.append(_socket->readAll());
    while (true) {
        const int newline = _rxBuffer.indexOf('\n');
        if (newline < 0) {
            break;
        }
        QByteArray line = _rxBuffer.left(newline);
        _rxBuffer.remove(0, newline + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        const StatusFrame frame = parseStatusFrame(line);
        if (!frame.valid) {
            qCDebug(SatcomAntennaControllerLog) << "Ignored antenna frame" << line;
            continue;
        }
        _applyStatus(frame);
    }

    constexpr int kMaxBuffer = 4096;
    if (_rxBuffer.size() > kMaxBuffer) {
        _rxBuffer.remove(0, _rxBuffer.size() - kMaxBuffer);
    }
}

void SatcomAntennaController::_onSocketConnected()
{
    _reconnectTimer.stop();
    _setConnected(true);
    _setConnectionStatusText(tr("Connected"));
    qCDebug(SatcomAntennaControllerLog) << "Antenna connected";
}

void SatcomAntennaController::_onSocketDisconnected()
{
    _setConnected(false);
    _setTelemetryValid(false);
    if (_enabled) {
        _setConnectionStatusText(tr("Disconnected"));
        _scheduleReconnect();
    }
}

void SatcomAntennaController::_onSocketError()
{
    const QString error = _socket ? _socket->errorString() : tr("Socket error");
    qCWarning(SatcomAntennaControllerLog) << error;
    _setConnected(false);
    _setTelemetryValid(false);
    if (_enabled) {
        _setConnectionStatusText(error);
        _scheduleReconnect();
    }
}

void SatcomAntennaController::_onStaleTimeout()
{
    _setTelemetryValid(false);
}
