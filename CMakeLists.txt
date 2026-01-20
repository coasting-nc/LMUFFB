cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 20)

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

# Tests
# add_subdirectory(tests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")

add_executable(LMUFFB
  src/main.cpp
  src/GuiLayer.cpp
  src/Config.cpp
  src/DirectInputFFB.cpp
  src/GameConnector.cpp
  src/DynamicVJoy.h
  src/FFBEngine.h
  ${IMGUI_SOURCES}
)

include(FetchContent)
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG master
)
FetchContent_MakeAvailable(yaml-cpp)

target_link_libraries(LMUFFB PUBLIC
  dinput8.lib
  dxguid.lib
  winmm.lib
  yaml-cpp::yaml-cpp
)

# Read Version
file(STRINGS "VERSION" LMUFFB_VERSION)
add_definitions(-DLMUFFB_VERSION="${LMUFFB_VERSION}")

# Copy Distribution Files
add_custom_command(TARGET LMUFFB POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/README.txt"
            "$<TARGET_FILE_DIR:LMUFFB>/README.txt"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/default_config/config.yaml"
            "$<TARGET_FILE_DIR:LMUFFB>/config.yaml")

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
