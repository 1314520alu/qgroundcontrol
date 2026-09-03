#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(SatcomAntennaControllerLog)

class Fact;
class MyAircraftSettings;
class QTcpSocket;

/// \brief TCP client for the high-altitude satellite antenna (Weitong aircraft).
///
/// Connects as Client to the antenna Server (default 192.168.0.200:23). Frames are
/// ASCII CSV ending in \\r\\n. Telemetry is 20 Hz; control commands are one-shot
/// ASCII lines. Instantiated as a process singleton; call init() after SettingsManager.
class SatcomAntennaController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool telemetryValid READ telemetryValid NOTIFY telemetryValidChanged)
    Q_PROPERTY(QString connectionStatusText READ connectionStatusText NOTIFY connectionStatusTextChanged)
    Q_PROPERTY(int antennaState READ antennaState NOTIFY antennaStateChanged)
    Q_PROPERTY(QString antennaStateText READ antennaStateText NOTIFY antennaStateChanged)
    Q_PROPERTY(double signalStrength READ signalStrength NOTIFY signalStrengthChanged)
    Q_PROPERTY(int networkState READ networkState NOTIFY networkStateChanged)
    Q_PROPERTY(QString networkStateText READ networkStateText NOTIFY networkStateChanged)
    Q_PROPERTY(double azimuthDeg READ azimuthDeg NOTIFY azimuthDegChanged)
    Q_PROPERTY(double elevationDeg READ elevationDeg NOTIFY elevationDegChanged)

public:
    enum class AntennaState
    {
        Folded = 0,
        Searching = 1,
        Tracking = 2,
        Stowed = 3,
        FoldedAlt = 4,
    };
    Q_ENUM(AntennaState)

    enum class Command
    {
        Unfold,
        Fold,
        ElevationUp,
        ElevationDown,
        AzimuthUp,
        AzimuthDown,
    };
    Q_ENUM(Command)

    struct StatusFrame
    {
        int antennaState = 0;
        double signalStrength = 0.0;
        int networkState = 0;
        double azimuthDeg = 0.0;
        double elevationDeg = 0.0;
        bool valid = false;
    };

    explicit SatcomAntennaController(QObject* parent = nullptr);
    ~SatcomAntennaController() override;

    static SatcomAntennaController* instance();

    /// Wire settings and start/stop the link from the Weitong aircraft checkbox.
    void init();

    bool enabled() const { return _enabled; }

    bool connected() const { return _connected; }

    bool telemetryValid() const { return _telemetryValid; }

    QString connectionStatusText() const { return _connectionStatusText; }

    int antennaState() const { return _antennaState; }

    QString antennaStateText() const;

    double signalStrength() const { return _signalStrength; }

    int networkState() const { return _networkState; }

    QString networkStateText() const;

    double azimuthDeg() const { return _azimuthDeg; }

    double elevationDeg() const { return _elevationDeg; }

    [[nodiscard]] static StatusFrame parseStatusFrame(const QByteArray& line);
    [[nodiscard]] static QByteArray commandBytes(Command command);

    Q_INVOKABLE void unfoldAntenna() { sendCommand(Command::Unfold); }

    Q_INVOKABLE void foldAntenna() { sendCommand(Command::Fold); }

    Q_INVOKABLE void elevationUp() { sendCommand(Command::ElevationUp); }

    Q_INVOKABLE void elevationDown() { sendCommand(Command::ElevationDown); }

    Q_INVOKABLE void azimuthUp() { sendCommand(Command::AzimuthUp); }

    Q_INVOKABLE void azimuthDown() { sendCommand(Command::AzimuthDown); }

    Q_INVOKABLE void sendCommand(Command command);

signals:
    void enabledChanged();
    void connectedChanged();
    void telemetryValidChanged();
    void connectionStatusTextChanged();
    void antennaStateChanged();
    void signalStrengthChanged();
    void networkStateChanged();
    void azimuthDegChanged();
    void elevationDegChanged();

private:
    void _syncFromSettings();
    void _connectToAntenna();
    void _disconnectFromAntenna();
    void _scheduleReconnect();
    void _setConnected(bool connected);
    void _setConnectionStatusText(const QString& text);
    void _setTelemetryValid(bool valid);
    void _applyStatus(const StatusFrame& frame);
    void _onReadyRead();
    void _onSocketConnected();
    void _onSocketDisconnected();
    void _onSocketError();
    void _onStaleTimeout();

    MyAircraftSettings* _settings = nullptr;
    QTcpSocket* _socket = nullptr;
    QTimer _reconnectTimer;
    QTimer _staleTimer;
    QByteArray _rxBuffer;

    bool _enabled = false;
    bool _connected = false;
    bool _telemetryValid = false;
    QString _connectionStatusText;
    int _antennaState = 0;
    double _signalStrength = 0.0;
    int _networkState = 0;
    double _azimuthDeg = 0.0;
    double _elevationDeg = 0.0;
    QString _targetHost;
    quint16 _targetPort = 0;

    static constexpr int kReconnectMs = 3000;
    static constexpr int kStaleTelemetryMs = 2000;
};
