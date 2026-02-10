cmake_minimum_required(VERSION 3.10)
project(LMUFFB_Tests)

set(CMAKE_CXX_STANDARD 17)


# Include main source dir for headers
include_directories(..)
include_directories(../src)
include_directories(${CMAKE_BINARY_DIR})

if(NOT WIN32)
    include_directories(linux_mock)
endif()

# Combined Test Executable
set(TEST_SOURCES 
    main_test_runner.cpp 
    test_ffb_common.cpp
    test_ffb_core_physics.cpp
    test_ffb_slope_detection.cpp
    test_ffb_understeer.cpp
    test_ffb_smoothstep.cpp
    test_ffb_yaw_gyro.cpp
    test_ffb_coordinates.cpp
    test_ffb_features.cpp
    test_ffb_road_texture.cpp
    test_ffb_lockup_braking.cpp
    test_ffb_config.cpp
    test_ffb_slip_grip.cpp
    test_ffb_internal.cpp
    test_persistence_v0625.cpp
    test_persistence_v0628.cpp
    test_async_logger.cpp
    test_versioned_presets.cpp
    test_preset_improvements.cpp
    test_ffb_stability.cpp

    # Core Application Logic (Now with Linux Mocks)
    ../src/Config.cpp
    ../src/DirectInputFFB.cpp
    ../src/GameConnector.cpp
    ../src/GuiLayer_Common.cpp
)

# ImGui Core is now always available if vendor exists

if(IMGUI_CORE_SOURCES)
    list(APPEND TEST_SOURCES
        test_gui_interaction.cpp
        ${IMGUI_CORE_SOURCES}
        ${IMGUI_BACKEND_SOURCES}
    )
endif()

if(WIN32)
    list(APPEND TEST_SOURCES 
        test_windows_platform.cpp 
        test_screenshot.cpp
        ../src/GuiLayer_Win32.cpp
        ../src/res.rc
    )
else()
    # On Linux, always include the Linux backend file,
    # but it will use stubs if HEADLESS_GUI is defined.
    list(APPEND TEST_SOURCES ../src/GuiLayer_Linux.cpp)
    if(BUILD_HEADLESS)
        add_definitions(-DHEADLESS_GUI)
    endif()
endif()

enable_testing()
add_executable(run_combined_tests ${TEST_SOURCES})

if(WIN32)
    target_link_libraries(run_combined_tests dinput8 dxguid version imm32 winmm d3d11 d3dcompiler dxgi)
elseif(NOT BUILD_HEADLESS)
    # Link GUI libraries if we are NOT in headless mode
    target_link_libraries(run_combined_tests glfw OpenGL::GL dl pthread)
endif()

# Add to CTest
add_test(NAME CombinedTests COMMAND run_combined_tests)
