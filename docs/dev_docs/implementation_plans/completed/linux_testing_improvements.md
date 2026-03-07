# Implementation Plan - Linux Testing & Reporting Improvements

## Problem Description
1.  **Platform Gap**: ~225 assertions are missing on Linux because `test_windows_platform.cpp` is excluded, despite containing many platform-agnostic logic tests.
2.  **Reporting**: Test runner reports total assertions, but not the number of test cases passed/failed.
3.  **Third-Party Files**: We modified provided files (`InternalsPlugin.hpp`, etc.) to support Linux, but this creates a maintenance burden. We need a non-invasive solution.

## Proposed Changes

### 1. Refactor Platform Tests
**Goal**: Move platform-agnostic tests out of `test_windows_platform.cpp` so they run on Linux.

*   **Create** `tests/test_ffb_logic.cpp`.
*   **Move** the following test cases from `tests/test_windows_platform.cpp` to the new file:
    *   `test_slider_precision_display`
    *   `test_slider_precision_regression`
    *   `test_latency_display_regression`
    *   `test_single_source_of_truth_t300_defaults` (renaming to `test_defaults_consistency`)
    *   `test_window_config_persistence` (renaming to `test_config_persistence_logic`)
    *   `test_config_persistence_guid`
    *   `test_config_always_on_top_persistence`
    *   `test_preset_management_system`
    *   `test_gui_style_application` (requires ImGui, which is available on Linux)
    *   `test_config_persistence_braking_group`
    *   `test_legacy_config_migration`
*   **Update** `tests/CMakeLists.txt` to include `test_ffb_logic.cpp` in the build.

### 2. Mocking Windows Types for Linux
**Goal**: Allow third-party files to compile without modification by mocking `windows.h`.

*   **Revert** changes to:
    *   `src/lmu_sm_interface/InternalsPlugin.hpp`
    *   `src/lmu_sm_interface/PluginObjects.hpp`
    *   `src/lmu_sm_interface/SharedMemoryInterface.hpp`
*   **Create** `src/lmu_sm_interface/linux_mock/windows.h`.
    *   Define `HWND`, `DWORD`, `HANDLE`, `BOOL`, `TRUE`, `FALSE` types/macros.
*   **Update** `CMakeLists.txt` (Main and Tests) to include `src/lmu_sm_interface/linux_mock` as an include directory on Linux.

### 3. Enhance Test Reporting
**Goal**: Report "Tests Passed/Failed" in addition to assertions.

*   **Modify** `tests/test_ffb_common.h`:
    *   Declare extern integers: `g_test_cases_run`, `g_test_cases_passed`, `g_test_cases_failed`.
*   **Modify** `tests/test_ffb_common.cpp`:
    *   Define the new globals.
    *   Update `Run()` loop to track failures per test function execution:
        ```cpp
        int initial_fails = g_tests_failed;
        test.func();
        if (g_tests_failed > initial_fails) g_test_cases_failed++;
        else g_test_cases_passed++;
        g_test_cases_run++;
        ```
*   **Modify** `tests/main_test_runner.cpp`:
    *   Update the summary output to print the new counters.

## Verification Plan

### Automated Tests
*   **Build (Linux Local)**: `cmake -S . -B build_linux -DHEADLESS_GUI=ON` (simulated via existing mock headers if needed, or just standard build). check for compilation errors in `InternalsPlugin.hpp`.
*   **Run Tests**: `run_combined_tests` should show:
    *   Higher assertion count (~925).
    *   New "Test Cases" summary line.
    *   No failures in the moved tests.

### Manual Verification
*   **Code Review**: Verify `InternalsPlugin.hpp` is identical to original (no `LinuxMock.h` include).
*   **Output Check**: Ensure the summary looks like:
    ```
    COMBINED TEST SUMMARY
    TEST CASES   : 54/54 (100%)
    ASSERTIONS   : 925 passed, 0 failed
    ```

## Implementation Notes
- Successfully moved logic tests to `tests/test_ffb_logic.cpp`. These tests are now platform-agnostic and will compile on Linux.
- Added `src/lmu_sm_interface/linux_mock/windows.h` to allow third-party ISI headers to compile on Linux without modification.
- Enhanced test reporting in `main_test_runner.cpp` to include test case counts.
- Verified on Windows that all 197 test cases and 928 assertions pass (recovering and slightly exceeding the previous 925 baseline).
- Restored full depth to `test_defaults_consistency`, `test_latency_display_regression`, and other logical tests to ensure no regression in test coverage during refactoring.
