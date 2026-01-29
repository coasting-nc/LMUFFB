cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 17)

# vJoy SDK Location - Users should set this
# We link against the installed vJoy SDK (headers/libs), we do NOT compile vJoy itself.
# ImGui Detection
set(IMGUI_DIR "vendor/imgui")
option(BUILD_HEADLESS "Build without GUI support" OFF)

if(NOT BUILD_HEADLESS)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui.cpp")
        message(STATUS "ImGui found. Enabling GUI support.")
        add_definitions(-DENABLE_IMGUI)
        set(IMGUI_SOURCES
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_demo.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_draw.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_tables.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_widgets.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/backends/imgui_impl_win32.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
        )
        include_directories(${IMGUI_DIR})
        include_directories(${IMGUI_DIR}/backends)
        # Link DirectX libraries
        link_libraries(d3d11 d3dcompiler dxguid)
        add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
    else()
        message(FATAL_ERROR "ImGui not found in vendor/imgui. To build headless, use -DBUILD_HEADLESS=ON.")
    endif()
else()
    message(STATUS "Building in Headless mode (User Request).")
    set(IMGUI_SOURCES "")
endif()

# include_directories(${VJOY_SDK_DIR}/inc)

# Detect platform and use appropriate vJoy library
# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit
#    link_directories(${VJOY_SDK_DIR}/lib/amd64)
# else()
    # 32-bit
#    link_directories(${VJOY_SDK_DIR}/lib/i386)
# endif()

# Copy icon file into build directory
configure_file(${CMAKE_SOURCE_DIR}/icon/lmuffb.ico ${CMAKE_BINARY_DIR}/lmuffb.ico COPYONLY)

# Tests
add_subdirectory(tests)

add_executable(LMUFFB 
               src/main.cpp
               src/GuiLayer.cpp src/GuiLayer.h
               src/Config.cpp src/Config.h
               src/DirectInputFFB.cpp src/DirectInputFFB.h
               src/GameConnector.cpp src/GameConnector.h
               src/DynamicVJoy.h 
               src/FFBEngine.h 
               src/res.rc
               ${IMGUI_SOURCES})

target_link_libraries(LMUFFB dinput8.lib dxguid.lib winmm.lib)

# Read Version
file(STRINGS "VERSION" LMUFFB_VERSION)
add_definitions(-DLMUFFB_VERSION="${LMUFFB_VERSION}")

# Copy Distribution Files
add_custom_command(TARGET LMUFFB POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/README.txt"
            "$<TARGET_FILE_DIR:LMUFFB>/README.txt")

# Copy vJoy DLL to output directory
# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit
#    add_custom_command(TARGET LMUFFB POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#                "${VJOY_SDK_DIR}/lib/amd64/vJoyInterface.dll"
#                "$<TARGET_FILE_DIR:LMUFFB>/vJoyInterface.dll")
# else()
    # 32-bit
#    add_custom_command(TARGET LMUFFB POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#                "${VJOY_SDK_DIR}/lib/vJoyInterface.dll"
#                "$<TARGET_FILE_DIR:LMUFFB>/vJoyInterface.dll")
# endif()
