#include <QtCore/QTime>

#include "TopotekTq10CameraControl.h"

#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "TopotekTq10Client.h"
#include "Vehicle.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(TopotekTq10CameraControlLog, "Camera.TopotekTq10CameraControl")

TopotekTq10CameraControl::TopotekTq10CameraControl(Vehicle *vehicle, TopotekTq10Client *client, QObject *parent)
    : MavlinkCameraControlInterface(vehicle, parent)
    , _client(client)
{
    qCDebug(TopotekTq10CameraControlLog) << this;

    if (!_client) {
        qCWarning(TopotekTq10CameraControlLog) << "TopotekTq10Client is required";
        return;
    }

    auto videoManager = VideoManager::instance();
    (void) connect(videoManager, &VideoManager::hasVideoChanged, this, &TopotekTq10CameraControl::infoChanged);
    (void) connect(videoManager, &VideoManager::decodingChanged, this, &TopotekTq10CameraControl::infoChanged);

    (void) connect(_client, &TopotekTq10Client::readyChanged, this, [this]() {
        emit infoChanged();
        emit captureVideoStateChanged();
        emit capturePhotosStateChanged();
    });
    (void) connect(_client, &TopotekTq10Client::recordStaChanged, this, &TopotekTq10CameraControl::_onRecordStaChanged);
    (void) connect(_client, &TopotekTq10Client::sendFailed, this, [](const QString &reason) {
        QGC::showAppMessage(reason);
    });
    (void) connect(this, &TopotekTq10CameraControl::photoCaptureStatusChanged, this, &TopotekTq10CameraControl::captureVideoStateChanged);
    (void) connect(this, &TopotekTq10CameraControl::photoCaptureStatusChanged, this, &TopotekTq10CameraControl::capturePhotosStateChanged);

    _videoRecordTimeUpdateTimer.setInterval(1000);
    (void) connect(&_videoRecordTimeUpdateTimer, &QTimer::timeout, this, &TopotekTq10CameraControl::recordTimeChanged);

    if (capturesVideo()) {
        _cameraMode = CAM_MODE_VIDEO;
    } else if (capturesPhotos()) {
        _cameraMode = CAM_MODE_PHOTO;
    } else {
        _cameraMode = CAM_MODE_UNDEFINED;
    }
}

TopotekTq10CameraControl::~TopotekTq10CameraControl()
{
    qCDebug(TopotekTq10CameraControlLog) << this;
}

bool TopotekTq10CameraControl::_isRecording() const
{
    return _client && (_client->recordSta() == 1);
}

void TopotekTq10CameraControl::_onRecordStaChanged(quint8 recordSta)
{
    const bool capturing = (recordSta == 1);
    if (capturing) {
        if (!_videoRecordTimeUpdateTimer.isActive()) {
            _videoRecordTimeElapsedTimer.start();
            _videoRecordTimeUpdateTimer.start();
        }
    } else {
        _videoRecordTimeUpdateTimer.stop();
    }

    emit captureVideoStateChanged();
    emit recordTimeChanged();
}

QString TopotekTq10CameraControl::recordTimeStr() const
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString(QStringLiteral("hh:mm:ss"));
}

void TopotekTq10CameraControl::setCameraMode(CameraMode cameraMode)
{
    qCDebug(TopotekTq10CameraControlLog) << cameraModeToStr(cameraMode);
}

void TopotekTq10CameraControl::toggleCameraMode()
{
}

void TopotekTq10CameraControl::setCameraModeVideo()
{
}

void TopotekTq10CameraControl::setCameraModePhoto()
{
}

bool TopotekTq10CameraControl::takePhoto()
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

bool TopotekTq10CameraControl::toggleVideoRecording()
{
    if (!_client || !capturesVideo()) {
        return false;
    }

    _client->toggleRecording();
    return true;
}

bool TopotekTq10CameraControl::startVideoRecording()
{
    if (!_client || !capturesVideo() || _isRecording()) {
        return false;
    }

    _client->toggleRecording();
    return true;
}

bool TopotekTq10CameraControl::stopVideoRecording()
{
    if (!_client || !capturesVideo() || !_isRecording()) {
        return false;
    }

    _client->toggleRecording();
    return true;
}

void TopotekTq10CameraControl::stepZoom(int direction)
{
    startZoom(direction);
    QTimer::singleShot(200, this, [this]() { stopZoom(); });
}

void TopotekTq10CameraControl::startZoom(int direction)
{
    if (_client) {
        _client->startZoom(direction);
    }
}

void TopotekTq10CameraControl::stopZoom()
{
    if (_client) {
        _client->stopZoom();
    }
}

void TopotekTq10CameraControl::ptzStart(int direction)
{
    if (_client) {
        _client->ptzStart(direction);
    }
}

void TopotekTq10CameraControl::ptzStop()
{
    if (_client) {
        _client->ptzStop();
    }
}

void TopotekTq10CameraControl::ptzHome()
{
    if (_client) {
        _client->ptzHome();
    }
}

quint32 TopotekTq10CameraControl::recordTime() const
{
    return (_videoRecordTimeUpdateTimer.isActive() ? static_cast<quint32>(_videoRecordTimeElapsedTimer.elapsed()) : 0);
}

bool TopotekTq10CameraControl::capturesVideo() const
{
    return true;
}

bool TopotekTq10CameraControl::capturesPhotos() const
{
    return true;
}

bool TopotekTq10CameraControl::hasVideoStream() const
{
    return VideoManager::instance()->decoding();
}

MavlinkCameraControlInterface::CaptureVideoState TopotekTq10CameraControl::captureVideoState() const
{
    if (!_client || !_client->isReady()) {
        return CaptureVideoStateDisabled;
    }

    if (_client->recordSta() == 1) {
        return CaptureVideoStateCapturing;
    }

    if (_photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        return CaptureVideoStateDisabled;
    }

    return CaptureVideoStateIdle;
}

MavlinkCameraControlInterface::CapturePhotosState TopotekTq10CameraControl::capturePhotosState() const
{
    if (!_client || !_client->isReady()) {
        return CapturePhotosStateDisabled;
    }

    if (_photoCaptureStatus() == PHOTO_CAPTURE_IN_PROGRESS) {
        return CapturePhotosStateCapturingSinglePhoto;
    }

    return CapturePhotosStateIdle;
}

void TopotekTq10CameraControl::setPhotoCaptureMode(MavlinkCameraControlInterface::PhotoCaptureMode photoCaptureMode)
{
    if (photoCaptureMode == PHOTO_CAPTURE_TIMELAPSE) {
        qCWarning(TopotekTq10CameraControlLog) << "Time lapse capture not supported by Topotek TQ10N";
        return;
    }

    if (_photoCaptureMode != photoCaptureMode) {
        _photoCaptureMode = photoCaptureMode;
        emit photoCaptureModeChanged();
    }
}
