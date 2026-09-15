#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkMessageType.h"
#include "QmlObjectListModel.h"

class Vehicle;

class CameraMetaData;
class Joystick;
class MavlinkCameraControlInterface;
class QGCCameraManagerTest;
class QGCVideoStreamInfo;
class SimulatedCameraControl;
class SiyiA8MiniCameraControl;
class UnipodMt11CameraControl;
class UnipodMt11Client;
class UnipodMt11MediaClient;
class TopotekTq10CameraControl;
class TopotekTq10Client;

/// \brief Camera Manager
///
class QGCCameraManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Joystick.h")
    Q_MOC_INCLUDE("MavlinkCameraControlInterface.h")

    Q_PROPERTY(QmlObjectListModel* cameras READ cameras NOTIFY camerasChanged)
    Q_PROPERTY(QStringList cameraLabels READ cameraLabels NOTIFY cameraLabelsChanged)
    Q_PROPERTY(
        MavlinkCameraControlInterface* currentCameraInstance READ currentCameraInstance NOTIFY currentCameraChanged)
    Q_PROPERTY(int currentCamera READ currentCamera WRITE setCurrentCamera NOTIFY currentCameraChanged)
    Q_PROPERTY(int currentZoomLevel READ currentZoomLevel NOTIFY currentZoomLevelChanged)
    /// SIYI/UniPod UDP zoom multiple. Pass the value in NOTIFY so QML does not re-read a cached property.
    Q_PROPERTY(qreal siyiZoomLevel READ siyiZoomLevel NOTIFY siyiZoomLevelChanged)
    Q_PROPERTY(bool siyiZoomHudVisible READ siyiZoomHudVisible NOTIFY siyiZoomHudVisibleChanged)
    Q_PROPERTY(QString siyiZoomHudText READ siyiZoomHudText NOTIFY siyiZoomHudTextChanged)
    Q_PROPERTY(UnipodMt11MediaClient* unipodMediaClient READ unipodMediaClient CONSTANT)
    Q_PROPERTY(UnipodMt11Client* unipodClient READ unipodClient CONSTANT)
    Q_MOC_INCLUDE("UnipodMt11MediaClient.h")
    Q_MOC_INCLUDE("UnipodMt11Client.h")

#ifdef QGC_UNITTEST_BUILD
    friend class QGCCameraManagerTest;
#endif

public:
    explicit QGCCameraManager(Vehicle* vehicle);
    ~QGCCameraManager();

    struct CameraStruct
    {
        CameraStruct(QGCCameraManager* manager_, uint8_t compID_, Vehicle* vehicle_);
        ~CameraStruct();

        bool infoReceived = false;
        uint8_t compID = 0;
        int retryCount = 0;
        QElapsedTimer lastHeartbeat;
        QTimer backoffTimer;
        Vehicle*
            vehicle;  ///< Raw pointer is safe: CameraStruct is owned by QGCCameraManager which is a child of Vehicle
        QPointer<QGCCameraManager> manager;

    private:
        Q_DISABLE_COPY_MOVE(CameraStruct)
    };

    /// Stable context passed as opaque handler data to async camera info request
    /// callbacks. Owned by the manager and kept alive for its full lifetime so a
    /// callback firing after the CameraStruct was deleted (lost camera) never
    /// dereferences freed memory (issue #13251).
    struct CameraInfoRequestContext
    {
        QPointer<QGCCameraManager> manager;
        uint8_t compID = 0;
    };

    /// Returns the lazily created, manager-lifetime context for compId.
    CameraInfoRequestContext* cameraInfoContext(uint8_t compId);

    QmlObjectListModel* cameras() { return &_cameras; }

    const QmlObjectListModel* cameras() const { return &_cameras; }

    QStringList cameraLabels() const { return _cameraLabels; }

    int currentCamera() const { return _currentCameraIndex; }

    MavlinkCameraControlInterface* currentCameraInstance();

    UnipodMt11MediaClient* unipodMediaClient() const { return _unipodMediaClient; }

    UnipodMt11Client* unipodClient() const { return _unipodClient; }

    void setCurrentCamera(int sel);
    QGCVideoStreamInfo* currentStreamInstance();
    QGCVideoStreamInfo* thermalStreamInstance();

    const QVariantList& cameraList() const;

    Vehicle* vehicle() const { return _vehicle; }

    CameraStruct* findCameraStruct(uint8_t compId) const
    {
        return _cameraInfoRequest.value(QString::number(compId), nullptr);
    }

    int currentZoomLevel() const;

    qreal siyiZoomLevel() const { return _siyiZoomLevel; }

    /// Bypass QML property cache; always calls the C++ getter.
    Q_INVOKABLE qreal readSiyiZoomLevel() const { return _siyiZoomLevel; }

    bool siyiZoomHudVisible() const { return _siyiZoomHudVisible; }

    QString siyiZoomHudText() const { return _siyiZoomHudText; }

    /// Screen +/- zoom: show HUD immediately without waiting for a multiple change.
    Q_INVOKABLE void showSiyiZoomHud();

    double aspectForComp(int compId) const;
    double currentCameraAspect();
    Q_INVOKABLE void requestCameraFovForComp(int compId);

private:
    int _zoomValueCurrent = 0;
    qreal _siyiZoomLevel = 0;
    bool _siyiZoomHudVisible = false;
    QString _siyiZoomHudText;

signals:
    void camerasChanged();
    void cameraLabelsChanged();
    void currentCameraChanged();
    void streamChanged();

    void currentZoomLevelChanged();
    void siyiZoomLevelChanged(qreal siyiZoomLevel);
    void siyiZoomHudVisibleChanged(bool siyiZoomHudVisible);
    void siyiZoomHudTextChanged(const QString& siyiZoomHudText);

protected slots:
    void _vehicleReady(bool ready);
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _activeJoystickChanged(Joystick* joystick);
    void _stepZoom(int direction);
    void _startZoom(int direction);
    void _stopZoom();
    void _stepFocus(int direction);
    void _startFocus(int direction);
    void _stopFocus();
    void _stepCamera(int direction);
    void _stepStream(int direction);
    void _checkForLostCameras();
    void _triggerCamera();
    void _startVideoRecording();
    void _stopVideoRecording();
    void _toggleVideoRecording();

private slots:
    void _initialConnectCompleted();
    void _setCurrentZoomLevel(int level);
    void _onUnipodZoomLevelChanged();
    void _hideSiyiZoomHud();
    void _onUnipodStartRetry();
    void _onTopotekStartRetry();

private:
    MavlinkCameraControlInterface* _findCamera(int id);
    void _requestCameraInfo(CameraStruct* cameraInfo);
    void _handleHeartbeat(const mavlink_message_t& message);
    void _handleCameraInfo(const mavlink_message_t& message);
    void _handleStorageInformation(const mavlink_message_t& message);
    void _handleCameraSettings(const mavlink_message_t& message);
    void _handleParamExtAck(const mavlink_message_t& message);
    void _handleParamExtValue(const mavlink_message_t& message);
    void _handleCameraCaptureStatus(const mavlink_message_t& message);
    void _handleVideoStreamInformation(const mavlink_message_t& message);
    void _handleVideoStreamStatus(const mavlink_message_t& message);
    void _handleBatteryStatus(const mavlink_message_t& message);
    void _handleTrackingImageStatus(const mavlink_message_t& message);
    void _addCameraControlToLists(MavlinkCameraControlInterface* cameraControl);
    void _ensureSimulatedCameraForLocalRecord();
    void _syncSiyiUdpCamera();
    void _syncTopotekCamera();
    /// Only the video-source payload (UniPod / A8 Mini / ZR10 / Topotek) is visible as a camera.
    void _syncPayloadCameraList();
    void _handleCameraFovStatus(const mavlink_message_t& message);
    void _setSiyiZoomHudVisible(bool visible);
    void _updateSiyiZoomHudText();

    Vehicle* _vehicle;  ///< Raw pointer is safe: QGCCameraManager is a QObject child of Vehicle, so Vehicle always
                        ///< outlives us
    QPointer<SimulatedCameraControl> _simulatedCameraControl;
    UnipodMt11Client* _unipodClient = nullptr;
    UnipodMt11CameraControl* _unipodCameraControl = nullptr;
    SiyiA8MiniCameraControl* _siyiA8CameraControl = nullptr;
    UnipodMt11MediaClient* _unipodMediaClient = nullptr;
    TopotekTq10Client* _topotekClient = nullptr;
    TopotekTq10CameraControl* _topotekCameraControl = nullptr;
    QTimer _unipodStartRetryTimer;
    QTimer _topotekStartRetryTimer;
    QTimer _siyiZoomHudHideTimer;
    int _unipodStartRetryTicks = 0;
    int _topotekStartRetryTicks = 0;
    QPointer<Joystick> _activeJoystick;
    bool _vehicleReadyState = false;
    int _currentTask = 0;
    QmlObjectListModel _cameras;
    QStringList _cameraLabels;
    int _currentCameraIndex = 0;
    QElapsedTimer _lastZoomChange;
    QElapsedTimer _lastFocusChange;
    QElapsedTimer _lastCameraChange;
    QTimer _camerasLostHeartbeatTimer;
    QMap<QString, CameraStruct*> _cameraInfoRequest;
    QHash<uint8_t, CameraInfoRequestContext*> _cameraInfoContexts;
    static QVariantList _cameraList;
    bool _initialConnectComplete = false;

    QHash<int, double> _aspectByCompId;
};
