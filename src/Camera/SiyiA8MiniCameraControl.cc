#include "SiyiA8MiniCameraControl.h"

#include <QtCore/QTime>

#include "AppMessages.h"
#include "PayloadCapabilityCatalog.h"
#include "QGCLoggingCategory.h"
#include "UnipodMt11Client.h"
#include "UnipodMt11Protocol.h"
#include "Vehicle.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(SiyiA8MiniCameraControlLog, "Camera.SiyiA8MiniCameraControl")

namespace {

bool a8OverlayHas(const char* feature)
{
    return PayloadCapabilityCatalog::overlayHas(
        PayloadCapabilityCatalog::instance().byModelName(QStringLiteral("SIYI A8 Mini")), QString::fromLatin1(feature));
}

}  // namespace

SiyiA8MiniCameraControl::SiyiA8MiniCameraControl(Vehicle* vehicle, UnipodMt11Client* client, QObject* parent)
    : MavlinkCameraControlInterface(vehicle, parent), _client(client)
{
    qCDebug(SiyiA8MiniCameraControlLog) << this;

    if (!_client) {
        qCWarning(SiyiA8MiniCameraControlLog) << "UnipodMt11Client is required";
        return;
    }

    auto* videoManager = VideoManager::instance();
    (void) connect(videoManager, &VideoManager::hasVideoChanged, this, &SiyiA8MiniCameraControl::infoChanged);
    (void) connect(videoManager, &VideoManager::decodingChanged, this, &SiyiA8MiniCameraControl::infoChanged);

    (void) connect(_client, &UnipodMt11Client::readyChanged, this, [this]() {
        emit infoChanged();
        emit captureVideoStateChanged();
        emit capturePhotosStateChanged();
    });
    (void) connect(_client, &UnipodMt11Client::recordStaChanged, this, &SiyiA8MiniCameraControl::_onRecordStaChanged);
    (void) connect(_client, &UnipodMt11Client::funcFeedback, this, &SiyiA8MiniCameraControl::_onFuncFeedback);
    (void) connect(_client, &UnipodMt11Client::sendFailed, this,
                   [](const QString& reason) { QGC::showAppMessage(reason); });
    (void) connect(this, &SiyiA8MiniCameraControl::photoCaptureStatusChanged, this,
                   &SiyiA8MiniCameraControl::captureVideoStateChanged);
    (void) connect(this, &SiyiA8MiniCameraControl::photoCaptureStatusChanged, this,
                   &SiyiA8MiniCameraControl::capturePhotosStateChanged);

    _videoRecordTimeUpdateTimer.setInterval(1000);
    (void) connect(&_videoRecordTimeUpdateTimer, &QTimer::timeout, this, &SiyiA8MiniCameraControl::recordTimeChanged);

    if (capturesVideo()) {
        _cameraMode = CAM_MODE_VIDEO;
    } else if (capturesPhotos()) {
        _cameraMode = CAM_MODE_PHOTO;
    } else {
        _cameraMode = CAM_MODE_UNDEFINED;
    }
}

SiyiA8MiniCameraControl::~SiyiA8MiniCameraControl()
{
    qCDebug(SiyiA8MiniCameraControlLog) << this;
}

bool SiyiA8MiniCameraControl::_isRecording() const
{
    if (!_client) {
        return false;
    }
    const quint8 recordSta = _client->recordSta();
    return (recordSta == 1) || (recordSta == 3);
}

void SiyiA8MiniCameraControl::_onRecordStaChanged(quint8 recordSta)
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
        QGC::showAppMessage(tr("SIYI A8 Mini: no storage card"));
    }

    emit captureVideoStateChanged();
    emit recordTimeChanged();
}

void SiyiA8MiniCameraControl::_onFuncFeedback(quint8 infoType)
{
    using UnipodMt11Protocol::FuncFeedback;

    switch (static_cast<FuncFeedback>(infoType)) {
        case FuncFeedback::PhotoFailNoCard:
            QGC::showAppMessage(tr("SIYI A8 Mini: photo failed — no storage card"));
            break;
        case FuncFeedback::PhotoFail:
            QGC::showAppMessage(tr("SIYI A8 Mini: photo failed"));
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

QString SiyiA8MiniCameraControl::recordTimeStr() const
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString(QStringLiteral("hh:mm:ss"));
}

void SiyiA8MiniCameraControl::setCameraMode(CameraMode cameraMode)
{
    Q_UNUSED(cameraMode);
}

void SiyiA8MiniCameraControl::toggleCameraMode() {}

void SiyiA8MiniCameraControl::setCameraModeVideo() {}

void SiyiA8MiniCameraControl::setCameraModePhoto() {}

bool SiyiA8MiniCameraControl::takePhoto()
{
    if (!_client || !capturesPhotos()) {
        return false;
    }
    if (_photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
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

bool SiyiA8MiniCameraControl::toggleVideoRecording()
{
    if (!_client || !capturesVideo()) {
        return false;
    }
    _client->toggleRecording();
    return true;
}

bool SiyiA8MiniCameraControl::startVideoRecording()
{
    if (!_client || !capturesVideo() || _isRecording()) {
        return false;
    }
    _client->toggleRecording();
    return true;
}

bool SiyiA8MiniCameraControl::stopVideoRecording()
{
    if (!_client || !capturesVideo() || !_isRecording()) {
        return false;
    }
    _client->toggleRecording();
    return true;
}

quint32 SiyiA8MiniCameraControl::recordTime() const
{
    return (_videoRecordTimeUpdateTimer.isActive() ? static_cast<quint32>(_videoRecordTimeElapsedTimer.elapsed()) : 0);
}

bool SiyiA8MiniCameraControl::capturesVideo() const
{
    return a8OverlayHas("video");
}

bool SiyiA8MiniCameraControl::capturesPhotos() const
{
    return a8OverlayHas("photo");
}

bool SiyiA8MiniCameraControl::hasZoom() const
{
    return a8OverlayHas("zoom");
}

bool SiyiA8MiniCameraControl::hasGimbalPad() const
{
    return a8OverlayHas("gimbal");
}

bool SiyiA8MiniCameraControl::hasExposureAuto() const
{
    return a8OverlayHas("exposure_auto");
}

bool SiyiA8MiniCameraControl::hasVideoStream() const
{
    return VideoManager::instance()->decoding();
}

void SiyiA8MiniCameraControl::stepZoom(int direction)
{
    startZoom(direction);
    QTimer::singleShot(200, this, [this]() { stopZoom(); });
}

void SiyiA8MiniCameraControl::startZoom(int direction)
{
    if (_client && hasZoom()) {
        _client->startZoom(direction);
    }
}

void SiyiA8MiniCameraControl::stopZoom()
{
    if (_client) {
        _client->stopZoom();
    }
}

void SiyiA8MiniCameraControl::ptzStart(int direction)
{
    if (_client && hasGimbalPad()) {
        _client->ptzStart(direction);
    }
}

void SiyiA8MiniCameraControl::ptzStop()
{
    if (_client) {
        _client->ptzStop();
    }
}

void SiyiA8MiniCameraControl::ptzHome()
{
    if (_client && hasGimbalPad()) {
        _client->ptzHome();
    }
}

MavlinkCameraControlInterface::CaptureVideoState SiyiA8MiniCameraControl::captureVideoState() const
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

MavlinkCameraControlInterface::CapturePhotosState SiyiA8MiniCameraControl::capturePhotosState() const
{
    if (!_client || !_client->isReady()) {
        return CapturePhotosStateDisabled;
    }
    if (_photoCaptureStatus() == PHOTO_CAPTURE_IN_PROGRESS) {
        return CapturePhotosStateCapturingSinglePhoto;
    }
    return CapturePhotosStateIdle;
}

void SiyiA8MiniCameraControl::setPhotoCaptureMode(MavlinkCameraControlInterface::PhotoCaptureMode photoCaptureMode)
{
    if (photoCaptureMode == PHOTO_CAPTURE_TIMELAPSE) {
        return;
    }
    if (_photoCaptureMode != photoCaptureMode) {
        _photoCaptureMode = photoCaptureMode;
        emit photoCaptureModeChanged();
    }
}
