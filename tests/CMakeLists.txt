cmake_minimum_required(VERSION 3.10)
project(LMUFFB_Tests)

set(CMAKE_CXX_STANDARD 17)

# Include main source dir for headers
include_directories(..)

# Test Executable
add_executable(run_tests test_ffb_engine.cpp ../src/Config.cpp)

# Enable testing
enable_testing()
add_test(NAME CoreLogicTest COMMAND run_tests)

# Windows-Specific Tests (DirectInput Helpers, Persistence)
if(WIN32)
    add_executable(run_tests_win32 
        test_windows_platform.cpp 
        ../src/DirectInputFFB.cpp 
        ../src/Config.cpp
        ../src/GuiLayer.cpp
        ../src/GameConnector.cpp
        ${IMGUI_SOURCES}
    )
    
    # Link required Windows libraries
    target_link_libraries(run_tests_win32 dinput8 dxguid version imm32 winmm d3d11 d3dcompiler dxgi)
    
    # Add to CTest
    add_test(NAME WindowsPlatformTest COMMAND run_tests_win32)
endif()
