# Investigation Report: codebase Improvements

## 1. Expanding Linux Tests
Currently, there is a discrepancy between the number of assertions run on Windows (~925) vs Linux (~700).

### Findings
The primary cause of this gap is the exclusion of the entire `tests/test_windows_platform.cpp` file in `tests/CMakeLists.txt`:

```cmake
if(WIN32)
    list(APPEND TEST_SOURCES
        test_windows_platform.cpp
        ...
    )
```

Inspection of `test_windows_platform.cpp` reveals that while it contains some Windows-specific code (e.g., `DirectInput`, `HWND` checks), it also hosts **significant platform-agnostic logic tests** that should be running on Linux.

Examples of portable tests currently locked in the Windows file:
*   `test_slider_precision_display`: Pure string formatting logic.
*   `test_latency_display_regression`: Pure arithmetic/logic.
*   `test_single_source_of_truth_t300_defaults`: Logic verification of the `Preset` system.
*   `test_window_config_persistence`: Verifies `Config` struct values (file I/O is standard C++).
*   `test_gui_style_application`: Uses `ImGui` which is cross-platform.

### Recommendations to Close the Gap
1.  **Refactor/Split `test_windows_platform.cpp`**:
    *   Move the logic-only tests (slider precision, latency display, defaults consistency, etc.) into a new or existing cross-platform file, such as `tests/test_ffb_logic.cpp` or `tests/test_ffb_common.cpp`.
    *   This alone will enable dozens of assertions on Linux.
2.  **Abstract Platform Types**:
    *   Tests that rely on `GUID` string conversion (`test_guid_string_conversion`) can be enabled on Linux if the `GUID` struct and conversion functions are mocked or abstracted in `LinuxMock.h`.
3.  **Mock Windows Handles**:
    *   GUI tests relying on `HWND` can be enabled if `HWND` is defined as `void*` in the Linux build (which is already being done partially via `LinuxMock.h`).

## 2. Improved Test Reporting
The current test runner outputs the total number of *assertions* passed/failed (`g_tests_passed`), but not the number of *test cases*.

### Findings
The test runner (`main_test_runner.cpp` and `test_ffb_common.h`) uses a simple counter system updated by `ASSERT_*` macros.
*   `TEST_CASE` macros register functions into a `TestRegistry`.
*   The `Run()` function (in `test_ffb_common.cpp`) iterates through these registered functions.

### Solution
To report both Test Cases and Assertions:

1.  **Add Test Case Counters**:
    *   In `main_test_runner.cpp`, introduce `int total_tests_run`, `total_tests_passed`, `total_tests_failed`.
2.  **Instrument the Runner Loop**:
    *   Modify the `Run()` loop in `test_ffb_common.cpp` (or wherever the registry iteration happens).
    *   Before executing a test function, record the current `g_tests_failed` count.
    *   Run the test function `entry.func()`.
    *   After return, check: `bool test_failed = (g_tests_failed > initial_failed_count);`
    *   Update the new Test Case counters accordingly.
3.  **Update Output**:
    *   Print the new counters in the summary block in `main_test_runner.cpp`.

## 3. Handling Third-Party Provided Files
You have updated `InternalsPlugin.hpp`, `PluginObjects.hpp`, and `SharedMemoryInterface.hpp` to support Linux, but these are third-party files that should ideally remain untouched to ease future updates.

### Findings
The files likely rely on `<windows.h>` or Windows-specific types (`HWND`) which are not present on Linux. The current solution modifies them to include `LinuxMock.h`.

### Proposed Solutions (Non-Invasive)

#### Option A: The "Mock Header" Strategy (Recommended)
If the original files utilize `#include <windows.h>`, you can "trick" the compiler on Linux without changing the source code.

1.  Create a file `src/lmu_sm_interface/linux_mock/windows.h`.
2.  In this file, add the necessary typedefs:
    ```cpp
    #pragma once
    // Mock Windows types for Linux
    typedef void* HWND;
    typedef unsigned long DWORD;
    // ... other types used by the plugins ...
    ```
3.  In `CMakeLists.txt`, specifically for Linux, add this directory to the include path *before* system includes:
    ```cmake
    if(NOT WIN32)
        include_directories(src/lmu_sm_interface/linux_mock)
    endif()
    ```
4.  When the compiler sees `#include <windows.h>` in the third-party files, it will grab your mock file instead of erroring.

#### Option B: Forced Include (GCC/Clang)
If the original files utilize `#ifdef _WIN32` guards around inclusions, they might end up with undefined symbols (like `HWND`) logic later in the file.

1.  Use the `-include` compiler flag to force the inclusion of your mock definitions *before* the file is processed.
2.  In `CMakeLists.txt`:
    ```cmake
    if(NOT WIN32)
        # Force include LinuxMock.h for every file (or specifically for the interface lib)
        add_compile_options("-include${CMAKE_SOURCE_DIR}/src/lmu_sm_interface/LinuxMock.h")
    endif()
    ```
    *Note: `LinuxMock.h` must contain the typedefs (`HWND`, etc.).* This ensures types are defined even if the file skips the Windows include block.

#### Option C: Local Wrappers
Use the wrapper files you already started (`InternalsPluginWrapper.h`), but you must ensure your *own* code includes the **Wrapper**, not the **Original**.
*   **Challenge**: If `SharedMemoryInterface.hpp` (provided file) includes `InternalsPlugin.hpp` (provided file), checking `SharedMemoryInterface.hpp` will still pull in the raw `InternalsPlugin.hpp`.
*   **Fix**: You would need to use Option A (Mock Header) to satisfy `SharedMemoryInterface.hpp`'s internal include.

**Conclusion**: **Option A** (creating a fake `windows.h`) is usually the cleanest for "header-only" dependencies that expect a Windows environment, combined with **Option B** (forced include) if the types are guarded by `_WIN32`.
