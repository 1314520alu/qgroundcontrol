# ============================================================================
# FirmwarePlugin Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FlightModeNameTranslatorTest.cc
        FlightModeNameTranslatorTest.h
        ParameterEnumStringTranslatorTest.cc
        ParameterEnumStringTranslatorTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(FlightModeNameTranslatorTest LABELS Unit FirmwarePlugin)
add_qgc_test(ParameterEnumStringTranslatorTest LABELS Unit FirmwarePlugin)
