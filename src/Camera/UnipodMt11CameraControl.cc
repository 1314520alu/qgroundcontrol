#include "UnipodMt11CameraControl.h"

#include <QtCore/QTime>
#include <QtCore/QtNumeric>

#include "AppMessages.h"
#include "PayloadCapabilityCatalog.h"
#include "QGCLoggingCategory.h"
#include "UnipodMt11Client.h"
#include "UnipodMt11MediaClient.h"
#include "UnipodMt11Protocol.h"
#include "Vehicle.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(UnipodMt11CameraControlLog, "Camera.UnipodMt11CameraControl")

namespace {

const PayloadCapabilityCatalog::Entry* unipodCatalogEntry()
{
    return PayloadCapabilityCatalog::instance().byModelName(QStringLiteral("UniPod MT11"));
}

bool unipodOverlayHas(const char* feature)
{
    return PayloadCapabilityCatalog::overlayHas(unipodCatalogEntry(), QString::fromLatin1(feature));
}

}  // namespace

UnipodMt11CameraControl::UnipodMt11CameraControl(Vehicle* vehicle, UnipodMt11Client* client, QObject* parent)
    : MavlinkCameraControlInterface(vehicle, parent), _client(client)
{
    qCDebug(UnipodMt11CameraControlLog) << this;

    if (!_client) {
        qCWarning(UnipodMt11CameraControlLog) << "UnipodMt11Client is required";
        return;
    }

    auto videoManager = VideoManager::instance();
    (void) connect(videoManager, &VideoManager::hasVideoChanged, this, &UnipodMt11CameraControl::infoChanged);
    (void) connect(videoManager, &VideoManager::decodingChanged, this, &UnipodMt11CameraControl::infoChanged);

    (void) connect(_client, &UnipodMt11Client::readyChanged, this, [this]() {
        emit infoChanged();
        emit captureVideoStateChanged();
        emit capturePhotosStateChanged();
    });
    (void) connect(_client, &UnipodMt11Client::recordStaChanged, this, &UnipodMt11CameraControl::_onRecordStaChanged);
    (void) connect(_client, &UnipodMt11Client::funcFeedback, this, &UnipodMt11CameraControl::_onFuncFeedback);
    (void) connect(_client, &UnipodMt11Client::sendFailed, this,
                   [](const QString& reason) { QGC::showAppMessage(reason); });
    (void) connect(_client, &UnipodMt11Client::laserEnabledChanged, this,
                   &UnipodMt11CameraControl::laserEnabledChanged);
    (void) connect(_client, &UnipodMt11Client::aiRecognitionEnabledChanged, this,
                   &UnipodMt11CameraControl::aiRecognitionEnabledChanged);
    (void) connect(_client, &UnipodMt11Client::laserDistanceChanged, this,
                   &UnipodMt11CameraControl::laserDistanceChanged);
    (void) connect(this, &UnipodMt11CameraControl::photoCaptureStatusChanged, this,
                   &UnipodMt11CameraControl::captureVideoStateChanged);
    (void) connect(this, &UnipodMt11CameraControl::photoCaptureStatusChanged, this,
                   &UnipodMt11CameraControl::capturePhotosStateChanged);

    _videoRecordTimeUpdateTimer.setInterval(1000);
    (void) connect(&_videoRecordTimeUpdateTimer, &QTimer::timeout, this, &UnipodMt11CameraControl::recordTimeChanged);

    if (capturesVideo()) {
        _cameraMode = CAM_MODE_VIDEO;
    } else if (capturesPhotos()) {
        _cameraMode = CAM_MODE_PHOTO;
    } else {
        _cameraMode = CAM_MODE_UNDEFINED;
    }
}

UnipodMt11CameraControl::~UnipodMt11CameraControl()
{
    qCDebug(UnipodMt11CameraControlLog) << this;
}

bool UnipodMt11CameraControl::_isRecording() const
{
    if (!_client) {
        return false;
    }

    const quint8 recordSta = _client->recordSta();
    return (recordSta == 1) || (recordSta == 3);
}

void UnipodMt11CameraControl::_onRecordStaChanged(quint8 recordSta)
{
    const bool capturing = (recordSta == 1) || (recordSta == 3);
    if (capturing) {
        if (!_videoRecordTimeUpdateTimer.isActive()) {
            _videoRecordTimeElapsedTimer.start();
            _videoRecordTimeUpdateTimer.start();
        }
    } else {
        _videoRecordTimeUpdateTimer.stop();
    }

    if (recordSta == 2) {
        QGC::showAppMessage(tr("UniPod MT11: no storage card"));
    }

    emit captureVideoStateChanged();
    emit recordTimeChanged();
}

void UnipodMt11CameraControl::_onFuncFeedback(quint8 infoType)
{
    using UnipodMt11Protocol::FuncFeedback;

    switch (static_cast<FuncFeedback>(infoType)) {
        case FuncFeedback::PhotoFailNoCard:
            QGC::showAppMessage(tr("UniPod MT11: photo failed — no storage card"));
            break;
        case FuncFeedback::PhotoFail:
            QGC::showAppMessage(tr("UniPod MT11: photo failed"));
            break;
        case FuncFeedback::RecordStart:
        case FuncFeedback::RecordEnd:
            emit captureVideoStateChanged();
            emit recordTimeChanged();
            break;
        default:
            break;
    }
}

QString UnipodMt11CameraControl::recordTimeStr() const
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString(QStringLiteral("hh:mm:ss"));
}

void UnipodMt11CameraControl::setCameraMode(CameraMode cameraMode)
{
    qCDebug(UnipodMt11CameraControlLog) << cameraModeToStr(cameraMode);

    if (!hasModes()) {
        qCWarning(UnipodMt11CameraControlLog) << "Set camera mode denied - camera does not support modes";
        return;
    }

    switch (cameraMode) {
        case CAM_MODE_VIDEO:
            setCameraModeVideo();
            break;
        case CAM_MODE_PHOTO:
            setCameraModePhoto();
            break;
        default:
            qCWarning(UnipodMt11CameraControlLog) << "Invalid mode" << cameraMode;
            break;
    }
}

void UnipodMt11CameraControl::toggleCameraMode()
{
    if (!hasModes()) {
        qCWarning(UnipodMt11CameraControlLog) << "Toggle camera mode denied - camera does not support modes";
    }
}

void UnipodMt11CameraControl::setCameraModeVideo()
{
    if (!hasModes()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera does not support modes";
    }
}

void UnipodMt11CameraControl::setCameraModePhoto()
{
    if (!hasModes()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera does not support modes";
    }
}

bool UnipodMt11CameraControl::takePhoto()
{
    if (!_client || !capturesPhotos()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera does not handle image capture";
        return false;
    }

    if (_photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera not idle";
        return false;
    }

    _client->takePhoto();
    _photoCaptureStatusValue = PHOTO_CAPTURE_IN_PROGRESS;
    emit photoCaptureStatusChanged();
    QTimer::singleShot(500, this, [this]() {
        _photoCaptureStatusValue = PHOTO_CAPTURE_IDLE;
        emit photoCaptureStatusChanged();
    });
    return true;
}

bool UnipodMt11CameraControl::toggleVideoRecording()
{
    if (!_client || !capturesVideo()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera does not handle video capture";
        return false;
    }

    if (_client->recordSta() == 2) {
        qCWarning(UnipodMt11CameraControlLog) << "No storage card";
        return false;
    }

    _client->toggleRecording();
    return true;
}

bool UnipodMt11CameraControl::startVideoRecording()
{
    if (!_client || !capturesVideo()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera does not handle video capture";
        return false;
    }

    if (_client->recordSta() == 2) {
        qCWarning(UnipodMt11CameraControlLog) << "No storage card";
        return false;
    }

    if (_isRecording()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera already recording";
        return false;
    }

    _client->toggleRecording();
    return true;
}

bool UnipodMt11CameraControl::stopVideoRecording()
{
    if (!_client || !capturesVideo()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera does not handle video capture";
        return false;
    }

    if (!_isRecording()) {
        qCWarning(UnipodMt11CameraControlLog) << "Camera not recording";
        return false;
    }

    _client->toggleRecording();
    return true;
}

quint32 UnipodMt11CameraControl::recordTime() const
{
    return (_videoRecordTimeUpdateTimer.isActive() ? static_cast<quint32>(_videoRecordTimeElapsedTimer.elapsed()) : 0);
}

bool UnipodMt11CameraControl::capturesVideo() const
{
    return unipodOverlayHas("video");
}

bool UnipodMt11CameraControl::capturesPhotos() const
{
    return unipodOverlayHas("photo");
}

bool UnipodMt11CameraControl::hasZoom() const
{
    return unipodOverlayHas("zoom");
}

bool UnipodMt11CameraControl::hasFocus() const
{
    return unipodOverlayHas("focus");
}

bool UnipodMt11CameraControl::hasGimbalPad() const
{
    return unipodOverlayHas("gimbal");
}

bool UnipodMt11CameraControl::hasLensSwitch() const
{
    return unipodOverlayHas("lens");
}

bool UnipodMt11CameraControl::hasLaserRange() const
{
    return unipodOverlayHas("laser");
}

bool UnipodMt11CameraControl::hasAiRecognition() const
{
    return unipodOverlayHas("ai");
}

bool UnipodMt11CameraControl::hasFollowFlight() const
{
    return unipodOverlayHas("follow");
}

bool UnipodMt11CameraControl::hasExposureAuto() const
{
    return unipodOverlayHas("exposure_auto");
}

bool UnipodMt11CameraControl::hasVideoStream() const
{
    return VideoManager::instance()->decoding();
}

bool UnipodMt11CameraControl::laserEnabled() const
{
    return _client && _client->laserEnabled();
}

bool UnipodMt11CameraControl::aiRecognitionEnabled() const
{
    return _client && _client->aiRecognitionEnabled();
}

double UnipodMt11CameraControl::laserDistanceMeters() const
{
    return _client ? _client->laserDistanceMeters() : qQNaN();
}

void UnipodMt11CameraControl::stepZoom(int direction)
{
    startZoom(direction);
    QTimer::singleShot(200, this, [this]() { stopZoom(); });
}

void UnipodMt11CameraControl::startZoom(int direction)
{
    if (_client && hasZoom()) {
        _client->startZoom(direction);
    }
}

void UnipodMt11CameraControl::stopZoom()
{
    if (_client) {
        _client->stopZoom();
    }
}

void UnipodMt11CameraControl::stepFocus(int direction)
{
    startFocus(direction);
    QTimer::singleShot(200, this, [this]() { stopFocus(); });
}

void UnipodMt11CameraControl::startFocus(int direction)
{
    if (_client && hasFocus()) {
        _client->startFocus(direction);
    }
}

void UnipodMt11CameraControl::stopFocus()
{
    if (_client) {
        _client->stopFocus();
    }
}

void UnipodMt11CameraControl::ptzStart(int direction)
{
    if (_client && hasGimbalPad()) {
        _client->ptzStart(direction);
    }
}

void UnipodMt11CameraControl::ptzStop()
{
    if (_client) {
        _client->ptzStop();
    }
}

void UnipodMt11CameraControl::ptzHome()
{
    if (_client && hasGimbalPad()) {
        _client->ptzHome();
    }
}

void UnipodMt11CameraControl::setVideoLayout(int mainMode, int secondaryMode)
{
    if (_client && hasLensSwitch()) {
        _client->setVideoLayout(static_cast<quint8>(mainMode), static_cast<quint8>(secondaryMode));
    }
}

void UnipodMt11CameraControl::setLaserEnabled(bool enabled)
{
    if (_client && hasLaserRange()) {
        _client->setLaserEnabled(enabled);
    }
}

void UnipodMt11CameraControl::requestLaserDistance()
{
    if (_client && hasLaserRange()) {
        _client->requestLaserDistance();
    }
}

void UnipodMt11CameraControl::setAiRecognitionEnabled(bool enabled)
{
    if (_client && hasAiRecognition()) {
        _client->setAiRecognitionEnabled(enabled);
    }
}

MavlinkCameraControlInterface::CaptureVideoState UnipodMt11CameraControl::captureVideoState() const
{
    if (!_client || !_client->isReady()) {
        return CaptureVideoStateDisabled;
    }

    const quint8 recordSta = _client->recordSta();
    if (recordSta == 2) {
        return CaptureVideoStateDisabled;
    }

    if (recordSta == 1 || recordSta == 3) {
        return CaptureVideoStateCapturing;
    }

    if (_photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        return CaptureVideoStateDisabled;
    }

    return CaptureVideoStateIdle;
}

MavlinkCameraControlInterface::CapturePhotosState UnipodMt11CameraControl::capturePhotosState() const
{
    if (!_client || !_client->isReady()) {
        return CapturePhotosStateDisabled;
    }

    if (_photoCaptureStatus() == PHOTO_CAPTURE_IN_PROGRESS) {
        return CapturePhotosStateCapturingSinglePhoto;
    }

    return CapturePhotosStateIdle;
}

void UnipodMt11CameraControl::setPhotoCaptureMode(MavlinkCameraControlInterface::PhotoCaptureMode photoCaptureMode)
{
    if (photoCaptureMode == PHOTO_CAPTURE_TIMELAPSE) {
        qCWarning(UnipodMt11CameraControlLog) << "Time lapse capture not supported by UniPod MT11";
        return;
    }

    if (_photoCaptureMode != photoCaptureMode) {
        _photoCaptureMode = photoCaptureMode;
        emit photoCaptureModeChanged();
    }
}

void UnipodMt11CameraControl::setMediaClient(UnipodMt11MediaClient* client)
{
    if (_mediaClient == client) {
        return;
    }
    if (_mediaClient) {
        disconnect(_mediaClient, nullptr, this, nullptr);
    }
    _mediaClient = client;
    if (_mediaClient) {
        connect(_mediaClient, &UnipodMt11MediaClient::readyChanged, this, &UnipodMt11CameraControl::infoChanged);
    }
    emit infoChanged();
}

bool UnipodMt11CameraControl::hasMediaLibrary() const
{
    return unipodOverlayHas("media_library") && _mediaClient && _mediaClient->isReady();
}
