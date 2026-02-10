cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 17)

# ImGui Core Detection (Always enabled for testing)
set(IMGUI_DIR "vendor/imgui")
if(EXISTS "${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui.cpp")
    message(STATUS "ImGui Core found.")
    add_definitions(-DENABLE_IMGUI)

    set(IMGUI_CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui.cpp
        ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui_widgets.cpp
    )
    include_directories(${IMGUI_DIR})
    include_directories(${IMGUI_DIR}/backends)
endif()

# Platform specific Backends
option(BUILD_HEADLESS "Build without GUI support" OFF)
if(BUILD_HEADLESS)
    add_definitions(-DHEADLESS_GUI)
endif()
set(IMGUI_BACKEND_SOURCES "")

if(NOT BUILD_HEADLESS AND IMGUI_CORE_SOURCES)
    if(WIN32)
        list(APPEND IMGUI_BACKEND_SOURCES
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/backends/imgui_impl_win32.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
        )
        link_libraries(d3d11 d3dcompiler dxguid)
        add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
    else()
        # Linux / GLFW + OpenGL
        find_package(glfw3 REQUIRED)
        find_package(OpenGL REQUIRED)

        list(APPEND IMGUI_BACKEND_SOURCES
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
            ${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        )

        include_directories(${OPENGL_INCLUDE_DIR})
    endif()
endif()

set(IMGUI_SOURCES ${IMGUI_CORE_SOURCES} ${IMGUI_BACKEND_SOURCES})

include_directories(src)

# Copy icon file into build directory (Windows only)
if(WIN32)
    configure_file(${CMAKE_SOURCE_DIR}/icon/lmuffb.ico ${CMAKE_BINARY_DIR}/lmuffb.ico COPYONLY)
endif()

# Tests
add_subdirectory(tests)

set(APP_SOURCES
    src/main.cpp
    src/GuiLayer_Common.cpp
    src/GuiLayer.h
    src/Config.cpp src/Config.h
    src/DirectInputFFB.cpp src/DirectInputFFB.h
    src/GameConnector.cpp src/GameConnector.h
    src/DynamicVJoy.h
    src/FFBEngine.h
)

if(WIN32)
    list(APPEND APP_SOURCES src/GuiLayer_Win32.cpp src/res.rc)
else()
    list(APPEND APP_SOURCES src/GuiLayer_Linux.cpp)
endif()

add_executable(LMUFFB ${APP_SOURCES} ${IMGUI_SOURCES})

if(WIN32)
    target_link_libraries(LMUFFB PRIVATE dinput8.lib dxguid.lib winmm.lib)
endif()

if(NOT WIN32 AND NOT BUILD_HEADLESS)
    target_link_libraries(LMUFFB PRIVATE glfw OpenGL::GL dl pthread)
endif()

# Read Version
file(STRINGS "VERSION" LMUFFB_VERSION)
add_definitions(-DLMUFFB_VERSION="${LMUFFB_VERSION}")

# Copy Distribution Files
add_custom_command(TARGET LMUFFB POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/README.txt"
            "$<TARGET_FILE_DIR:LMUFFB>/README.txt")
