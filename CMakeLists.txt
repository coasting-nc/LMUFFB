cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 17)

# Security Flags (MSVC)
if(MSVC)
    # Compile flags: Buffer Security Check (/GS) is usually default, but good to ensure.
    add_compile_options(/GS)

    # Linker flags: ASLR (/DYNAMICBASE) and DEP (/NXCOMPAT)
    add_link_options(/DYNAMICBASE /NXCOMPAT)
endif()

# ImGui Core Detection & FetchContent
include(FetchContent)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/vendor/imgui")

if(EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "ImGui Core found locally.")
    set(IMGUI_LOCAL_FOUND TRUE)
else()
    message(STATUS "ImGui not found locally. Using FetchContent...")
    FetchContent_Declare(
      imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG        v1.91.8
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()
    set(IMGUI_DIR "${imgui_SOURCE_DIR}")
    set(IMGUI_FETCHED TRUE)
endif()

if(EXISTS "${IMGUI_DIR}/imgui.cpp")
    add_definitions(-DENABLE_IMGUI)

    set(IMGUI_CORE_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
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
            ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
            ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
        )
        link_libraries(d3d11 d3dcompiler dxguid)
        add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DIMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)
    else()
        # Linux / GLFW + OpenGL
        find_package(glfw3 REQUIRED)
        find_package(OpenGL REQUIRED)

        list(APPEND IMGUI_BACKEND_SOURCES
            ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
            ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        )

        include_directories(${OPENGL_INCLUDE_DIR})
    endif()
endif()

set(IMGUI_SOURCES ${IMGUI_CORE_SOURCES} ${IMGUI_BACKEND_SOURCES})

include_directories(src)
if(NOT WIN32)
    include_directories(src/lmu_sm_interface/linux_mock)
    include_directories(src/lmu_sm_interface)
endif()

# Copy icon file into build directory (Windows only)
if(WIN32)
    configure_file(${CMAKE_SOURCE_DIR}/icon/lmuffb.ico ${CMAKE_BINARY_DIR}/lmuffb.ico COPYONLY)
endif()

# Read Version
file(STRINGS "VERSION" LMUFFB_VERSION)

# Parse version for Resource File (e.g. 0.7.29 -> 0,7,29,0)
string(REPLACE "." ";" VERSION_LIST ${LMUFFB_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)
set(LMUFFB_VERSION_STR "${LMUFFB_VERSION}")
set(LMUFFB_VERSION_COMMA "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},0")

# Auto-generate versioned files in the BUILD directory
# This keeps the source tree clean and ensures a single source of truth.
configure_file(src/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/Version.h @ONLY)
if(WIN32)
    configure_file(src/res.rc.in ${CMAKE_CURRENT_BINARY_DIR}/src/res.rc @ONLY)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
add_definitions(-DLMUFFB_VERSION="${LMUFFB_VERSION}")

# Tests
add_subdirectory(tests)

set(APP_SOURCES
    src/main.cpp
    src/GuiLayer_Common.cpp
    src/GuiLayer.h
    src/Config.cpp src/Config.h
    src/DirectInputFFB.cpp src/DirectInputFFB.h
    src/GameConnector.cpp src/GameConnector.h
    src/FFBEngine.h
)

if(WIN32)
    list(APPEND APP_SOURCES src/GuiLayer_Win32.cpp ${CMAKE_CURRENT_BINARY_DIR}/src/res.rc)
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

# Copy Distribution Files
add_custom_command(TARGET LMUFFB POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/README.txt"
            "$<TARGET_FILE_DIR:LMUFFB>/README.txt")
