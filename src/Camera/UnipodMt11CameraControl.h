#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>

#include "MavlinkCameraControlInterface.h"

class QGCVideoStreamInfo;
class UnipodMt11Client;
class UnipodMt11MediaClient;
class Vehicle;

/// \brief Camera control adapter for UniPod MT11 onboard photo/record via UDP SDK.
class UnipodMt11CameraControl : public MavlinkCameraControlInterface
{
    Q_OBJECT

public:
    explicit UnipodMt11CameraControl(Vehicle* vehicle, UnipodMt11Client* client, QObject* parent = nullptr);
    ~UnipodMt11CameraControl() override;

    void setMediaClient(UnipodMt11MediaClient* client);
    bool hasMediaLibrary() const override;

    void setCameraModeVideo() override;
    void setCameraModePhoto() override;
    void toggleCameraMode() override;
    bool takePhoto() override;
    bool startVideoRecording() override;
    bool stopVideoRecording() override;
    bool toggleVideoRecording() override;

    void resetSettings() override {}

    void formatCard(int id = 1) override { Q_UNUSED(id); }

    void stepZoom(int direction) override;

    void startZoom(int direction) override;

    void stopZoom() override;

    void stepFocus(int direction) override;

    void startFocus(int direction) override;

    void stopFocus() override;

    void stopStream() override {}

    bool stopTakePhoto() override { return false; }

    void resumeStream() override {}

    void startTrackingRect(QRectF /*rec*/) override {}

    void startTrackingPoint(QPointF /*point*/, double /*radius*/) override {}

    void stopTracking() override {}

    int version() const override { return 0; }

    QString modelName() const override { return QStringLiteral("UniPod MT11"); }

    QString vendor() const override { return QStringLiteral("Reebot"); }

    QString firmwareVersion() const override { return QString(); }

    qreal focalLength() const override { return qQNaN(); }

    QSizeF sensorSize() const override { return QSizeF(qQNaN(), qQNaN()); }

    QSize resolution() const override { return QSize(0, 0); }

    bool capturesVideo() const override;
    bool capturesPhotos() const override;

    bool hasModes() const override { return false; }

    bool hasZoom() const override;

    bool hasFocus() const override;

    bool hasTracking() const override { return false; }

    bool supportsTrackingPoint() const override { return false; }

    bool supportsTrackingRect() const override { return false; }

    bool hasVideoStream() const override;

    bool photosInVideoMode() const override { return true; }

    bool videoInPhotoMode() const override { return false; }

    CaptureVideoState captureVideoState() const override;
    CapturePhotosState capturePhotosState() const override;

    int compID() const override { return 0; }

    bool isBasic() const override { return true; }

    StorageStatus storageStatus() const override { return STORAGE_NOT_SUPPORTED; }

    QStringList activeSettings() const override { return QStringList(); }

    quint32 storageFree() const override { return 0; }

    QString storageFreeStr() const override { return QString(); }

    quint32 storageTotal() const override { return 0; }

    int batteryRemaining() const override { return -1; }

    QString batteryRemainingStr() const override { return QString(); }

    bool paramComplete() const override { return true; }

    qreal zoomLevel() const override { return 1.0; }

    qreal focusLevel() const override { return 1.0; }

    QmlObjectListModel* streams() override { return nullptr; }

    QGCVideoStreamInfo* currentStreamInstance() override { return nullptr; }

    QGCVideoStreamInfo* thermalStreamInstance() override { return nullptr; }

    int currentStream() const override { return 0; }

    void setCurrentStream(int /*stream*/) override {}

    bool autoStream() const override { return false; }

    quint32 recordTime() const override;
    QString recordTimeStr() const override;

    Fact* exposureMode() override { return nullptr; }

    Fact* ev() override { return nullptr; }

    Fact* iso() override { return nullptr; }

    Fact* shutterSpeed() override { return nullptr; }

    Fact* aperture() override { return nullptr; }

    Fact* wb() override { return nullptr; }

    Fact* mode() override { return nullptr; }

    QStringList streamLabels() const override { return QStringList(); }

    ThermalViewMode thermalMode() const override { return THERMAL_OFF; }

    void setThermalMode(ThermalViewMode /*mode*/) override {}

    double thermalOpacity() const override { return 0.0; }

    void setThermalOpacity(double /*val*/) override {}

    void setZoomLevel(qreal /*level*/) override {}

    void setFocusLevel(qreal /*level*/) override {}

    void setCameraMode(CameraMode cameraMode) override;
    void setPhotoCaptureMode(PhotoCaptureMode mode) override;

    void setPhotoLapse(qreal /*interval*/) override {}

    void setPhotoLapseCount(int /*count*/) override {}

    bool trackingEnabled() const override { return false; }

    void setTrackingEnabled(bool /*set*/) override {}

    bool trackingImageIsActive() const override { return false; }

    bool trackingImageIsPoint() const override { return false; }

    QRectF trackingImageRect() const override { return QRectF(); }

    QPointF trackingImagePoint() const override { return QPointF(); }

    qreal trackingImageRadius() const override { return 0.0; }

    void factChanged(Fact* /*pFact*/) override {}

    bool incomingParameter(Fact* /*pFact*/, QVariant& /*newValue*/) override { return false; }

    bool validateParameter(Fact* /*pFact*/, QVariant& /*newValue*/) override { return false; }

    void handleBatteryStatus(const mavlink_battery_status_t& /*bs*/) override {}

    void handleCameraCaptureStatus(const mavlink_camera_capture_status_t& /*cameraCaptureStatus*/) override {}

    void handleParamExtAck(const mavlink_param_ext_ack_t& /*paramExtAck*/) override {}

    void handleParamExtValue(const mavlink_param_ext_value_t& /*paramExtValue*/) override {}

    void handleCameraSettings(const mavlink_camera_settings_t& /*settings*/) override {}

    void handleStorageInformation(const mavlink_storage_information_t& /*storageInformation*/) override {}

    void handleTrackingImageStatus(const mavlink_camera_tracking_image_status_t& /*trackingImageStatus*/) override {}

    void handleVideoStreamInformation(const mavlink_video_stream_information_t& /*videoStreamInformation*/) override {}

    void handleVideoStreamStatus(const mavlink_video_stream_status_t& /*videoStreamStatus*/) override {}

    bool hasGimbalPad() const override;
    bool hasLensSwitch() const override;
    bool hasLaserRange() const override;
    bool hasAiRecognition() const override;
    bool hasFollowFlight() const override;
    bool hasExposureAuto() const override;

    Q_PROPERTY(bool laserEnabled READ laserEnabled NOTIFY laserEnabledChanged)
    Q_PROPERTY(bool aiRecognitionEnabled READ aiRecognitionEnabled NOTIFY aiRecognitionEnabledChanged)
    Q_PROPERTY(double laserDistanceMeters READ laserDistanceMeters NOTIFY laserDistanceChanged)

    bool laserEnabled() const;
    bool aiRecognitionEnabled() const;
    double laserDistanceMeters() const;

    Q_INVOKABLE void ptzStart(int direction);
    Q_INVOKABLE void ptzStop();
    Q_INVOKABLE void ptzHome();
    Q_INVOKABLE void setVideoLayout(int mainMode, int secondaryMode);
    Q_INVOKABLE void setLaserEnabled(bool enabled);
    Q_INVOKABLE void requestLaserDistance();
    Q_INVOKABLE void setAiRecognitionEnabled(bool enabled);

signals:
    void laserEnabledChanged();
    void aiRecognitionEnabledChanged();
    void laserDistanceChanged();

protected slots:

    void _paramDone() override {}

private:
    void _onRecordStaChanged(quint8 recordSta);
    void _onFuncFeedback(quint8 infoType);
    bool _isRecording() const;
    bool _isSelectedVideoSource() const;
    void _onSendFailed(const QString& reason);

    UnipodMt11Client* _client = nullptr;
    bool _noStorageCardNotified = false;
    UnipodMt11MediaClient* _mediaClient = nullptr;
    QElapsedTimer _videoRecordTimeElapsedTimer;
};
