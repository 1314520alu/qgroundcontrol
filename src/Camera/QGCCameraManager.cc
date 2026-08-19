#include "QGCCameraManager.h"
#include "CameraMetaData.h"
#include "FirmwarePlugin.h"
#include "Joystick.h"
#include "JoystickManager.h"
#include "MAVLinkLib.h"
#include "MavlinkCameraControlInterface.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "SimulatedCameraControl.h"
#include "UnipodMt11CameraControl.h"
#include "UnipodMt11Client.h"
#include "UnipodMt11MediaClient.h"
#include "TopotekTq10CameraControl.h"
#include "TopotekTq10Client.h"
#include "Vehicle.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "SettingsManager.h"
#include "Fact.h"

#include <cmath>
#include "GimbalControllerSettings.h"
#include "SettingsManager.h"
#include <numbers>

constexpr double kPi = std::numbers::pi_v<double>;

QGC_LOGGING_CATEGORY(CameraManagerLog, "Camera.QGCCameraManager")

namespace {
    constexpr int kHeartbeatTickMs = 500;
    constexpr int kSilentTimeoutMs = 5000;
    constexpr int kMaxRetryCount = 10;
    constexpr int kUnipodStartRetryMs = 1000;
    constexpr int kUnipodStartSlowRetryMs = 5000;
    constexpr int kUnipodStartRetryMaxTicks = 60;
    constexpr int kTopotekStartRetryMs = 1000;
    constexpr int kTopotekStartSlowRetryMs = 5000;
    constexpr int kTopotekStartRetryMaxTicks = 60;

    bool isUnipodVideoSource()
    {
        SettingsManager *settingsManager = SettingsManager::instance();
        if (!settingsManager || !settingsManager->videoSettings() || !settingsManager->videoSettings()->videoSource()) {
            return false;
        }
        return settingsManager->videoSettings()->videoSource()->rawValue().toString()
               == QLatin1String(VideoSettings::videoSourceUnipodMT11);
    }

    bool isTopotekVideoSource()
    {
        SettingsManager *settingsManager = SettingsManager::instance();
        if (!settingsManager || !settingsManager->videoSettings() || !settingsManager->videoSettings()->videoSource()) {
            return false;
        }
        return settingsManager->videoSettings()->videoSource()->rawValue().toString()
               == QLatin1String(VideoSettings::videoSourceTopotekTq10N);
    }
}

QVariantList QGCCameraManager::_cameraList;

static void _requestFovOnZoom_Handler(
    void* user,
    MAV_RESULT result,
    Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
    const mavlink_message_t& message)
{
    auto* mgr = static_cast<QGCCameraManager*>(user);

    if (result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "CAMERA_FOV_STATUS request failed, result:"
                                  << result << "failure:" << failureCode;
        return;
    }

    if (message.msgid != MAVLINK_MSG_ID_CAMERA_FOV_STATUS) {
        qCDebug(CameraManagerLog) << "Unexpected msg id:" << message.msgid;
        return;
    }

    mavlink_camera_fov_status_t fov{};

    mavlink_msg_camera_fov_status_decode(&message, &fov);

    if (!mgr) return;
}

/*===========================================================================*/

QGCCameraManager::CameraStruct::CameraStruct(QGCCameraManager *manager_, uint8_t compID_, Vehicle *vehicle_)
    : compID(compID_)
    , vehicle(vehicle_)
    , manager(manager_)
{
    qCDebug(CameraManagerLog) << this;
    backoffTimer.setSingleShot(true);
}

QGCCameraManager::CameraStruct::~CameraStruct()
{
    qCDebug(CameraManagerLog) << this;
}

/*===========================================================================*/

QGCCameraManager::QGCCameraManager(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _simulatedCameraControl(new SimulatedCameraControl(vehicle, this))
    , _unipodClient(new UnipodMt11Client(this))
    , _unipodCameraControl(new UnipodMt11CameraControl(vehicle, _unipodClient, this))
    , _unipodMediaClient(new UnipodMt11MediaClient(this))
    , _topotekClient(new TopotekTq10Client(this))
    , _topotekCameraControl(new TopotekTq10CameraControl(vehicle, _topotekClient, this))
{
    qCDebug(CameraManagerLog) << this;

    (void) qRegisterMetaType<CameraMetaData*>("CameraMetaData*");

    _addCameraControlToLists(_simulatedCameraControl);

    if (_vehicle) {
        (void) connect(_vehicle, &Vehicle::initialConnectComplete, this, &QGCCameraManager::_initialConnectCompleted, Qt::UniqueConnection);
        (void) connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &QGCCameraManager::_mavlinkMessageReceived);
    }
    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &QGCCameraManager::_vehicleReady);
    (void) connect(&_camerasLostHeartbeatTimer, &QTimer::timeout, this, &QGCCameraManager::_checkForLostCameras);

    _camerasLostHeartbeatTimer.setSingleShot(false);
    _lastZoomChange.start();
    _lastFocusChange.start();
    _lastCameraChange.start();
    _camerasLostHeartbeatTimer.start(kHeartbeatTickMs);

    _unipodStartRetryTimer.setInterval(kUnipodStartRetryMs);
    (void) connect(&_unipodStartRetryTimer, &QTimer::timeout, this, &QGCCameraManager::_onUnipodStartRetry);

    _topotekStartRetryTimer.setInterval(kTopotekStartRetryMs);
    (void) connect(&_topotekStartRetryTimer, &QTimer::timeout, this, &QGCCameraManager::_onTopotekStartRetry);

    // After ethernet-loss stop(), the client may stay _active so setActive(true) is a no-op.
    // Re-run sync to call start() again and re-arm the retry window.
    (void) connect(_unipodClient, &UnipodMt11Client::readyChanged, this, [this]() {
        emit currentCameraChanged();
        if (_unipodMediaClient) {
            _unipodMediaClient->setReady(_unipodClient->isReady());
        }
        _syncUnipodCamera();
    });

    (void) connect(_topotekClient, &TopotekTq10Client::readyChanged, this, [this]() {
        emit currentCameraChanged();
        _syncTopotekCamera();
    });

    // Manual streams (RTSP/UDP) record via SimulatedCameraControl; keep it available
    // even after a MAVLink camera appears, and re-select it when the current camera can't capture.
    // UniPod MT11 must not fall through to that Simulated DIGICAM / local GST path.
    if (Fact *videoSource = SettingsManager::instance()->videoSettings()->videoSource()) {
        (void) connect(videoSource, &Fact::rawValueChanged, this, [this](const QVariant &) {
            _syncUnipodCamera();
            _syncTopotekCamera();
            _ensureSimulatedCameraForLocalRecord();
        });
    }
    if (VideoManager *videoManager = VideoManager::instance()) {
        (void) connect(videoManager, &VideoManager::hasVideoChanged, this, [this]() {
            _syncUnipodCamera();
            _syncTopotekCamera();
        });
        (void) connect(videoManager, &VideoManager::decodingChanged, this, [this]() {
            _syncUnipodCamera();
            _syncTopotekCamera();
        });
    }
    QTimer::singleShot(0, this, [this]() {
        _syncUnipodCamera();
        _syncTopotekCamera();
        _ensureSimulatedCameraForLocalRecord();
    });
}

void QGCCameraManager::_initialConnectCompleted()
{
    _initialConnectComplete = true;
}

QGCCameraManager::~QGCCameraManager()
{
    // Stop all camera info request timers and clean up
    for (auto* cameraInfo : _cameraInfoRequest) {
        cameraInfo->backoffTimer.stop();
        QObject::disconnect(&cameraInfo->backoffTimer, nullptr, nullptr, nullptr);
    }
    qDeleteAll(_cameraInfoRequest);
    _cameraInfoRequest.clear();

    qDeleteAll(_cameraInfoContexts);
    _cameraInfoContexts.clear();

    // Stop the main heartbeat timer
    _camerasLostHeartbeatTimer.stop();
    _unipodStartRetryTimer.stop();

    qCDebug(CameraManagerLog) << this;
}

QGCCameraManager::CameraInfoRequestContext* QGCCameraManager::cameraInfoContext(uint8_t compId)
{
    CameraInfoRequestContext*& context = _cameraInfoContexts[compId];
    if (!context) {
        context = new CameraInfoRequestContext{this, compId};
    }
    return context;
}

void QGCCameraManager::setCurrentCamera(int sel)
{
    if ((sel != _currentCameraIndex) && (sel >= 0) && (sel < _cameras.count())) {
        _currentCameraIndex = sel;
        emit currentCameraChanged();
        emit streamChanged();
    }
}

void QGCCameraManager::_vehicleReady(bool ready)
{
    qCDebug(CameraManagerLog) << ready;
    if (!ready) {
        return;
    }
    if (!_vehicle || (MultiVehicleManager::instance()->activeVehicle() != _vehicle)) {
        return;
    }

    _vehicleReadyState = true;
    _activeJoystickChanged(JoystickManager::instance()->activeJoystick());
    (void) connect(JoystickManager::instance(), &JoystickManager::activeJoystickChanged, this, &QGCCameraManager::_activeJoystickChanged, Qt::UniqueConnection);
}

void QGCCameraManager::_mavlinkMessageReceived(const mavlink_message_t &message)
{
    if (!_initialConnectComplete) {
        return;
    }

    // Only pay attention to camera components (MAV_COMP_ID_CAMERA..CAMERA6)
    // and camera-related messages proxied by the autopilot.
    const bool fromAutopilot = message.compid == MAV_COMP_ID_AUTOPILOT1;
    const bool fromCamera = (message.compid >= MAV_COMP_ID_CAMERA) && (message.compid <= MAV_COMP_ID_CAMERA6);
    if ((message.sysid == _vehicle->id()) && (fromAutopilot || fromCamera)) {
        switch (message.msgid) {
        case MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS:
            _handleCameraCaptureStatus(message);
            break;
        case MAVLINK_MSG_ID_STORAGE_INFORMATION:
            _handleStorageInformation(message);
            break;
        case MAVLINK_MSG_ID_HEARTBEAT:
            // Autopilot heartbeats should not be treated as camera discovery.
            // Only actual camera component heartbeats should start CAMERA_INFORMATION requests.
            if (fromCamera) {
                _handleHeartbeat(message);
            }
            break;
        case MAVLINK_MSG_ID_CAMERA_INFORMATION:
            _handleCameraInfo(message);
            break;
        case MAVLINK_MSG_ID_CAMERA_SETTINGS:
            _handleCameraSettings(message);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_ACK:
            _handleParamExtAck(message);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_VALUE:
            _handleParamExtValue(message);
            break;
        case MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION:
            _handleVideoStreamInformation(message);
            break;
        case MAVLINK_MSG_ID_VIDEO_STREAM_STATUS:
            _handleVideoStreamStatus(message);
            break;
        case MAVLINK_MSG_ID_BATTERY_STATUS:
            _handleBatteryStatus(message);
            break;
        case MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS:
            _handleTrackingImageStatus(message);
            break;
        case MAVLINK_MSG_ID_CAMERA_FOV_STATUS:
            _handleCameraFovStatus(message);
            break;
        default:
            break;
        }
    }
}

void QGCCameraManager::_handleHeartbeat(const mavlink_message_t &message)
{
    const QString sCompID = QString::number(message.compid);

    if (!_cameraInfoRequest.contains(sCompID)) {
        qCDebug(CameraManagerLog) << "Heartbeat from" << QGCMAVLink::compIdToString(message.compid);
        CameraStruct *pInfo = new CameraStruct(this, message.compid, _vehicle);
        pInfo->lastHeartbeat.start();
        _cameraInfoRequest[sCompID] = pInfo;
        _requestCameraInfo(pInfo);
        return;
    }

    CameraStruct *pInfo = _cameraInfoRequest[sCompID];
    if (!pInfo) {
        qCWarning(CameraManagerLog) << sCompID << "is null";
        return;
    }

    if (pInfo->infoReceived) {
        pInfo->lastHeartbeat.start();
        return;
    }

    if (pInfo->lastHeartbeat.elapsed() > kSilentTimeoutMs) {
        qCDebug(CameraManagerLog) << "Camera" << QGCMAVLink::compIdToString(message.compid) << "reappeared after being silent. Resetting retry count and requesting info.";
        pInfo->retryCount = 0;
        pInfo->backoffTimer.stop();
        pInfo->lastHeartbeat.start();
        _requestCameraInfo(pInfo);
        return;
    }

    pInfo->lastHeartbeat.start();
}

MavlinkCameraControlInterface *QGCCameraManager::currentCameraInstance()
{
    // While UniPod MT11 is the selected video source, always expose UniPod control so
    // PhotoVideoControl does not fall through to Simulated DIGICAM / local GST. Buttons
    // stay Disabled via capture*State until the UDP client is ready.
    if (_unipodCameraControl && isUnipodVideoSource()) {
        return _unipodCameraControl;
    }

    if (_topotekCameraControl && isTopotekVideoSource()) {
        return _topotekCameraControl;
    }

    if ((_currentCameraIndex < _cameras.count()) && !_cameras.isEmpty()) {
        MavlinkCameraControlInterface *pCamera = qobject_cast<MavlinkCameraControlInterface*>(_cameras[_currentCameraIndex]);
        return pCamera;
    }
    return nullptr;
}

QGCVideoStreamInfo *QGCCameraManager::currentStreamInstance()
{
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        QGCVideoStreamInfo *pInfo = pCamera->currentStreamInstance();
        return pInfo;
    }
    return nullptr;
}

QGCVideoStreamInfo *QGCCameraManager::thermalStreamInstance()
{
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        QGCVideoStreamInfo *pInfo = pCamera->thermalStreamInstance();
        return pInfo;
    }
    return nullptr;
}

MavlinkCameraControlInterface *QGCCameraManager::_findCamera(int id)
{
    for (int i = 0; i < _cameras.count(); i++) {
        if (!_cameras[i]) {
            continue;
        }
        MavlinkCameraControlInterface *pCamera = qobject_cast<MavlinkCameraControlInterface*>(_cameras[i]);
        if (!pCamera) {
            qCCritical(CameraManagerLog) << "Invalid MavlinkCameraControlInterface instance";
            continue;
        }
        if (pCamera->compID() == id) {
            return pCamera;
        }
    }

    // qCWarning(CameraManagerLog) << "Camera component id not found:" << id;
    return nullptr;
}

void QGCCameraManager::_addCameraControlToLists(MavlinkCameraControlInterface *cameraControl)
{
    if (qobject_cast<SimulatedCameraControl*>(cameraControl)) {
        qCDebug(CameraManagerLog) << "Adding simulated camera to list";
    } else {
        qCDebug(CameraManagerLog) << "Adding real camera to list";
    }

    _cameras.append(cameraControl);
    _cameraLabels.append(cameraControl->modelName());
    emit camerasChanged();
    emit cameraLabelsChanged();

    // Drop the simulated camera when a real MAVLink camera appears — unless QGC is using a
    // manual stream source (RTSP/UDP/UniPod/…). Local VideoManager record/screenshot depends on
    // SimulatedCameraControl; removing it leaves PhotoVideoControl empty even with showRecControl on.
    if ((_cameras.count() == 2) && (_cameras[0] == _simulatedCameraControl)) {
        const bool keepSimulatedForManualStream =
            VideoManager::instance() && VideoManager::instance()->isManualStreamSource();
        if (keepSimulatedForManualStream) {
            qCInfo(CameraManagerLog)
                << "Keeping simulated camera alongside real camera (manual video stream active)";
        } else {
            qCDebug(CameraManagerLog) << "Removing simulated camera after real camera appeared";
            (void) _cameras.removeAt(0);
            (void) _cameraLabels.removeAt(0);
            emit camerasChanged();
            emit cameraLabelsChanged();
            emit currentCameraChanged();
        }
    }

    _ensureSimulatedCameraForLocalRecord();
}

void QGCCameraManager::_ensureSimulatedCameraForLocalRecord()
{
    if (!_simulatedCameraControl) {
        return;
    }
    // UniPod MT11 / Topotek TQ10N onboard capture must own the strip; do not re-select Simulated.
    if (isUnipodVideoSource() || isTopotekVideoSource()) {
        return;
    }
    if (!VideoManager::instance() || !VideoManager::instance()->isManualStreamSource()) {
        return;
    }

    int simIdx = _cameras.indexOf(_simulatedCameraControl.data());
    if (simIdx < 0) {
        qCInfo(CameraManagerLog) << "Re-adding simulated camera for manual stream local record UI";
        _cameras.insert(0, _simulatedCameraControl);
        _cameraLabels.insert(0, _simulatedCameraControl->modelName());
        emit camerasChanged();
        emit cameraLabelsChanged();
        simIdx = 0;
        if (_currentCameraIndex >= 0) {
            _currentCameraIndex += 1;
        }
    }

    MavlinkCameraControlInterface *cur = currentCameraInstance();
    const bool curCanShowControls = cur && (cur->capturesVideo() || cur->capturesPhotos() || cur->hasTracking() ||
                                            cur->hasVideoStream());
    if (!curCanShowControls && (simIdx >= 0) && (_currentCameraIndex != simIdx)) {
        qCInfo(CameraManagerLog) << "Selecting simulated camera for PhotoVideoControl (current camera has no capture UI)";
        setCurrentCamera(simIdx);
    }
}

void QGCCameraManager::_syncUnipodCamera()
{
    if (!_unipodClient) {
        return;
    }

    VideoSettings *videoSettings = SettingsManager::instance() ? SettingsManager::instance()->videoSettings() : nullptr;
    if (!videoSettings || !videoSettings->videoSource()) {
        return;
    }

    const QString source = videoSettings->videoSource()->rawValue().toString();
    const bool wantUnipod = (source == VideoSettings::videoSourceUnipodMT11);

    if (wantUnipod) {
        _unipodClient->setActive(true);
        // setActive(true) is a no-op if already active; after ethernet-loss stop() the
        // client stays _active and must be start()'d again.
        _unipodClient->start();
        if (_unipodMediaClient) {
            _unipodMediaClient->setReady(_unipodClient->isReady());
        }
        if (!_unipodClient->isReady()) {
            if (!_unipodStartRetryTimer.isActive()) {
                _unipodStartRetryTicks = 0;
                _unipodStartRetryTimer.setInterval(kUnipodStartRetryMs);
                _unipodStartRetryTimer.start();
            } else if (_unipodStartRetryTimer.interval() != kUnipodStartRetryMs) {
                // Video/ethernet came up after the slow-retry window — resume 1s attempts.
                qCInfo(CameraManagerLog) << "UniPod start retry resuming at 1s";
                _unipodStartRetryTicks = 0;
                _unipodStartRetryTimer.setInterval(kUnipodStartRetryMs);
            }
        } else {
            _unipodStartRetryTimer.stop();
            _unipodStartRetryTimer.setInterval(kUnipodStartRetryMs);
            _unipodStartRetryTicks = 0;
        }
    } else {
        _unipodStartRetryTimer.stop();
        _unipodStartRetryTimer.setInterval(kUnipodStartRetryMs);
        _unipodStartRetryTicks = 0;
        _unipodClient->setActive(false);
        _unipodClient->stop();
        if (_unipodMediaClient) {
            _unipodMediaClient->setReady(false);
        }
    }

    emit currentCameraChanged();
}

void QGCCameraManager::_onUnipodStartRetry()
{
    if (!isUnipodVideoSource()) {
        _unipodStartRetryTimer.stop();
        _unipodStartRetryTimer.setInterval(kUnipodStartRetryMs);
        _unipodStartRetryTicks = 0;
        return;
    }

    ++_unipodStartRetryTicks;

    if (_unipodClient) {
        _unipodClient->start();
    }

    if (_unipodClient && _unipodClient->isReady()) {
        if (_unipodMediaClient) {
            _unipodMediaClient->setReady(true);
        }
        _unipodStartRetryTimer.stop();
        _unipodStartRetryTimer.setInterval(kUnipodStartRetryMs);
        _unipodStartRetryTicks = 0;
        emit currentCameraChanged();
        return;
    }

    if (_unipodStartRetryTicks == kUnipodStartRetryMaxTicks) {
        qCWarning(CameraManagerLog) << "UniPod camera start retry slowing to 5s (will keep trying)";
        _unipodStartRetryTimer.setInterval(kUnipodStartSlowRetryMs);
    }
}

void QGCCameraManager::_syncTopotekCamera()
{
    if (!_topotekClient) {
        return;
    }

    VideoSettings *videoSettings = SettingsManager::instance() ? SettingsManager::instance()->videoSettings() : nullptr;
    if (!videoSettings || !videoSettings->videoSource()) {
        return;
    }

    const QString source = videoSettings->videoSource()->rawValue().toString();
    const bool wantTopotek = (source == VideoSettings::videoSourceTopotekTq10N);

    if (wantTopotek) {
        _topotekClient->setActive(true);
        _topotekClient->start();
        if (!_topotekClient->isReady()) {
            if (!_topotekStartRetryTimer.isActive()) {
                _topotekStartRetryTicks = 0;
                _topotekStartRetryTimer.setInterval(kTopotekStartRetryMs);
                _topotekStartRetryTimer.start();
            } else if (_topotekStartRetryTimer.interval() != kTopotekStartRetryMs) {
                qCInfo(CameraManagerLog) << "Topotek start retry resuming at 1s";
                _topotekStartRetryTicks = 0;
                _topotekStartRetryTimer.setInterval(kTopotekStartRetryMs);
            }
        } else {
            _topotekStartRetryTimer.stop();
            _topotekStartRetryTimer.setInterval(kTopotekStartRetryMs);
            _topotekStartRetryTicks = 0;
        }
    } else {
        _topotekStartRetryTimer.stop();
        _topotekStartRetryTimer.setInterval(kTopotekStartRetryMs);
        _topotekStartRetryTicks = 0;
        _topotekClient->setActive(false);
        _topotekClient->stop();
    }

    emit currentCameraChanged();
}

void QGCCameraManager::_onTopotekStartRetry()
{
    if (!isTopotekVideoSource()) {
        _topotekStartRetryTimer.stop();
        _topotekStartRetryTimer.setInterval(kTopotekStartRetryMs);
        _topotekStartRetryTicks = 0;
        return;
    }

    ++_topotekStartRetryTicks;

    if (_topotekClient) {
        _topotekClient->start();
    }

    if (_topotekClient && _topotekClient->isReady()) {
        _topotekStartRetryTimer.stop();
        _topotekStartRetryTimer.setInterval(kTopotekStartRetryMs);
        _topotekStartRetryTicks = 0;
        emit currentCameraChanged();
        return;
    }

    if (_topotekStartRetryTicks == kTopotekStartRetryMaxTicks) {
        qCWarning(CameraManagerLog) << "Topotek camera start retry slowing to 5s (will keep trying)";
        _topotekStartRetryTimer.setInterval(kTopotekStartSlowRetryMs);
    }
}

void QGCCameraManager::_handleCameraInfo(const mavlink_message_t& message)
{
    const QString sCompID = QString::number(message.compid);
    if (!_cameraInfoRequest.contains(sCompID)) {
        qCDebug(CameraManagerLog) << "Ignoring - Camera info not requested for component" << QGCMAVLink::compIdToString(message.compid);
        return;
    }
    if (_cameraInfoRequest[sCompID]->infoReceived) {
        qCDebug(CameraManagerLog) << "Ignoring - Already received camera info for component" << QGCMAVLink::compIdToString(message.compid);
        return;
    }

    mavlink_camera_information_t info{};
    mavlink_msg_camera_information_decode(&message, &info);
    qCDebug(CameraManagerLog) << "Camera information received from" << QGCMAVLink::compIdToString(message.compid)
                          << "Model:" << reinterpret_cast<const char*>(info.model_name);
    qCDebug(CameraManagerLog) << "Creating MavlinkCameraControlInterface for camera";

    MavlinkCameraControlInterface *pCamera = _vehicle->firmwarePlugin()->createCameraControl(&info, _vehicle, message.compid, this);
    if (pCamera) {
        _addCameraControlToLists(pCamera);

        _cameraInfoRequest[sCompID]->infoReceived = true;
        _cameraInfoRequest[sCompID]->retryCount = 0;
        _cameraInfoRequest[sCompID]->backoffTimer.stop();
        qCDebug(CameraManagerLog) << "Success for compId" << QGCMAVLink::compIdToString(message.compid) << "- reset retry counter";
    }

    double aspect = std::numeric_limits<double>::quiet_NaN();

    if (info.resolution_h > 0 && info.resolution_v > 0) {
        aspect = double(info.resolution_v) / double(info.resolution_h);
    } else if (info.sensor_size_h > 0.f && info.sensor_size_v > 0.f) {
        aspect = double(info.sensor_size_v) / double(info.sensor_size_h);
    }

    _aspectByCompId.insert(message.compid, aspect);
}

void QGCCameraManager::_checkForLostCameras()
{
    QList<QString> stale;
    for (auto it = _cameraInfoRequest.cbegin(), end = _cameraInfoRequest.cend(); it != end; ++it) {
        const auto *info = it.value();
        if (info && info->infoReceived && (info->lastHeartbeat.elapsed() > kSilentTimeoutMs)) {
            stale.push_back(it.key());
        }
    }
    if (stale.isEmpty()) {
        return;
    }

    bool removedAny = false;
    for (const QString& key : std::as_const(stale)) {
        CameraStruct* pInfo = _cameraInfoRequest.take(key);
        if (!pInfo) {
            continue;
        }

        MavlinkCameraControlInterface* pCamera = _findCamera(pInfo->compID);
        if (pCamera) {
            const int idx = _cameras.indexOf(pCamera);
            if (idx >= 0) {
                qCDebug(CameraManagerLog) << "Removing lost camera" << QGCMAVLink::compIdToString(pInfo->compID);
                removedAny = true;
                (void) _cameraLabels.removeAt(idx);
                (void) _cameras.removeAt(idx);
                pCamera->deleteLater();
            }
        }

        delete pInfo;
    }

    if (!removedAny) {
        return;
    }

    if (_cameras.isEmpty()) {
        _addCameraControlToLists(_simulatedCameraControl);
    }

    emit cameraLabelsChanged();
    emit camerasChanged();

    if (_currentCameraIndex != 0) {
        _currentCameraIndex = 0;
        emit currentCameraChanged();
    }
    emit streamChanged();
}

void QGCCameraManager::_handleCameraCaptureStatus(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_camera_capture_status_t cap{};
        mavlink_msg_camera_capture_status_decode(&message, &cap);
        pCamera->handleCameraCaptureStatus(cap);
    }
}

void QGCCameraManager::_handleStorageInformation(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_storage_information_t st{};
        mavlink_msg_storage_information_decode(&message, &st);
        pCamera->handleStorageInformation(st);
    }
}

void QGCCameraManager::_handleCameraSettings(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_camera_settings_t settings{};
        mavlink_msg_camera_settings_decode(&message, &settings);
        pCamera->handleCameraSettings(settings);

        const int newZoom = static_cast<int>(settings.zoomLevel);
        if (QThread::currentThread() == thread()) {
            _setCurrentZoomLevel(newZoom);
        } else {
            QMetaObject::invokeMethod(
                this,
                "_setCurrentZoomLevel",
                Qt::QueuedConnection,
                Q_ARG(int, newZoom)
            );
        }

        requestCameraFovForComp(message.compid);
    }
}

void QGCCameraManager::_handleParamExtAck(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_param_ext_ack_t ack{};
        mavlink_msg_param_ext_ack_decode(&message, &ack);
        pCamera->handleParamExtAck(ack);
    }
}

void QGCCameraManager::_handleParamExtValue(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_param_ext_value_t value{};
        mavlink_msg_param_ext_value_decode(&message, &value);
        pCamera->handleParamExtValue(value);
    }
}

void QGCCameraManager::_handleVideoStreamInformation(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_video_stream_information_t streamInfo{};
        mavlink_msg_video_stream_information_decode(&message, &streamInfo);
        pCamera->handleVideoStreamInformation(streamInfo);
        emit streamChanged();
    }
}

void QGCCameraManager::_handleVideoStreamStatus(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_video_stream_status_t streamStatus{};
        mavlink_msg_video_stream_status_decode(&message, &streamStatus);
        pCamera->handleVideoStreamStatus(streamStatus);
    }
}

void QGCCameraManager::_handleBatteryStatus(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_battery_status_t batteryStatus{};
        mavlink_msg_battery_status_decode(&message, &batteryStatus);
        pCamera->handleBatteryStatus(batteryStatus);
    }
}

void QGCCameraManager::_handleTrackingImageStatus(const mavlink_message_t &message)
{
    MavlinkCameraControlInterface *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_camera_tracking_image_status_t tis{};
        mavlink_msg_camera_tracking_image_status_decode(&message, &tis);
        pCamera->handleTrackingImageStatus(tis);
    }
}

static void _handleCameraInfoRetry(QGCCameraManager::CameraStruct *cameraInfo);

/// Resolves the opaque callback context back to the live CameraStruct. Returns nullptr
/// if the manager is gone or the camera was removed while the request was in flight.
static QGCCameraManager::CameraStruct* _cameraStructFromContext(void *resultHandlerData)
{
    auto *context = static_cast<QGCCameraManager::CameraInfoRequestContext*>(resultHandlerData);
    if (!context->manager) {
        return nullptr;
    }

    QGCCameraManager::CameraStruct *cameraInfo = context->manager->findCameraStruct(context->compID);
    if (!cameraInfo) {
        qCDebug(CameraManagerLog) << "Camera info request callback for removed camera. compId" << QGCMAVLink::compIdToString(context->compID);
    }
    return cameraInfo;
}

static void _requestCameraInfoCommandResultHandler(void *resultHandlerData, int /*compId*/, const mavlink_command_ack_t &ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    QGCCameraManager::CameraStruct *cameraInfo = _cameraStructFromContext(resultHandlerData);
    if (!cameraInfo) {
        return;
    }

    if (ack.result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "MAV_CMD_REQUEST_CAMERA_INFORMATION failed. compId" << QGCMAVLink::compIdToString(cameraInfo->compID)
                                    << "Result:" << QGCMAVLink::mavResultToString(ack.result)
                                    << "FailureCode:" << Vehicle::mavCmdResultFailureCodeToString(failureCode)
                                    << "retryCount:" << cameraInfo->retryCount;
        _handleCameraInfoRetry(cameraInfo);
    }
}

static void _requestCameraInfoMessageResultHandler(void *resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, [[maybe_unused]] const mavlink_message_t &message)
{
    QGCCameraManager::CameraStruct *cameraInfo = _cameraStructFromContext(resultHandlerData);
    if (!cameraInfo) {
        return;
    }

    if (result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "MAV_CMD_REQUEST_MESSAGE:MAVLINK_MSG_ID_CAMERA_INFORMATION failed. compId" << QGCMAVLink::compIdToString(cameraInfo->compID)
                                    << "Result:" << QGCMAVLink::mavResultToString(result)
                                    << "FailureCode:" << Vehicle::requestMessageResultHandlerFailureCodeToString(failureCode)
                                    << "retryCount:" << cameraInfo->retryCount;
        _handleCameraInfoRetry(cameraInfo);
    }
}

static void _requestCameraInfoHelper(QGCCameraManager *manager, QGCCameraManager::CameraStruct *pInfo)
{
    // Give up after max attempts
    if (pInfo->retryCount >= kMaxRetryCount) {
        qCDebug(CameraManagerLog) << "Giving up requesting camera info after" << pInfo->retryCount << "attempts for compId" << QGCMAVLink::compIdToString(pInfo->compID);
        return;
    }

    // Alternate between REQUEST_MESSAGE and REQUEST_CAMERA_INFORMATION
    if ((pInfo->retryCount % 2) == 0) {
        qCDebug(CameraManagerLog) << "Using MAV_CMD_REQUEST_MESSAGE:CAMERA_INFORMATION for compId" << QGCMAVLink::compIdToString(pInfo->compID);
        manager->vehicle()->requestMessage(_requestCameraInfoMessageResultHandler, manager->cameraInfoContext(pInfo->compID), pInfo->compID, MAVLINK_MSG_ID_CAMERA_INFORMATION);
    } else {
        qCDebug(CameraManagerLog) << "Using MAV_CMD_REQUEST_CAMERA_INFORMATION for compId" << QGCMAVLink::compIdToString(pInfo->compID);

        Vehicle::MavCmdAckHandlerInfo_t ackHandlerInfo{};
        ackHandlerInfo.resultHandler        = _requestCameraInfoCommandResultHandler;
        ackHandlerInfo.resultHandlerData    = manager->cameraInfoContext(pInfo->compID);
        ackHandlerInfo.progressHandler      = nullptr;
        ackHandlerInfo.progressHandlerData  = nullptr;

        pInfo->vehicle->sendMavCommandWithHandler(&ackHandlerInfo, pInfo->compID, MAV_CMD_REQUEST_CAMERA_INFORMATION, 1 /* request camera capabilities */);
    }
}

static void _handleCameraInfoRetry(QGCCameraManager::CameraStruct *cameraInfo)
{
    if (!cameraInfo) {
        return;
    }

    QGCCameraManager *manager = cameraInfo->manager;
    if (!manager) {
        qCDebug(CameraManagerLog) << "manager is unavailable for compId" << QGCMAVLink::compIdToString(cameraInfo->compID);
        return;
    }

    cameraInfo->retryCount++;

    // For even attempts >= 2, use exponential backoff
    if ((cameraInfo->retryCount >= 2) && ((cameraInfo->retryCount % 2) == 0)) {
        const int delaySeconds = 1 << (cameraInfo->retryCount / 2);
        const int delayMs = delaySeconds * 1000;

        qCDebug(CameraManagerLog) << "Waiting" << delaySeconds << "seconds before retry for compId" << QGCMAVLink::compIdToString(cameraInfo->compID);

        cameraInfo->backoffTimer.stop();
        (void) QObject::disconnect(&cameraInfo->backoffTimer, nullptr, nullptr, nullptr);

        // Capture compID by value and look up the struct
        const uint8_t compId = cameraInfo->compID;
        QPointer<QGCCameraManager> mgrGuard(manager);

        (void) QObject::connect(&cameraInfo->backoffTimer, &QTimer::timeout,
                                &cameraInfo->backoffTimer,  // context ensures timer is still alive
                                [mgrGuard, compId]() {
            if (!mgrGuard) {
                return;
            }

            auto* info = mgrGuard->findCameraStruct(compId);
            if (info) {
                _requestCameraInfoHelper(mgrGuard.data(), info);
            }
        });

        cameraInfo->backoffTimer.start(delayMs);
    } else {
        _requestCameraInfoHelper(manager, cameraInfo);
    }
}

void QGCCameraManager::_requestCameraInfo(CameraStruct *pInfo)
{
    if (!pInfo) {
        return;
    }
    _requestCameraInfoHelper(this, pInfo);
}

void QGCCameraManager::_activeJoystickChanged(Joystick *joystick)
{
    qCDebug(CameraManagerLog) << "Joystick changed";
    if (_activeJoystick) {
        (void) disconnect(_activeJoystick, &Joystick::stepZoom,             this, &QGCCameraManager::_stepZoom);
        (void) disconnect(_activeJoystick, &Joystick::startContinuousZoom,  this, &QGCCameraManager::_startZoom);
        (void) disconnect(_activeJoystick, &Joystick::stopContinuousZoom,   this, &QGCCameraManager::_stopZoom);
        (void) disconnect(_activeJoystick, &Joystick::stepFocus,            this, &QGCCameraManager::_stepFocus);
        (void) disconnect(_activeJoystick, &Joystick::startContinuousFocus, this, &QGCCameraManager::_startFocus);
        (void) disconnect(_activeJoystick, &Joystick::stopContinuousFocus,  this, &QGCCameraManager::_stopFocus);
        (void) disconnect(_activeJoystick, &Joystick::stepCamera,          this, &QGCCameraManager::_stepCamera);
        (void) disconnect(_activeJoystick, &Joystick::stepStream,          this, &QGCCameraManager::_stepStream);
        (void) disconnect(_activeJoystick, &Joystick::triggerCamera,       this, &QGCCameraManager::_triggerCamera);
        (void) disconnect(_activeJoystick, &Joystick::startVideoRecord,    this, &QGCCameraManager::_startVideoRecording);
        (void) disconnect(_activeJoystick, &Joystick::stopVideoRecord,     this, &QGCCameraManager::_stopVideoRecording);
        (void) disconnect(_activeJoystick, &Joystick::toggleVideoRecord,   this, &QGCCameraManager::_toggleVideoRecording);
    }

    _activeJoystick = joystick;

    if (_activeJoystick) {
        (void) connect(_activeJoystick, &Joystick::stepZoom,             this, &QGCCameraManager::_stepZoom, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::startContinuousZoom,  this, &QGCCameraManager::_startZoom, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stopContinuousZoom,   this, &QGCCameraManager::_stopZoom, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stepFocus,            this, &QGCCameraManager::_stepFocus, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::startContinuousFocus, this, &QGCCameraManager::_startFocus, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stopContinuousFocus,  this, &QGCCameraManager::_stopFocus, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stepCamera,          this, &QGCCameraManager::_stepCamera, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stepStream,          this, &QGCCameraManager::_stepStream, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::triggerCamera,       this, &QGCCameraManager::_triggerCamera, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::startVideoRecord,    this, &QGCCameraManager::_startVideoRecording, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stopVideoRecord,     this, &QGCCameraManager::_stopVideoRecording, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::toggleVideoRecord,   this, &QGCCameraManager::_toggleVideoRecording, Qt::UniqueConnection);
    }
}

void QGCCameraManager::_triggerCamera()
{
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->takePhoto();
    }
}

void QGCCameraManager::_startVideoRecording()
{
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->startVideoRecording();
    }
}

void QGCCameraManager::_stopVideoRecording()
{
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->stopVideoRecording();
    }
}

void QGCCameraManager::_toggleVideoRecording()
{
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->toggleVideoRecording();
    }
}

void QGCCameraManager::_stepZoom(int direction)
{
    if (_lastZoomChange.elapsed() > 40) {
        _lastZoomChange.start();
        qCDebug(CameraManagerLog) << "Step Camera Zoom" << direction;
        MavlinkCameraControlInterface *pCamera = currentCameraInstance();
        if (pCamera) {
            pCamera->stepZoom(direction);
        }
    }
}

void QGCCameraManager::_startZoom(int direction)
{
    qCDebug(CameraManagerLog) << "Start Camera Zoom" << direction;
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->startZoom(direction);
    }
}

void QGCCameraManager::_stopZoom()
{
    qCDebug(CameraManagerLog) << "Stop Camera Zoom";
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->stopZoom();
    }
}

void QGCCameraManager::_stepFocus(int direction)
{
    if (_lastFocusChange.elapsed() > 40) {
        _lastFocusChange.start();
        qCDebug(CameraManagerLog) << "Step Camera Focus" << direction;
        MavlinkCameraControlInterface *pCamera = currentCameraInstance();
        if (pCamera) {
            pCamera->stepFocus(direction);
        }
    }
}

void QGCCameraManager::_startFocus(int direction)
{
    qCDebug(CameraManagerLog) << "Start Camera Focus" << direction;
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->startFocus(direction);
    }
}

void QGCCameraManager::_stopFocus()
{
    qCDebug(CameraManagerLog) << "Stop Camera Focus";
    MavlinkCameraControlInterface *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->stopFocus();
    }
}

void QGCCameraManager::_stepCamera(int direction)
{
    if (_lastCameraChange.elapsed() > 1000) {
        _lastCameraChange.start();
        qCDebug(CameraManagerLog) << "Step Camera" << direction;
        int camera = _currentCameraIndex + direction;
        if (camera < 0) {
            camera = _cameras.count() - 1;
        } else if (camera >= _cameras.count()) {
            camera = 0;
        }
        setCurrentCamera(camera);
    }
}

void QGCCameraManager::_stepStream(int direction)
{
    if (_lastCameraChange.elapsed() > 1000) {
        _lastCameraChange.start();
        MavlinkCameraControlInterface *pCamera = currentCameraInstance();
        if (pCamera) {
            qCDebug(CameraManagerLog) << "Step Camera Stream" << direction;
            int stream = pCamera->currentStream() + direction;
            if (stream < 0) {
                stream = pCamera->streams()->count() - 1;
            } else if (stream >= pCamera->streams()->count()) {
                stream = 0;
            }
            pCamera->setCurrentStream(stream);
        }
    }
}

const QVariantList &QGCCameraManager::cameraList() const
{
    if (_cameraList.isEmpty()) {
        const QList<CameraMetaData*> cams = CameraMetaData::parseCameraMetaData();
        _cameraList.reserve(cams.size());

        for (CameraMetaData *cam : cams) {
            _cameraList << QVariant::fromValue(cam);
        }
    }
    return _cameraList;
}

void QGCCameraManager::requestCameraFovForComp(int compId) {
    if (!_vehicle) {
        qCWarning(CameraManagerLog) << "requestCameraFovForComp: vehicle is null";
        return;
    }
    _vehicle->requestMessage(_requestFovOnZoom_Handler, /*user*/this,
                             compId, MAVLINK_MSG_ID_CAMERA_FOV_STATUS);
}

//-----------------------------------------------------------------------------
double QGCCameraManager::aspectForComp(int compId) const {
    auto it = _aspectByCompId.constFind(compId);
    return (it == _aspectByCompId.cend())
           ? std::numeric_limits<double>::quiet_NaN()
           : it.value();
}

double QGCCameraManager::currentCameraAspect(){
    if (auto* cam = currentCameraInstance()) {
        return aspectForComp(cam->compID());
    }
    return std::numeric_limits<double>::quiet_NaN();
}
void QGCCameraManager::_handleCameraFovStatus(const mavlink_message_t& message)
{
    mavlink_camera_fov_status_t fov{};
    mavlink_msg_camera_fov_status_decode(&message, &fov);

    if (!std::isfinite(fov.hfov) || fov.hfov <= 0.0 || fov.hfov >= 180.0) {
        return;
    }

    double aspect = aspectForComp(message.compid);
    if (!std::isfinite(aspect) || aspect <= 0.0) {
        aspect = 16.0 / 9.0;
    }

    const double hfovRad = fov.hfov * kPi / 180.0;
    const double vfovRad = 2.0 * std::atan(std::tan(hfovRad * 0.5) * aspect);
    const double vfovDeg = vfovRad * 180.0 / kPi;

    if (!std::isfinite(vfovDeg) || vfovDeg <= 0.0 || vfovDeg >= 180.0) {
        qCWarning(CameraManagerLog) << "Invalid calculated VFOV:" << vfovDeg
                                    << "hfov:" << fov.hfov
                                    << "aspect:" << aspect
                                    << "compId:" << message.compid;
        return;
    }

    auto* settings = SettingsManager::instance()->gimbalControllerSettings();
    settings->cameraHFov()->setRawValue(fov.hfov);
    settings->cameraVFov()->setRawValue(vfovDeg);
}

void QGCCameraManager::_setCurrentZoomLevel(int level)
{
    if (_zoomValueCurrent == level) {
        return;
    }
    _zoomValueCurrent = level;
    emit currentZoomLevelChanged();
}

int QGCCameraManager::currentZoomLevel() const
{
    return _zoomValueCurrent;
}
