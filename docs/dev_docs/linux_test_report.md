# Linux Test Verification Report

## Overview
This report documents the verification of the LMUFFB build and test suite in a Linux environment. Following initial assessment and feedback, several improvements were implemented to automate dependency management, increase Linux test coverage, and enhance test reliability.

## Build Status
- **Result**: Successful
- **Build Configuration**: Headless mode (`-DBUILD_HEADLESS=ON`)
- **Dependencies**:
    - **Automated ImGui**: Automatically handled via `FetchContent` in `CMakeLists.txt`. Pinned to version `v1.91.8` for stability.
    - Standard Linux build tools (GCC, CMake).

### Build Commands:
```bash
# Build App and Tests (ImGui is automatically fetched if needed)
cmake -S . -B build -DBUILD_HEADLESS=ON
cmake --build build --config Release
```

## Test Results
- **Test Cases**: 195 / 195 passed
- **Assertions**: 920 passed / 0 failed

## Comparison with Windows
| Metric | Windows | Linux (Before) | Linux (After Improvements) | Gap |
| :--- | :--- | :--- | :--- | :--- |
| **Test Cases** | 197 | 192 | 195 | -2 |
| **Assertions** | 928 | 912 | 920 | -8 |

### Improvements Implemented

#### 1. Automated Dependency Management
- Integrated `FetchContent` for ImGui in the root `CMakeLists.txt`, pinned to `v1.91.8`. This removes manual setup steps for developers on all platforms.

#### 2. Enhanced Shared Memory Mocking
- Expanded `src/lmu_sm_interface/LinuxMock.h` to include a functional memory-mapped file mock using global storage.
- Mocked `CreateFileMappingA`, `OpenFileMappingA`, `MapViewOfFile`, and `UnmapViewOfFile`.
- Enabled `SharedMemoryLock` implementation on Linux (using `HEADLESS_GUI` guard) to allow testing concurrent-safe telemetry access.
- Updated `test_game_connector_lifecycle` and `test_game_connector_staleness` to fully exercise the connection logic on Linux via mocks.

#### 3. Windowing Logic Abstraction
- Introduced `IGuiPlatform` interface in `src/GuiPlatform.h` to abstract platform-specific windowing calls.
- Implemented `Win32GuiPlatform` and `LinuxGuiPlatform` (with headless mock support).
- Refactored `SetWindowAlwaysOnTopPlatform` and other helpers to use this interface.
- Verified state changes via `test_window_always_on_top_interface` using mock state inspection.

#### 4. Increased Platform Test Coverage
- Enabled `test_windows_platform.cpp` on Linux builds.
- Updated `test_icon_presence` to support Linux source tree layout, ensuring the `lmuffb.ico` asset exists.
- Resolved potential namespace resolution issues by using explicit global scope (`::GetGuiPlatform()`) in tests.

### Remaining Gaps
The following tests remain Windows-only due to deep coupling with OS internals that are not yet mocked:
1. `test_window_always_on_top_behavior`: Checks actual Win32 window styles (`WS_EX_TOPMOST`).
2. `test_executable_metadata`: Uses Windows Resource API (`GetFileVersionInfo`) to check binary metadata.
3. `test_is_window_safety`: Verifies `IsWindow` behavior for valid/invalid handles.

## Summary
The Linux environment now verifies **99% of the test cases** (195/197) and **99% of assertions** (920/928) compared to the Windows baseline. The core physics engine, configuration persistence, and platform-agnostic GUI logic are fully validated on every build, with a simplified setup process.
