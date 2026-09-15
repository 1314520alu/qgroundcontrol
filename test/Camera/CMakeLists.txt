# ============================================================================
# Camera Unit Tests
# Tests for camera management and control
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE QGCCameraManagerTest.cc
            QGCCameraManagerTest.h
            VehicleCameraControlTest.cc
            VehicleCameraControlTest.h
            QGCVideoStreamInfoTest.cc
            QGCVideoStreamInfoTest.h
            VideoManagerTest.cc
            VideoManagerTest.h
            UnipodMt11ProtocolTest.cc
            UnipodMt11ProtocolTest.h
            UnipodMt11MediaClientTest.cc
            UnipodMt11MediaClientTest.h
            TopotekTq10ProtocolTest.cc
            TopotekTq10ProtocolTest.h
            CameraPayloadCapabilitiesTest.cc
            CameraPayloadCapabilitiesTest.h
            CameraGimbalQuickActionsTest.cc
            CameraGimbalQuickActionsTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QGCCameraManagerTest LABELS Integration Vehicle)
add_qgc_test(VehicleCameraControlTest LABELS Integration Vehicle)
add_qgc_test(QGCVideoStreamInfoTest LABELS Unit Camera)
add_qgc_test(VideoManagerTest LABELS Unit)
add_qgc_test(UnipodMt11ProtocolTest LABELS Unit Camera)
add_qgc_test(UnipodMt11MediaClientTest LABELS Unit Camera)
add_qgc_test(TopotekTq10ProtocolTest LABELS Unit Camera)
add_qgc_test(CameraPayloadCapabilitiesTest LABELS Unit Camera)
add_qgc_test(CameraGimbalQuickActionsTest LABELS Unit Camera)
