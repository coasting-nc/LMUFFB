cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 17)

# vJoy SDK Location - Users should set this
set(VJOY_SDK_DIR "C:/Program Files/vJoy/SDK" CACHE PATH "Path to vJoy SDK")

# ImGui Detection
set(IMGUI_DIR "vendor/imgui")
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
    message(STATUS "ImGui not found. Building in Headless mode.")
    set(IMGUI_SOURCES "")
endif()

include_directories(${VJOY_SDK_DIR}/inc)
link_directories(${VJOY_SDK_DIR}/lib/amd64)

add_executable(LMUFFB main.cpp rF2Data.h src/GuiLayer.cpp src/GuiLayer.h ${IMGUI_SOURCES})

target_link_libraries(LMUFFB vJoyInterface.lib)

# Copy dll to output if possible, or instruct user
