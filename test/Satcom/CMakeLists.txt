# ============================================================================
# Satcom Unit Tests
# High-altitude satellite antenna protocol parsing and commands
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE SatcomAntennaControllerTest.cc SatcomAntennaControllerTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(SatcomAntennaControllerTest LABELS Unit)
