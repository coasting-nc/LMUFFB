# Linux Test Verification Report

## Overview
This report documents the verification of the LMUFFB build and test suite in a Linux environment. The goal was to ensure that the core logic (physics, config, etc.) is correctly verified on Linux, even if the hardware-specific layers (DirectInput, Windows Shared Memory) are mocked.

## Build Status
- **Result**: Successful
- **Build Configuration**: Headless mode (`-DBUILD_HEADLESS=ON`)
- **Dependencies**:
    - ImGui: Required manual download into `vendor/imgui` as it is not bundled in the repository.
    - Standard Linux build tools (GCC, CMake).

### Build Commands Used:
```bash
# Download ImGui (manual step required)
mkdir -p vendor/imgui
curl -L https://github.com/ocornut/imgui/archive/refs/heads/master.zip -o vendor/imgui-master.zip
unzip vendor/imgui-master.zip -d vendor
cp -r vendor/imgui-master/* vendor/imgui/
rm -rf vendor/imgui-master.zip vendor/imgui-master

# Build App and Tests
cmake -S . -B build -DBUILD_HEADLESS=ON
cmake --build build --config Release
```

## Test Results
- **Test Cases**: 192 / 192 passed
- **Assertions**: 912 passed / 0 failed

## Comparison with Windows
| Metric | Windows | Linux | Difference |
| :--- | :--- | :--- | :--- |
| **Test Cases** | 197 | 192 | -5 |
| **Assertions** | 928 | 912 | -16 |

### Identified Missing Test Cases (Linux)
The following test cases are skipped on Linux as they depend on Windows-specific APIs:
1.  `test_window_always_on_top_behavior` (3 assertions) - Uses `SetWindowPos` and `WS_EX_TOPMOST`.
2.  `test_icon_presence` (1 assertion) - Checks for Windows build artifacts.
3.  `test_game_connector_staleness` (3 assertions) - Uses Windows Shared Memory APIs (`CreateFileMapping`, `MapViewOfFile`).
4.  `test_executable_metadata` (2 assertions) - Uses `GetFileVersionInfo`.
5.  `test_is_window_safety` (3 assertions) - Uses `IsWindow` and `GetConsoleWindow`.

### Identified Missing Assertions (Linux)
In addition to the 12 assertions from the missing test cases above:
- `test_game_connector_lifecycle`: 1 assertion missing because the connection fails on Linux (due to missing LMU shared memory), skipping the "connected" verification path.
- Other minor discrepancies in conditional logic paths within portable tests.

## Issues Encountered
1.  **Missing Dependencies**: ImGui is not included in the repository and requires a manual download. This is documented in the README for Windows but needs similar clarity or automation for Linux.
2.  **Platform Coupling**: Several tests are tightly coupled to Windows APIs even when the logic being tested (like GameConnector staleness) could conceptually be verified using a more abstract shared memory mock.

## Suggestions for Improvement

### 1. Automate Dependency Management
- **Action**: Update `CMakeLists.txt` to use `FetchContent` or a similar mechanism to automatically download ImGui if it is missing from `vendor/imgui`. This would simplify the setup for both Windows and Linux developers.

### 2. Increase Linux Test Coverage
- **Mocking Shared Memory**: Expand the Linux mock layer to simulate the LMU shared memory file. This would allow `test_game_connector_staleness` and the full `test_game_connector_lifecycle` to run on Linux.
- **Abstracting Windowing Logic**: Move window-specific checks (like Always on Top) into an interface, allowing a mock implementation for Linux that can still verify that the correct *calls* are being made by the business logic.
- **Icon Presence on Linux**: Adjust `test_icon_presence` to look for the source icon file or a Linux-equivalent artifact to ensure the asset exists in the repo.

### 3. CI Integration
- With 912 assertions already passing on Linux, it is highly recommended to integrate this headless build/test process into a CI pipeline (e.g., GitHub Actions) to catch regressions in core physics and config logic immediately.
