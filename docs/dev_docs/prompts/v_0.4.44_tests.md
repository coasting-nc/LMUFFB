Please proceed with the following task:

**Task: Implement Windows-Specific Unit Tests**

**Reference Documents:**
*   `docs/dev_docs/windows_platform_tests_plan.md` (Contains the CMake configuration and the full C++ source code for the tests).

**Context:**
We are adding platform-specific features (DirectInput Device Persistence and Window Title Diagnostics). Since these rely on Windows APIs (`<windows.h>`, `<dinput.h>`), they cannot be tested in our standard Linux-based CI environment.
We need to create a separate test suite that is only built and run when compiling on Windows.

**Implementation Requirements:**

1.  **Create `tests/test_windows_platform.cpp`**:
    *   Copy the C++ source code provided in the reference document (`docs/dev_docs/windows_platform_tests_plan.md`).
    *   This file includes tests for:
        *   `test_guid_string_conversion`: Verifies binary GUID <-> String serialization.
        *   `test_window_title_extraction`: Verifies access to the foreground window API.
        *   `test_config_persistence_guid`: Verifies saving/loading the device ID.

2.  **Update `tests/CMakeLists.txt`**:
    *   Add a conditional block `if(WIN32)` ... `endif()`.
    *   Inside this block, define a new executable target `run_tests_win32`.
    *   Link it against `dinput8`, `dxguid`, `version`, `imm32`, and `winmm`.
    *   Register the test with CTest using `add_test`.
    *   *Refer to the reference document for the exact CMake syntax.*

**Deliverables:**
*   New file: `tests/test_windows_platform.cpp`
*   Modified file: `tests/CMakeLists.txt`

**Check-list for completion:**
- [ ] `tests/test_windows_platform.cpp` created with correct logic.
- [ ] `tests/CMakeLists.txt` updated with `if(WIN32)` guard.
- [ ] Verify that `run_tests_win32` is NOT built on Linux (to prevent CI failure).