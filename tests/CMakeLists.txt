cmake_minimum_required(VERSION 3.10)
project(LMUFFB_Tests)

set(CMAKE_CXX_STANDARD 17)


# Include main source dir for headers
include_directories(..)
include_directories(../src)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/src)

if(NOT WIN32)
    include_directories(linux_mock)
    include_directories(../src/lmu_sm_interface/linux_mock)
    include_directories(../src/lmu_sm_interface)
endif()

# Combined Test Executable
set(TEST_SOURCES 
    main_test_runner.cpp 
    test_ffb_accuracy_tools.cpp
    test_ffb_advanced_slope.cpp
    test_ffb_slope_edge_cases.cpp
    test_ffb_common.cpp
    test_ffb_core_physics.cpp
    test_ffb_slope_detection.cpp
    test_ffb_slope_fix.cpp
    test_ffb_understeer.cpp
    test_ffb_smoothstep.cpp
    test_ffb_smoothing.cpp
    test_ffb_yaw_gyro.cpp
    test_ffb_coordinates.cpp
    test_ffb_features.cpp
    test_ffb_road_texture.cpp
    test_ffb_lockup_braking.cpp
    test_ffb_config.cpp
    test_ffb_slip_grip.cpp
    test_ffb_internal.cpp
    test_ffb_coverage_refactor.cpp
    test_persistence_v0625.cpp
    test_persistence_v0628.cpp
    test_async_logger.cpp
    test_ffb_load_normalization.cpp
    test_versioned_presets.cpp
    test_preset_improvements.cpp
    test_ffb_stability.cpp
    test_gui_tooltips_presence.cpp
    test_ffb_logic.cpp
    test_ffb_safety.cpp
    test_ffb_soft_lock.cpp
    test_issue_100_timing.cpp
    test_issue_104_slope_disconnect.cpp
    test_ffb_dynamic_weight.cpp
    test_rate_monitor.cpp
    test_math_utils.cpp
    test_perf_stats.cpp
    test_vehicle_utils.cpp
    test_coverage_boost.cpp
    test_ffb_coverage_target.cpp
    test_ffb_diag_updates.cpp
    test_ffb_hardware_scaling.cpp
    test_ffb_issue_142.cpp
    test_ffb_tactile_normalization.cpp
    test_health_monitor.cpp
    test_ffb_persistent_load.cpp
)

# ImGui Core is now always available if vendor exists

if(IMGUI_CORE_SOURCES)
    list(APPEND TEST_SOURCES
        test_gui_interaction.cpp
    )
endif()

if(WIN32)
    list(APPEND TEST_SOURCES 
        test_windows_platform.cpp 
        test_security_metadata.cpp
        ${CMAKE_BINARY_DIR}/src/res.rc
    )
else()
    # On Linux, backend file is in LMUFFB_Core
    if(BUILD_HEADLESS)
        add_definitions(-DHEADLESS_GUI)
        # Enable some Windows-style tests on Linux using mocks
        list(APPEND TEST_SOURCES test_windows_platform.cpp)
    endif()
endif()

enable_testing()
add_executable(run_combined_tests ${TEST_SOURCES})
target_compile_definitions(run_combined_tests PRIVATE HEADLESS_GUI)
target_link_libraries(run_combined_tests PRIVATE LMUFFB_Core)

if(WIN32)
    target_link_libraries(run_combined_tests PRIVATE version imm32 dxgi)
endif()

# Add to CTest
add_test(NAME CombinedTests COMMAND run_combined_tests)

