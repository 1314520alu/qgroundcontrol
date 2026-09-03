# ============================================================================
# Satcom Module
# High-altitude satellite antenna TCP control (Weitong aircraft)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE SatcomAntennaController.cc SatcomAntennaController.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
