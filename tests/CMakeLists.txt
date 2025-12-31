cmake_minimum_required(VERSION 3.10)
project(LMUFFB_Tests)

set(CMAKE_CXX_STANDARD 17)

# Include main source dir for headers
include_directories(..)
include_directories(../src)

# Combined Test Executable
set(TEST_SOURCES 
    main_test_runner.cpp 
    test_ffb_engine.cpp 
    test_persistence_v0625.cpp
    test_persistence_v0628.cpp
    ../src/Config.cpp
)

if(WIN32)
    list(APPEND TEST_SOURCES 
        test_windows_platform.cpp 
        test_screenshot.cpp
        test_gui_interaction.cpp
        ../src/DirectInputFFB.cpp 
        ../src/GuiLayer.cpp
        ../src/GameConnector.cpp
        ${IMGUI_SOURCES}
    )
endif()

enable_testing()
add_executable(run_combined_tests ${TEST_SOURCES})

if(WIN32)
    target_link_libraries(run_combined_tests dinput8 dxguid version imm32 winmm d3d11 d3dcompiler dxgi)
endif()

# Add to CTest
add_test(NAME CombinedTests COMMAND run_combined_tests)
