# Code Coverage Challenges Report - Coverage Boost V6

This report discusses the challenges encountered when attempting to increase code coverage for specific files in the LMUFFB project.

## Priority 1: High-Level Interface and Mocking

### LinuxMock.h (Branch Coverage)
*   **Challenge:** Many branches in `LinuxMock.h` are for error conditions in simulated Win32 API calls (e.g., `CreateFileMappingA` failing).
*   **Strategy:** Used a `FailNext()` toggle to force failures in critical paths. Added `WaitResult()` to simulate timeout and failure in synchronization primitives.
*   **Status:** Branch coverage increased from 69.4% to 74.3%. Remaining branches often involve `nullptr` checks for parameters that are rarely `nullptr` in actual usage, or complex `VerQueryValueA` logic.

### SafeSharedMemoryLock.h (Branch Coverage)
*   **Challenge:** The only missing branch was the failure path in `MakeSafeSharedMemoryLock` when the underlying `SharedMemoryLock` fails to initialize.
*   **Strategy:** Successfully triggered this using `MockSM::FailNext()`.
*   **Status:** 100% line coverage achieved. Branch coverage is limited by the simple logic of the wrapper.

### SharedMemoryInterface.hpp (Branch Coverage)
*   **Challenge:** Triggering contention in the spin-lock and handling the timeout in `Lock()` is non-deterministic in a unit test environment.
*   **Strategy:** Used a separate thread to hold the lock while the main test thread attempted to acquire it with a short timeout.
*   **Status:** Coverage for `CopySharedMemoryObj` reached high levels by testing all `SharedMemoryEvent` flags.

## Priority 2: Application Entry and GUI Logic

### main.cpp (Function and Line Coverage)
*   **Challenge:** `FFBThread` is a long-running loop with many time-based branches (e.g., 5-second intervals for health warnings). Running the loop long enough to trigger these makes the test suite slow. Additionally, `lmuffb_app_main` blocks on the main loop.
*   **Strategy:** Ran `FFBThread` in a separate thread and used `g_running` to signal exit. Used `lmuffb_app_main` with `--headless` and simulated early exit.
*   **Status:** Line coverage is ~50%, Function coverage ~60%. Many branches are Windows-only (`timeBeginPeriod`) or related to `SIGTERM` which is hard to test repeatedly.

### GuiPlatform.h (Function Coverage)
*   **Challenge:** This is an interface with virtual methods.
*   **Strategy:** Used a `DummyPlatform` and `GetGuiPlatform()` (which returns a `LinuxPlatform` mock in headless mode) to invoke methods.
*   **Status:** 100% line/function coverage.

## Priority 3: Component Logic

### GuiLayer_Common.cpp and GuiWidgets.h (Line/Branch Coverage)
*   **Challenge:** These files contain massive amounts of ImGui UI code. Every slider, checkbox, and tree node creates multiple branches in the ImGui backend. Exhaustive coverage requires simulating clicks/drags on every single UI element in every possible state.
*   **Strategy:** Used `GuiLayerTestAccess` to call `DrawTuningWindow` and `DrawDebugWindow`. Implemented systematic fuzzing of mouse positions in `test_gui_interaction_v2.cpp`.
*   **Status:** Line coverage for `GuiLayer_Common.cpp` remains at ~68.7% due to the sheer volume of UI code.

### GameConnector.cpp (Line/Branch Coverage)
*   **Challenge:** Testing connection failures requires precise control over the mock shared memory state.
*   **Strategy:** Added tests for legacy conflict detection and failed lock initialization.
*   **Status:** Line coverage increased from 89.1% to 93.5%. Branch coverage increased from 55.9% to 60.8%.

## Priority 4: Branch Coverage in Core Physics/Logic

### Config.cpp
*   **Challenge:** The `ParsePresetLine` and `Load` functions have hundreds of branches for every possible INI key.
*   **Strategy:** Created a comprehensive `test_comprehensive.ini` with all known keys and some legacy ones to trigger migration logic.
*   **Status:** Coverage is high (96.6% line), but some branches for extremely rare legacy keys or specific `std::stof` catch blocks are hard to hit.

### FFBEngine.cpp and GripLoadEstimation.cpp
*   **Challenge:** Complex physics interactions (e.g., slope detection, kinematic fallback) require specific telemetry sequences.
*   **Strategy:** Added targeted tests for kinematic load on different wheels, manual slip ratio fallbacks, and zero-load cases in grip calculation.
*   **Status:** `GripLoadEstimation.cpp` branch coverage improved to 76.8%.

### DirectInputFFB.cpp
*   **Challenge:** This file is almost entirely platform-specific (Win32).
*   **Strategy:** Targeted the cross-platform GUID conversion logic and the force update optimization.
*   **Status:** Line coverage is ~96.8%. Remaining branches are mostly DirectInput error recovery paths which are only reachable on Windows with actual hardware.

### Logger.h, RateMonitor.h, AsyncLogger.h
*   **Challenge:** `AsyncLogger` uses a worker thread and decimation logic.
*   **Strategy:** Added tests for markers, filename sanitization (all special characters), and the 1-second interval logic in `RateMonitor`.
*   **Status:** High coverage achieved across all three utilities.
