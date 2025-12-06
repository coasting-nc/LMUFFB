cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 17)

# vJoy SDK Location - Users should set this
# We link against the installed vJoy SDK (headers/libs), we do NOT compile vJoy itself.
set(VJOY_SDK_DIR "C:/Program Files/vJoy/SDK" CACHE PATH "Path to vJoy SDK")

# ImGui Detection
set(IMGUI_DIR "vendor/imgui")
option(BUILD_HEADLESS "Build without GUI support" OFF)

if(NOT BUILD_HEADLESS)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui.cpp")
        message(STATUS "ImGui found. Enabling GUI support.")
        add_definitions(-DENABLE_IMGUI)
        set(IMGUI_SOURCES
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_demo.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
            ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
        )
        include_directories(${IMGUI_DIR})
        include_directories(${IMGUI_DIR}/backends)
        # Link DirectX libraries
        link_libraries(d3d11 d3dcompiler dxguid)
    else()
        message(FATAL_ERROR "ImGui not found in vendor/imgui. To build headless, use -DBUILD_HEADLESS=ON.")
    endif()
else()
    message(STATUS "Building in Headless mode (User Request).")
    set(IMGUI_SOURCES "")
endif()

include_directories(${VJOY_SDK_DIR}/inc)

# Detect platform and use appropriate vJoy library
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit
    link_directories(${VJOY_SDK_DIR}/lib/amd64)
else()
    # 32-bit
    link_directories(${VJOY_SDK_DIR}/lib/i386)
endif()

add_executable(LMUFFB main.cpp rF2Data.h src/GuiLayer.cpp src/GuiLayer.h src/Config.cpp src/Config.h ${IMGUI_SOURCES})

target_link_libraries(LMUFFB vJoyInterface.lib)

# Copy vJoy DLL to output directory
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit
    add_custom_command(TARGET LMUFFB POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${VJOY_SDK_DIR}/lib/amd64/vJoyInterface.dll"
                "$<TARGET_FILE_DIR:LMUFFB>/vJoyInterface.dll")
else()
    # 32-bit
    add_custom_command(TARGET LMUFFB POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${VJOY_SDK_DIR}/lib/vJoyInterface.dll"
                "$<TARGET_FILE_DIR:LMUFFB>/vJoyInterface.dll")
endif()
