cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 17)

# Security & Debug Flags (MSVC)
if(MSVC)
    # Compile flags: Buffer Security Check (/GS) is usually default, but good to ensure.
    add_compile_options(/GS)
    
    # Always generate PDBs, even in Release mode (required for OpenCppCoverage)
    add_compile_options(/Zi)

    # Multi-processor compilation for faster builds (Issue #165 follow-up)
    add_compile_options(/MP)
    
    # Enable C++ exceptions explicitly
    add_compile_options(/EHsc)

    # Linker flags: ASLR (/DYNAMICBASE) and DEP (/NXCOMPAT)
    add_link_options(/DYNAMICBASE /NXCOMPAT)
    
    # Force debug info generation in the linker
    add_link_options(/DEBUG)
endif()

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
endif()

# Coverage Support (Linux/GCC/Clang)
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    if(NOT MSVC)
        message(STATUS "Enabling code coverage instrumentation...")
        # v0.7.145: Add -fprofile-update=atomic to prevent 'Unexpected negative count' in geninfo
        # during multi-threaded test execution on Linux.
        add_compile_options(--coverage -O0 -g -fprofile-update=atomic)
        add_link_options(--coverage)
    else()
        # On Windows/MSVC, OpenCppCoverage is used. 
        # It doesn't need flags, but we should be in Debug or at least unoptimized.
        message(STATUS "Code coverage enabled for MSVC. Ensure you use Debug or unoptimized Release.")
    endif()
    
    # Requirement: Make sure debug builds are used for code coverage
    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug" AND NOT MSVC)
        message(WARNING "Coverage is enabled but build type is NOT Debug. Coverage reports might be inaccurate.")
    endif()
endif()



# FetchContent for External Dependencies
include(FetchContent)

# ImGui Core Detection & FetchContent
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

# LZ4 Detection & FetchContent
set(LZ4_DIR "${CMAKE_SOURCE_DIR}/vendor/lz4")

if(EXISTS "${LZ4_DIR}/lz4.c")
    message(STATUS "LZ4 found locally.")
    set(LZ4_LOCAL_FOUND TRUE)
else()
    message(STATUS "LZ4 not found locally. Using FetchContent...")
    FetchContent_Declare(
      lz4
      GIT_REPOSITORY https://github.com/lz4/lz4.git
      GIT_TAG        v1.10.0
    )
    FetchContent_GetProperties(lz4)
    if(NOT lz4_POPULATED)
        FetchContent_Populate(lz4)
    endif()
    set(LZ4_DIR "${lz4_SOURCE_DIR}/lib")
    set(LZ4_FETCHED TRUE)
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
        link_libraries(d3d11 dxgi d3dcompiler dxguid)
        add_definitions(-DIMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)
    else()
        # Linux / GLFW + OpenGL
        find_package(glfw3 REQUIRED)
        find_package(OpenGL REQUIRED)

        list(APPEND IMGUI_BACKEND_SOURCES
            ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
            ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        )
    endif()
endif()

set(IMGUI_SOURCES ${IMGUI_CORE_SOURCES} ${IMGUI_BACKEND_SOURCES})

# Create a separate target for ImGui to isolate it from linting/warnings
if(IMGUI_SOURCES)
    add_library(imgui_vendor STATIC ${IMGUI_SOURCES})
    # Mark headers as SYSTEM to suppress warnings in files that include them
    target_include_directories(imgui_vendor SYSTEM PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
    # Disable clang-tidy for this target explicitly
    set_target_properties(imgui_vendor PROPERTIES CXX_CLANG_TIDY "")
    # Disable اکثر warnings for vendor code
    if(MSVC)
        target_compile_options(imgui_vendor PRIVATE /W0)
    else()
        target_compile_options(imgui_vendor PRIVATE -w)
    endif()
endif()


if(NOT WIN32)
    include_directories(src/lmu_sm_interface/linux_mock)
    include_directories(src/lmu_sm_interface)
endif()
include_directories(src)
include_directories(${LZ4_DIR})

# Copy icon file into build directory (Windows only)
if(WIN32)
    configure_file(${CMAKE_SOURCE_DIR}/icon/lmuffb.ico ${CMAKE_BINARY_DIR}/lmuffb.ico COPYONLY)
endif()

# Read Version
file(STRINGS "VERSION" LMUFFB_VERSION)

# Ensure clang-tidy sees exception support on Windows
if(CMAKE_CXX_CLANG_TIDY AND MSVC)
    list(APPEND CMAKE_CXX_CLANG_TIDY "--extra-arg=/EHsc" "--extra-arg=-fexceptions" "--extra-arg=-fcxx-exceptions")
endif()
message(STATUS ">>> CMake VERSION detected: [${LMUFFB_VERSION}]")

# Parse version for Resource File (e.g. 0.7.29 -> 0,7,29,0)
string(REPLACE "." ";" VERSION_LIST ${LMUFFB_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)
set(LMUFFB_VERSION_STR "${LMUFFB_VERSION}")
set(LMUFFB_VERSION_COMMA "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},0")

# Auto-generate versioned files in the BUILD directory
configure_file(src/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/Version.h @ONLY)
if(WIN32)
    configure_file(src/res.rc.in ${CMAKE_CURRENT_BINARY_DIR}/src/res.rc @ONLY)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(src)
add_definitions(-DLMUFFB_VERSION="${LMUFFB_VERSION}")

# Core Library (Shared between App and Tests)
set(CORE_SOURCES
    ${LZ4_DIR}/lz4.c
    src/GuiLayer_Common.cpp
    src/GuiLayer.h
    src/DXGIUtils.cpp
    src/DXGIUtils.h
    src/Config.cpp src/Config.h
    src/DirectInputFFB.cpp src/DirectInputFFB.h
    src/GameConnector.cpp src/GameConnector.h
    src/FFBEngine.h src/FFBEngine.cpp
    src/GripLoadEstimation.cpp
    src/SteeringUtils.cpp
    src/VehicleUtils.cpp
    src/RestApiProvider.cpp
    src/RestApiProvider.h
    src/UpSampler.cpp
    src/UpSampler.h
)

if(WIN32)
    if (NOT BUILD_HEADLESS)
        list(APPEND CORE_SOURCES src/GuiLayer_Win32.cpp ${CMAKE_CURRENT_BINARY_DIR}/src/res.rc)
    else()
        list(APPEND CORE_SOURCES src/GuiLayer_Win32.cpp)
    endif()
else()
    list(APPEND CORE_SOURCES src/GuiLayer_Linux.cpp)
endif()

# Optimized Core for the Main Application
add_library(LMUFFB_Core STATIC ${CORE_SOURCES})
target_include_directories(LMUFFB_Core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/src src)
if(TARGET imgui_vendor)
    target_link_libraries(LMUFFB_Core PUBLIC imgui_vendor)
endif()

# Fast-Compile Core for Tests (Reduced Optimization)
add_library(LMUFFB_Core_Fast STATIC ${CORE_SOURCES})
target_include_directories(LMUFFB_Core_Fast PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/src src)
if(TARGET imgui_vendor)
    target_link_libraries(LMUFFB_Core_Fast PUBLIC imgui_vendor)
endif()
target_compile_definitions(LMUFFB_Core_Fast PRIVATE LMUFFB_UNIT_TEST)

if(MSVC)
    # Ensure LMUFFB_Core is optimized in Release
    target_compile_options(LMUFFB_Core PRIVATE $<$<CONFIG:Release>:/O2>)
    # Ensure LMUFFB_Core_Fast is unoptimized in all configurations
    target_compile_options(LMUFFB_Core_Fast PRIVATE /Od)
else()
    # Linux / GCC
    target_compile_options(LMUFFB_Core PRIVATE $<$<CONFIG:Release>:-O3>)
    target_compile_options(LMUFFB_Core_Fast PRIVATE -O0)
endif()

if(WIN32)
    target_link_libraries(LMUFFB_Core PUBLIC dinput8 dxgi dxguid winmm wininet)
    target_link_libraries(LMUFFB_Core_Fast PUBLIC dinput8 dxgi dxguid winmm wininet)
endif()

if(NOT WIN32 AND NOT BUILD_HEADLESS)
    target_link_libraries(LMUFFB_Core PUBLIC glfw OpenGL::GL dl pthread)
    target_link_libraries(LMUFFB_Core_Fast PUBLIC glfw OpenGL::GL dl pthread)
else()
    # On headless or windows
    # target_link_libraries(LMUFFB_Core PUBLIC ...)
endif()

# Main Application
set(APP_SOURCES
    src/main.cpp
)

if(WIN32 AND NOT BUILD_HEADLESS)
    list(APPEND APP_SOURCES ${CMAKE_BINARY_DIR}/src/res.rc)
endif()

add_executable(LMUFFB ${APP_SOURCES})
if(WIN32)
    set_target_properties(LMUFFB PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        set_target_properties(LMUFFB PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
    endif()
endif()
target_link_libraries(LMUFFB PRIVATE LMUFFB_Core)

# Tests
add_subdirectory(tests)

# Tools Distribution - Automatic Globbing (Issue #317 follow-up)
file(GLOB TOOL_ROOT_FILES CONFIGURE_DEPENDS 
    "${CMAKE_SOURCE_DIR}/tools/lmuffb_log_analyzer/*.py"
    "${CMAKE_SOURCE_DIR}/tools/lmuffb_log_analyzer/*.txt"
    "${CMAKE_SOURCE_DIR}/tools/lmuffb_log_analyzer/*.md"
)
file(GLOB ANALYZER_FILES CONFIGURE_DEPENDS 
    "${CMAKE_SOURCE_DIR}/tools/lmuffb_log_analyzer/analyzers/*.py"
)
file(GLOB EXPORTER_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/tools/lmuffb_log_analyzer/exporters/*.py"
)

set(TOOL_COPY_COMMANDS "")
foreach(F ${TOOL_ROOT_FILES})
    list(APPEND TOOL_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_if_different "${F}" "$<TARGET_FILE_DIR:LMUFFB>/tools/lmuffb_log_analyzer/")
endforeach()
foreach(F ${ANALYZER_FILES})
    list(APPEND TOOL_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_if_different "${F}" "$<TARGET_FILE_DIR:LMUFFB>/tools/lmuffb_log_analyzer/analyzers/")
endforeach()
foreach(F ${EXPORTER_FILES})
    list(APPEND TOOL_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_if_different "${F}" "$<TARGET_FILE_DIR:LMUFFB>/tools/lmuffb_log_analyzer/exporters/")
endforeach()

# Copy Distribution Files & Tools
add_custom_command(TARGET LMUFFB POST_BUILD
    # README
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/README.txt" "$<TARGET_FILE_DIR:LMUFFB>/README.txt"
    # Tools folder - Clear and create subfolders
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:LMUFFB>/tools"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LMUFFB>/tools/lmuffb_log_analyzer/analyzers"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LMUFFB>/tools/lmuffb_log_analyzer/exporters"
    # Execute automatic copy commands
    ${TOOL_COPY_COMMANDS}
)

