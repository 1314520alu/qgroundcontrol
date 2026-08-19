# ============================================================================
# Camera Module
# MAVLink camera control and video stream management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CameraMetaData.cc
        CameraMetaData.h
        MavlinkCameraControlInterface.cc
        MavlinkCameraControlInterface.h
        QGCCameraIO.cc
        QGCCameraIO.h
        QGCCameraManager.cc
        QGCCameraManager.h
        QGCVideoStreamInfo.cc
        QGCVideoStreamInfo.h
        SimulatedCameraControl.cc
        SimulatedCameraControl.h
        UnipodMt11CameraControl.cc
        UnipodMt11CameraControl.h
        UnipodMt11Client.cc
        UnipodMt11Client.h
        UnipodMt11MediaClient.cc
        UnipodMt11MediaClient.h
        UnipodMt11Protocol.cc
        UnipodMt11Protocol.h
        TopotekTq10CameraControl.cc
        TopotekTq10CameraControl.h
        TopotekTq10Client.cc
        TopotekTq10Client.h
        TopotekTq10Protocol.cc
        TopotekTq10Protocol.h
        VehicleCameraControl.cc
        VehicleCameraControl.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Camera Metadata Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} json_mavlink_camera
    PREFIX "/json"
    FILES CameraMetaData.json
)

qt_add_resources(${CMAKE_PROJECT_NAME} camera_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/images"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/images/camera_photo.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/images/camera_video.svg"
)
