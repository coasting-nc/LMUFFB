# Changelog

All notable changes to this project will be documented in this file.

## [0.7.62] - 2026-02-19
### Added
- **Game FFB (FFBTorque) Visualization (#138)**:
  - Added "Direct Torque" (400Hz native LMU) and "Shaft Torque" (100Hz legacy) to the diagnostic graphs.
  - Users can now compare both torque sources side-by-side to diagnose weak or missing FFB issues.
  - Included both torque channels in the asynchronous telemetry logs (CSV) for offline analysis.
  - Renamed the primary torque graph to "Selected Torque" to clarify which signal is driving the physics.
- **Testing**:
  - Added `tests/test_ffb_diag_updates.cpp` to verify data propagation from shared memory to snapshots and logs.

## [0.7.61] - 2026-02-19
### Added
- **Soft Lock Feature (#117)**:
  - Implemented physical resistance when the steering wheel reaches the car's maximum range.
  - Combines a progressive spring force with steering velocity damping for a realistic and stable "hard stop" feel.
  - Added configurable **Stiffness** and **Damping** parameters to the "General FFB" section in the GUI.
  - Integrated Soft Lock into the telemetry snapshot system and added a real-time diagnostic plot.
  - Updated all presets and configuration logic to persist new settings.
- **Improved Thread Safety**:
  - Migrated `g_engine_mutex` to `std::recursive_mutex` to support nested locking patterns in complex GUI and configuration workflows.
  - Ensured consistent locking across `Config::Save`, `Config::Load`, and UI settings modifications.
- **Testing**:
  - Added `tests/test_ffb_soft_lock.cpp` to verify spring and damping logic across steering limits.

## [0.7.60] - 2026-02-19
### Fixed
- **Tire Load Normalization (#120)**:
  - Replaced hardcoded Newton constants with class-aware dynamic thresholds for suspension bottoming and static load fallbacks.
  - Fixed false bottoming triggers in high-downforce vehicles (Hypercars, LMP2) by scaling the safety threshold to 1.6x the car's peak load.
  - Aligned `LMP2_UNSPECIFIED` seed load with header definition (8000N).
  - Improved `m_static_front_load` handling by resetting it on car class changes to avoid carry-over from previous sessions.
- **Build & Portability**:
  - Introduced `NOINLINE` cross-platform macro to fix Linux build errors caused by MSVC-specific `__declspec(noinline)`.

## [0.7.59] - 2026-02-19
### Fixed
- **FFB Safety Logic Refinement**: Removed the dependency on `gamePhase` from `IsFFBAllowed`. FFB now remains active even if the session is "stopped" or "over" in the game engine, provided the driver is still in control and not disqualified.
- **Test Integrity**:
  - Updated `test_ffb_safety.cpp` to align with the new session gating logic.
  - Fixed a logic error in `test_ffb_coverage_refactor.cpp` where `bottoming_crunch` results were incorrectly cleared right before assertions, causing false failures.
  - Restored missing method-level and internal comments in `FFBEngine.cpp` for improved maintainability.

## [0.7.58] - 2026-02-17
### Refactored
- **FFB Engine Split**:
  - Moved the entire implementation of `FFBEngine` from `src/FFBEngine.h` to a dedicated `src/FFBEngine.cpp` source file.
  - This improves build times, hides implementation details, and prevents namespace pollution.
  - Updated `CMakeLists.txt` to include the new source file.
  - Added `tests/test_ffb_coverage_refactor.cpp` to maintain 100% code coverage for the now-private implementation details.

## [0.7.57] - 2026-02-17
### Refactored
- **Ancillary Code Extraction**:
  - Extracted math utilities into `src/MathUtils.h` (namespace `ffb_math`).
  - Extracted performance statistics into `src/PerfStats.h`.
  - Extracted vehicle classification logic into `src/VehicleUtils.h` and `src/VehicleUtils.cpp`.
  - Refactored `FFBEngine.h` to use these new modular components, significantly reducing header complexity.
  - Updated all related unit tests to test these components directly, removing dependency on `FFBEngineTestAccess` for utility logic.

## [0.7.56] - 2026-02-17
### Fixed
- **100% Coverage for FFBEngine Core**:
  - Achieved full coverage for `ParseVehicleClass` and `calculate_slope_grip` torque fusion paths.
  - Used explicit boolean assignments to resolve 'ghost line' issues where coverage tools failed to attribute hits to complex conditional logic in headers.

## [0.7.55] - 2026-02-17
### Fixed
- **Code Coverage Visibility**:
  - Resolved an issue where `calculate_gyro_damping` and `calculate_abs_pulse` were reported as uncovered due to aggressive compiler inlining in Release builds.
  - Applied `__declspec(noinline)` to these methods to ensure correct line attribution in coverage reports.
- **Physics Test Hardening**:
  - Refined targeted coverage tests in `test_ffb_coverage_target.cpp` to include full engine integration loops, verifying both unit logic and previous-frame state updates.

## [0.7.54] - 2026-02-17
### Added
- **Targeted Coverage for Core Physics**:
  - Implemented exhaustive unit tests for `calculate_gyro_damping` and `calculate_abs_pulse` to achieve 100% coverage for these critical methods.
  - Verified all branches, including default steering range fallbacks, minimal smoothing tau, and ABS phase wrapping.
- **Build Optimization**:
  - Updated standard build commands to prioritize faster incremental builds while maintaining a clean build option.
  - Synchronized developer guidelines (`.gemini/GEMINI.md`, `NEW_AGENTS.md`) with the new optimized workflow.

## [0.7.53] - 2026-02-17
### Added
- **Integrated Coverage & Observability**:
  - Enhanced `test_coverage_integrated` with meaningful assertions to verify seed loads, gyro damping, and slope detection behavior.
  - Added `ffb_abs_pulse` to `FFBSnapshot` for real-time diagnostics and better unit testing of effect pulsing.
  - Achieved full coverage for `ParseVehicleClass` and `calculate_slope_grip` (torque fusion branch).
### Fixed
- **Build Warning Suppression**:
  - Resolved `C4244` (double to float conversion) warnings in `test_ffb_common.h`.
  - Added missing `ASSERT_FALSE` and `ASSERT_GT` macros to the test framework.

## [0.7.52] - 2026-02-17
### Added
- **Final Coverage Boost for FFBEngine**:
  - Implemented `test_coverage_integrated` in `test_coverage_boost.cpp` to close remaining gaps in `FFBEngine.h`.
  - Achieved full coverage for `ParseVehicleClass` (restricted/WEC branches) and `calculate_slope_grip` (torque fusion branch).
  - Ensured `calculate_gyro_damping` and `calculate_abs_pulse` are fully exercised through integrated engine loops.
### Fixed
- **Build Warning Suppression**:
  - Resolved `C4244` (double to float conversion) warnings in `test_ffb_common.h` by adjusting `CallApplySignalConditioning` to use consistent `double` types, matching the engine implementation.

## [0.7.51] - 2026-02-17
### Added
- **Ancillary Testing Safety Net**:
  - Implemented comprehensive unit tests for core math utilities, performance statistics, and vehicle metadata parsing.
  - This establishes a safety net for upcoming refactoring of the monolithic `FFBEngine.h`, ensuring utility functions remain stable as they are extracted into modular components.
### Refactored
- **FFBEngine Code Hygiene**:
  - Added Doxygen-style documentation to internal utility structures (e.g., `BiquadNotch`).
  - Exposed internal utility functions through `FFBEngineTestAccess` to enable direct unit testing of private/protected logic without full engine simulation.
### Testing
- **New Test Suites**:
  - `tests/test_math_utils.cpp`: Verifies Biquad filter stability, Savitzky-Golay derivative accuracy, and interpolation helpers (`inverse_lerp`, `smoothstep`).
  - `tests/test_perf_stats.cpp`: Validates performance tracking channels and reset behavior.
  - `tests/test_vehicle_utils.cpp`: Hardens vehicle class parsing and default load reference seeding.

## [0.7.49] - 2026-02-17
### Added
- **FFB Rate Monitoring & Verification (Issue #129)**:
  - Implemented a high-precision `RateMonitor` to track effective frequencies of FFB loop, telemetry updates, and hardware calls.
  - Added a new **System Health** section in the GUI displaying real-time Hz for all critical channels.
  - **Torque Source Selection**: Users can now choose between "Shaft Torque" (100Hz Legacy) and "Direct Torque" (400Hz LMU 1.2+). The direct stream provides much higher fidelity and is recommended for modern LMU versions.
  - **Precise Loop Timing**: Migrated the main FFB loop to a high-resolution `sleep_until` mechanism, stabilizing the update rate at exactly 400Hz regardless of DirectInput driver latency.
  - **Health Warnings**: The application now outputs console and logger warnings if it detects telemetry or torque rates dropping below the 400Hz target.

## [0.7.48] - 2026-02-15
### Fixed
- **FFB Persistence After Finish (Issue #126)**: Force Feedback now remains active during cool-down laps and after a DNF, provided the player retains vehicle control and the session is not officially over.
- **Reliability & Safety (Issue #79)**: Implemented a **Safety Slew Rate Limiter** to prevent violent wheel jolts during session transitions or control handovers.
  - **Dynamic Clamping**: Limits force rate-of-change to 1000 units/s normally, and a stricter 100 units/s during "restricted" phases (cool-down, lost control).
  - **NaN/Inf Protection**: Added rigorous mathematical sanitization for telemetry inputs and FFB outputs.
- **Hardware Zeroing**: Moved hardware update calls outside the connection conditional to ensure the wheel is always zeroed out when the game is disconnected or FFB is inactive.

### Improved
- **Effect Tuning**:
  - **Slide Texture**: Increased activation threshold from 0.5 m/s to 1.5 m/s to suppress vibration artifacts on straights caused by tire toe-in.
  - **Texture Load Cap**: Increased `texture_safe_max` from 2.0 to 10.0 to match the brake load cap, providing more headroom for road texture scaling.
- **Scrub Drag**: Decoupled the Scrub Drag effect from the main Road Texture toggle, allowing it to function independently if its gain is set.

### Testing
- **New Safety Suite**: Added test cases for the Slew Rate Limiter and updated `IsFFBAllowed` verification.
- **Issue Reproduction**: Added regression test for Issue #126.

## [0.7.47] - 2026-02-14
### Added
- **Dynamic Weight & Grip Smoothing**: Implemented advanced signal processing for tire load and grip telemetry.
  - **Adaptive Non-Linear Grip Filter**: Introduced an adaptive smoothing filter for estimated grip that provides high stability in steady states ($\tau \approx 50ms$) while maintaining zero-latency response during rapid grip loss transients ($\tau \approx 5ms$).
  - **Dynamic Weight LPF**: Implemented a standard Low Pass Filter for the steering weight modulation to simulate suspension damping and filter out road noise.
- **GUI Tuning Controls**: Added new sliders for Weight Smoothing and Grip Smoothing in the Tuning window.
- **Persistence**: Fully integrated new smoothing parameters into the Preset system and global configuration.

### Testing
- **Smoothing Verification Suite**: Added `tests/test_ffb_smoothing.cpp` to verify adaptive filter logic, weight LPF characteristics, and end-to-end integration.
- **Regression Guard**: Updated existing physics tests to handle the new smoothing state.

## [0.7.46] - 2026-02-14
### Added
- **Dynamic Weight**: Implemented master gain scaling based on front tire load transfer. Steering becomes heavier under braking and lighter under acceleration.
- **Load-Weighted Grip**: Refined grip estimation by weighting individual wheel grip by vertical load, prioritizing the feel of the loaded tire during cornering.

## [0.7.45] - 2026-02-14
### Refactored
- **Car Detection Infrastructure**: 
  - Introduced `ParsedVehicleClass` enum to centralize car categorization.
  - Refactored `InitializeLoadReference` into specialized helper methods: `ParseVehicleClass`, `GetDefaultLoadForClass`, and `VehicleClassToString`.
  - Decoupled detection logic from initialization, making it easier to add new car classes.
  - Added `LMP2 Unspecified` category to handle generic ORECA detections.

### Changed
- **Testing Integrity**: Updated unit tests in `test_ffb_smoothstep.cpp`, `test_ffb_road_texture.cpp`, and `test_ffb_features.cpp` to explicitly set the load reference, matching the new default of 4500N.

## [0.7.44] - 2026-02-14
### Improved
- **Vehicle Class Robustness**:
  - Implemented case-insensitive matching for car classes using `std::transform` and `toupper`.
  - Added a secondary fallback mechanism that uses keywords from `mVehicleName` (e.g., "499P", "ORECA", "GTE", "LMH", "LMDH") to infer car class when `mVehicleClass` is ambiguous or unknown.
  - Implemented hierarchical matching for LMP2, differentiating ELMS and restricted variants via name keywords.
  - Increased default load reference from 4000.0N to 4500.0N.

### Changed
- **FFB Integration API**:
  - Updated `calculate_force` signature to accept `vehicleName` for seeding logic.
  - Updated `main.cpp` telemetry loop to pass the vehicle name to the engine.

### Testing
- **New Regression Tests**:
  - Added `test_fallback_seeding` to `test_ffb_load_normalization.cpp` to verify name-based detection.
  - Updated existing `test_class_seeding` to cover case-insensitivity.

## [0.7.43] - 2026-02-14
### Added
- **Automatic Tire Load Normalization**:
  - Implemented an adaptive load scaling system that automatically adjusts FFB texture intensity based on vehicle capabilities.
  - **Class-Based Seeding**: On session start, the engine detects the vehicle class (GT3, GTE, LMP2, Hypercar, etc.) and sets an appropriate baseline (e.g., 9500N for Hypercars vs 4000N for GT3).
  - **Peak Hold Adaptation**: Continuously monitors tire loads. Actual loads exceeding the baseline trigger a "Fast Attack" update to prevent clipping, while a "Slow Decay" (~100N/s) maintains sensitivity.
  - **Improved Fidelity**: Prototype and high-downforce cars now maintain rich road texture and haptic detail at high speeds, previously lost to hardcoded 4000N normalization clipping.

### Improved
- **Telemetry Logging**: Updated `AsyncLogger` to include the `LoadPeakRef` channel, enabling detailed analysis of the dynamic normalization behavior in the Log Analyzer.

### Testing
- **Load Normalization Suite**: Added `tests/test_ffb_load_normalization.cpp` to verify class seeding, fast attack response, and slow decay stability.
- **Regression Guard**: Updated existing braking and texture tests to handle dynamic normalization by providing a toggle to freeze the reference during verification.

## [0.7.42] - 2026-02-14
### Fixed
- **Slope Detection Asymmetry**: Fixed a logic bug in the advanced Torque Slope calculation where left turns (negative torque) would not correctly contribute to grip loss detection. Both directions now behave symmetrically using absolute torque values.
- **Unified Thresholding**: Replaced hardcoded steering movement thresholds with the user-configurable `slope_alpha_threshold` for better consistency across different steering racks.

### Added
- **Configurable Confidence Limit**: Introduced `slope_confidence_max_rate` to allow users to tune the steering speed at which the slope detection effect reaches full strength. This is particularly beneficial for users with slower steering racks or smoother driving styles.

### Testing
- **Edge Case Verification**: Added `tests/test_ffb_slope_edge_cases.cpp` to verify directional symmetry, confidence tuning, and torque-based anticipation timing.

## [0.7.41] - 2026-02-14
### Added
- **Log Analyzer Tool Integration in Context Generator**:
  - Updated `scripts/create_context.py` to include a new `--include-log-analyzer` boolean option.
  - This allows users to optionally include the Python code for the log analyzer tool (located in `tools/lmuffb_log_analyzer/`) when generating the full project context for LLMs.
  - Added `--exclude-log-analyzer` for explicit exclusion (default behavior).
### Testing
- **Python Unit Tests**: Updated `tests/test_create_context_script.py` and `tests/test_create_context_extended.py` to verify the new flag logic and default behaviors.
- **Integration Verification**: Confirmed via manual runs and `grep` that files are correctly included/excluded based on the provided flags.


## [0.7.40] - 2026-02-13
### Added
- **Leading Indicator (Torque Slope)**: Implemented an anticipatory grip loss estimator based on Steering Torque vs. Steering Angle. This detects the peak of the Self-Aligning Torque (SAT) curve, providing haptic cues *before* the car physically slides.
- **Signal Hygiene (Slew Rate Limiter)**: Added a configurable slew rate limiter on Lateral G input to reject non-steering artifacts (e.g., curb strikes, suspension jolts) from the slope calculation.
- **Estimator Fusion**: Implemented a conservative fusion logic that combines G-based and Torque-based estimators, prioritizing the one that detects grip loss earliest.

### Testing
- **Advanced Slope Verification**: Added `tests/test_ffb_advanced_slope.cpp` to verify curb rejection and pneumatic trail anticipation.

## [0.7.39] - 2026-02-13
### Added
- **Surface Type Logging**: Added `SurfaceFL` and `SurfaceFR` channels to the telemetry log. This allows for precise filtering of noisy data (e.g., curb strikes, grass excursions) during offline analysis.
- **Phase Lag Analysis Spec**: Created technical specification for advanced Cross-Correlation analysis in the Log Analyzer to measure tire relaxation lag and optimize the SG filter window.

### Testing
- **Accuracy Tools Verification**: Added `tests/test_ffb_accuracy_tools.cpp` to verify surface type extraction and logging.

## [0.7.38] - 2026-02-13
### Fixed
- **Slope Detection Mathematical Stability**: Replaced the unstable division-based slope calculation with a robust "Projected Slope" method: `slope = (dG * dAlpha) / (dAlpha^2 + epsilon)`. This eliminates haptic "banging" and singularities when steering movement is near zero.
- **Steady-State Cornering (Hold-and-Decay)**: Implemented "Hold-and-Decay" logic to maintain understeer feel during long, steady-state corners. The engine now holds the last valid grip loss for 250ms after steering movement stops before gradually decaying.
- **Signal Hygiene**: Added input pre-smoothing (LPF, tau ~0.01s) for Lateral G and Slip Angle before they enter the derivative buffers, reducing high-frequency noise injection.

### Improved
- **Telemetry Logging**: Expanded the `AsyncLogger` to capture internal math states, including unclamped raw slope, numerator/denominator values, hold timer state, and smoothed inputs for better offline diagnostic analysis.

### Testing
- **Slope Fix Verification Suite**: Added `tests/test_ffb_slope_fix.cpp` to verify singularity rejection, hold-timer logic, and input smoothing attenuation.

## [0.7.37] - 2026-02-13
### Fixed
- **Slope Detection Threshold Disconnect (Issue #104)**: Unified the redundant slope threshold variables. The "Slope Threshold" slider in the GUI now correctly controls the physics engine by binding directly to `m_slope_min_threshold`. Removed the dead `m_slope_negative_threshold` variable.
- **Backward Compatibility**: Implemented migration logic to ensure that legacy configuration files using the `slope_negative_threshold` key are correctly mapped to the new unified variable on load.

### Testing
- **Slope Migration Regression Tests**: Added `tests/test_issue_104_slope_disconnect.cpp` to verify legacy key migration and new key persistence.
- **Updated Physics Tests**: Updated `test_ffb_slope_detection.cpp` and `test_persistence_v0625.cpp` to align with the unified variable naming.


## [0.7.36] - 2026-02-12
### Fixed
- **FFB Throttling Regression (Issue #100)**: Removed focus-based throttling in the main loop. The application now maintains a consistent 60Hz message loop even when minimized or backgrounded. This ensures that DirectInput background operations (specifically FFB updates) remain high-frequency and reliable regardless of window state.

### Testing
- **Timing Regression Test**: Added `tests/test_issue_100_timing.cpp` to verify consistent main loop frequency and GUI layer return values.

## [0.7.35] - 2026-02-12
### Fixed
- **ImGui ID Conflict**: Fixed "2 visible wheels with conflicting ID" error by using `PushID`/`PopID` in device and preset selection loops. This ensures unique identifier scopes for UI elements even when they share identical names. (#70)

### Testing
- **GUI Regression Test**: Added automated verification to ensure unique ImGui IDs are generated for identical labels when scoped with `PushID`.

## [0.7.34] - 2026-02-12
### Fixed
- **FFB Safety (Issue #79)**: Implemented automatic FFB muting when the car is no longer under player control (AI/Remote) or when the session has finished. This prevents violent "finish line jolts" during race weekends.
### Testing
- **New Safety Test Suite**: Added `tests/test_ffb_safety.cpp` to verify FFB lifecycle gating.

## [0.7.33] - 2026-02-12
### Fixed
- **Preset Handling**: Fixed issue where modifying any setting would immediately deselect the current preset and switch to "Custom", making "Save Current Config" non-functional (#72).
### Refactored
- **Dirty Detection**: Centralized preset comparison logic into a new `Preset::Equals` method and simplified `Config::IsEngineDirtyRelativeToPreset` (#71).

## [0.7.32] - 2026-02-11
### Improved
- **Linux Build & Test Parity**:
  - **Automated Dependency Management**: Fixed CMake configuration to automatically download and configure ImGui (v1.91.8) if not found locally using FetchContent. ImGui paths are now absolute, ensuring proper propagation to test subdirectories.
  - **Platform Abstraction**: Introduced `IGuiPlatform` interface with platform-specific implementations (`Win32GuiPlatform`, `LinuxGuiPlatform`) to separate business logic from windowing APIs (Win32 vs. GLFW/Headless). This improves maintainability and enables comprehensive testing on both platforms.
  - **Shared Memory Mocking**: Expanded `LinuxMock.h` to provide functional simulation of Windows memory-mapped files using `std::map`, enabling `GameConnector` and `SharedMemoryLock` logic to run and be tested on Linux.
  - **Build Fix**: Resolved CMake error "Cannot find source file: vendor/imgui/imgui.cpp" by ensuring `IMGUI_DIR` uses absolute paths from `CMAKE_SOURCE_DIR`.
  - **Test Coverage**: Enabled `test_windows_platform.cpp` on Linux builds with appropriate mocks, and ensured `test_game_connector_lifecycle` passes on both platforms.
### Testing
- **Cross-Platform Verification**: All 928 assertions and 197 test cases pass on Windows. Linux test infrastructure is now fully operational with the new mock layer. Correction: on Windows, there are 198 tests and 930 assertions, while on Linux there are 195 tests (3 less) and 928 assertions (2 less).

## [0.7.31] - 2026-02-11
### Improved
- **Linux Testing & Reporting Refactor**:
  - **Cross-Platform Logic Coverage**: Refactored the test suite to move platform-agnostic logic tests (Slider Precision, Config Persistence, Preset Management) from `test_windows_platform.cpp` to a new `test_ffb_logic.cpp`. This significantly increases Linux test coverage (~225 assertions previously skipped).
  - **Non-Invasive Linux Mocking**: Introduced `src/lmu_sm_interface/linux_mock/windows.h` to allow third-party ISI headers to compile on Linux without modification. Reverted all invasive platform guards in vendor headers.
  - **Enhanced Test Reporting**: The test runner now reports **Test Case counts** (Passed/Failed) in addition to individual assertions, providing a clearer high-level view of test health.
  - **Verification**: Confirmed all 197 test cases and 928 assertions pass on Windows, slightly exceeding the previous baseline due to improved depth in logic verification.

## [0.7.30] - 2026-02-11
### Improved
- **Single Source of Truth Versioning**:
  - Automated the versioning process using CMake. The `VERSION` file is now the single source of truth for the entire project.
  - Eliminated manual updates for `src/Version.h` and `src/res.rc`. These files are now automatically generated in the build directory during the configuration step.
  - Keeps the source tree clean and prevents desynchronization between binary metadata and application logic.
- **AMD Hardware Diagnostics (v0.7.29 Implementation)**:
  - Finalized the deployment of the persistent `Logger` system to help troubleshoot AMD driver crashes.

## [0.7.29] - 2026-02-11
### Added
- **Persistent Debug Logging**:
  - Implemented a high-reliability `Logger` system that writes critical system events to `lmuffb_debug.log`.
  - **Crash Resilience**: The log file is flushed to disk after every single line, ensuring diagnostic data is preserved even in the event of a full GPU driver hang or system reset.
  - **Hardware Instrumentation**: Added detailed logging for D3D11 device creation, swap chain resizing, and Win32 window management to isolate potential conflicts with AMD Adrenalin drivers.
- **Shared Memory Diagnostics**: Added detailed Windows error code logging for shared memory mapping failures.

## [0.7.28] - 2026-02-11
### Fixed
- **GUI Tooltip Restoration**:
  - Restored over 40 missing tooltips that were lost during recent GUI refactoring.
  - Added new descriptive tooltips for modern features including Slope Detection stability fixes, Lockup Prediction, and advanced signal filters.
  - **Comprehensive Coverage**: Verified via automated script and manual audit that 100% of interactive widgets (including utility buttons, checkboxes, and system controls) now have descriptive tooltips.
### Improved
- **Tooltip UX Enhancement**:
  - Tooltips now trigger when hovering over a widget's **Label**, not just the input field (slider/checkbox/combo). This makes discovering parameter info much more intuitive.
  - Standardized "Fine Tune" instructions across all sliders.
### Testing
- **GUI Regression Tests**:
  - Added `test_widgets_have_tooltips` to verify that core widget functions correctly handle and render tooltip strings.
  - Hardened `GuiWidgets` logic to prevent future regressions.

## [0.7.27] - 2026-02-11
### Security & Privacy
- **Heuristic Reduction**:
  - Replaced `OpenProcess` usage in `GameConnector` with `IsWindow` checks. This eliminates the "Process Access" behavior flag often targeted by antivirus heuristics.
### Testing
- **Security Verification**: Added automated tests to verify executable metadata integrity and `IsWindow` safety.

## [0.7.26] - 2026-02-11
### Security & Privacy
- **Executable Metadata**:
  - Added `VERSIONINFO` resource to the executable (`src/res.rc`).
  - The binary now includes standard metadata (Company Name, File Description, Version), which eliminates "anonymous file" heuristics used by antivirus software.
- **Build Hardening**:
  - Enabled `/GS` (Buffer Security Check), `/DYNAMICBASE` (ASLR), and `/NXCOMPAT` (DEP) flags for MSVC builds in `CMakeLists.txt`.

### Testing
- **Robustness**: Updated `test_game_connector_lifecycle` to gracefully handle pre-existing shared memory mappings (e.g., from running game instance), preventing false test failures.

## [0.7.25] - 2026-02-11
### Removed
- **vJoy Support**:
  - Removed vJoy support and dynamic library loading to eliminate security false positives and simplify the codebase.
  - Deleted `src/DynamicVJoy.h` and associated integration logic in `main.cpp`.
  - Removed vJoy-related configuration variables from `Config.h` and `Config.cpp`.
  - Cleaned up `test_config_runner.ini` and updated application version to `0.7.25`.

## [0.7.24] - 2026-02-11
### Security & Privacy
- **Disabled Clipboard Access**: Added build flag `IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS` to prevent ImGui from accessing the Windows clipboard. This removes a common antivirus heuristic trigger.
- **Removed Window Title Tracking**: Replaced `GetActiveWindowTitle()` logic (which used `GetForegroundWindow`) with a static string. This eliminates behavior that could be flagged as "spyware-like" activity monitoring.

## [0.7.23] - 2026-02-11
### Removed
- **Screenshot Feature**:
  - Removed "Save Screenshot" feature to improve application reputation with antivirus software (addressing false positives in Windows Defender and VirusTotal).
  - Cleaned up `GuiLayer` (Windows/Linux/Common) by removing screen capture logic and dependencies on `stb_image_write.h`.
  - Removed dedicated screenshot test suite.

## [0.7.22] - 2026-02-10
### Added
- **Linux Port (GLFW + OpenGL)**:
  - **Cross-Platform GUI**: Refactored the GUI layer to support both Windows (DirectX 11) and Linux (GLFW + OpenGL 3) backends.
  - **Maximized Testability**: Enabled GUI widget logic and configuration handling tests on Linux CI by splitting ImGui into Core and Backend components.
  - **Hardware Mocking**: Implemented Linux-specific mocks for DirectInput FFB, Game Connector (Shared Memory), and vJoy, enabling the application to run and be tested on Linux environments.
  - **Shared Memory Compatibility**: Updated shared memory interface wrappers and vendor headers to compile on Linux by guarding Windows-specific headers and providing dummy typedefs.
  - **Build System**: Enhanced `CMakeLists.txt` with conditional logic for platform-specific dependencies and ImGui backends. Added `HEADLESS_GUI` option for non-graphical environments.
  - **Headless Mode Support**: Improved headless mode robustness by providing stubs for all GUI-related platform calls.

## [0.7.21] - 2026-02-10
### Changed
- **Slope Detection Refinement (Full v0.7.17 Plan Compliance)**:
  - **Improved Mathematical Stability**: Replaced the binary derivative gate with a continuous `smoothstep` confidence ramp for grip loss scaling.
  - **Denominator Protection**: Implemented robust protection against division-by-small-number artifacts using a minimum epsilon for `dAlpha/dt`.
  - **Physically Constrained Slope**: Enforced a hard `std::clamp` on calculated slope values to the physically possible range [-20.0, 20.0].
  - **Smooth Transitions**: The `smoothstep` ramp ensures zero-derivative endpoints for seamless haptic transitions between cornering and straights.

## [0.7.20] - 2026-02-10
### Added
- **Slope Detection Hardening**:
  - **Numerical Stability Fixes**: Implemented a hard clamp on calculated raw slope values within [-20.0, 20.0] to prevent mathematical explosions near thresholds.
  - **Improved Confidence Ramp**: Updated `calculate_slope_confidence` to use a dedicated 0.01 to 0.10 rad/s `inverse_lerp` ramp, providing smoother transitions and rejecting singularity artifacts at low steering speeds.
### Testing
- **Comprehensive Hardening Suite**: Added 6 new test cases to `tests/test_ffb_slope_detection.cpp` covering:
  - `TestSlope_NearThreshold_Singularity`: Verifies clamping under extreme dG/dAlpha conditions.
  - `TestSlope_ZeroCrossing`: Ensures stability during direction changes.
  - `TestSlope_SmallSignals`: Validates noise rejection below active thresholds.
  - `TestSlope_ImpulseRejection`: Confirms smoothing of single-frame physics spikes.
  - `TestSlope_NoiseImmunity`: Statistically verifies slope stability under high-noise conditions.
  - `TestConfidenceRamp_Progressive`: Validates the smooth blending of the grip effect.
- **Verification**: All tests passing, bringing the total validated assertions to 962.

## [0.7.18] - 2026-02-09
### Added
- **Batch Processing for Log Analyzer**:
  - Implemented a new `batch` command in the `lmuffb_log_analyzer` tool.
  - Automatically processes all `.csv` logs in a directory, running `info`, `analyze`, `plots`, and `report` for each.
  - Generates a centralized results directory containing reports and diagnostic plots for all captured sessions.
### Testing
- **New Test Suite**: `tools/lmuffb_log_analyzer/tests/test_batch.py`
  - Validates end-to-end directory processing and file generation logic.

## [0.7.17] - 2026-02-07
### Changed
- **Test Suite Refactoring (Completion)**:
  - **Full Auto-Registration Migration**: Completed the transition of the entire test suite to the self-registering `TEST_CASE` macro (it was started with version 0.7.8). Legacy `static void` tests in `test_screenshot.cpp`, `test_windows_platform.cpp`, and persistence files have been fully migrated.
  - **Infrastructure Consolidation**: Centralized assertion macros (`ASSERT_EQ`, `ASSERT_EQ_STR`, etc.) and global test counters into `test_ffb_common.h` and `test_ffb_common.cpp`.
  - **Unified Test Runner**: Streamlined `main_test_runner.cpp` to rely solely on the automated registry. Removed redundant manual `Run()` calls and namespace forward declarations, ensuring all 867+ assertions contribute to a single, unified report.
  - **Code Cleanup**: Removed duplicated logic and redundant headers across the test suite, improving compilation overhead and maintainability.

## [0.7.16] - 2026-02-06
### Added
- **Stability and Safety Improvements**:
  - **FFB Heartbeat Watchdog**: Implemented telemetry staleness detection. If the game crashes or freezes (telemetry stops updating for > 100ms), FFB is automatically muted to prevent the wheel from being "stuck" with high forces.
  - **Comprehensive Parameter Validation**: Added rigorous safety clamping for all physical parameters in `Preset::Apply` and `Config::Load`. This prevents application crashes or `NaN` outputs caused by manually editing `config.ini` with invalid/negative values (e.g., negative `lockup_gamma`, zero `max_torque_ref`).
### Testing
- **New Stability Test Suite**: `tests/test_ffb_stability.cpp`
  - `test_negative_parameter_safety`: Verifies that invalid inputs are correctly clamped and do not cause crashes.
  - `test_config_load_validation`: Validates robust loading from malformed `.ini` files.
  - `test_engine_robustness_to_static_telemetry`: Ensures the engine handles frozen telemetry gracefully.

## [0.7.15] - 2026-02-06
### Fixed
- **Build Error**: Resolved compilation error C2513 in `Config.cpp` by renaming the local lambda `near` to `is_near` to avoid collision with legacy MSVC macros.

## [0.7.14] - 2026-02-05
### Added
- **Preset Handling Improvements**:
  - **Last Preset Persistence**: The app now remembers and automatically selects the last used preset on startup.
  - **Dirty State Indication**: Added a `*` suffix to the preset name in the GUI when settings have been modified but not saved.
  - **Improved "Save Current Config"**: Now updates the currently selected user preset instead of just saving to the top-level config.
  - **Preset Management**: Added "Delete" and "Duplicate" buttons to the GUI for easier preset management.
  - **Safety**: Built-in presets are protected from deletion.

## [0.7.13] - 2026-02-05
### Added
- **Preset Import/Export**:
  - Implemented standalone preset file I/O (.ini) for easy sharing of FFB configurations.
  - Added "Import Preset..." and "Export Selected..." buttons to the GUI with native Win32 file dialogs.
  - Robust name collision handling for imported presets (automatic renaming).
### Testing
- **New Test Suite**: `tests/test_ffb_import_export.cpp`
  - `test_preset_export_import`: Verifies single-preset round-trip integrity.
  - `test_import_name_collision`: Validates automatic renaming logic.

## [0.7.12] - 2026-02-05
### Added
- **Preset & Telemetry Versioning**:
  - **Preset Version Tracking**: Added `app_version` field to user presets to track compatibility.
  - **Legacy Migration**: Automated migration of presets missing version information to the current `0.7.12` version.
  - **Telemetry Version Header**: Injected application version into telemetry log headers for improved offline analysis.
- **Improved Config Persistence**:
  - Fixed a long-standing bug where `gain` was not being correctly loaded from user presets.
  - Implemented auto-save trigger when legacy presets are migrated during load.
### Testing
- **New Test Suite**: `tests/test_versioned_presets.cpp`
  - `test_preset_version_persistence`: Verifies version round-trip.
  - `test_legacy_preset_migration`: Validates automatic version injection for old presets.
- **Updated Test**: `tests/test_async_logger.cpp`
  - `test_logger_header_version_check`: Confirms the CSV header contains the correct version string.

## [0.7.11] - 2026-02-05
### Added
- **Slope Detection Min/Max Threshold Mapping**:
  - Implemented a more intuitive and powerful threshold system for slope-driven understeer.
  - **Min Threshold**: The dG/dAlpha slope value where the effect begins (dead zone edge). Defaults to -0.3.
  - **Max Threshold**: The slope value where the effect reaches maximum saturation. Defaults to -2.0.
  - **Linear Mapping**: Replaced the previous sensitivity-based exponential scaling with a predictable linear mapping between thresholds.
- **Improved InverseLerp Helper**: Added `inverse_lerp` utility with robust floating-point safety and support for both positive and negative ranges, optimized for tire slope physics.
- **MSVC Build Warning Mitigation**: Implemented explicit divide-by-zero protection in `inverse_lerp` and `smoothstep` to suppress compiler warning C4723.

### Fixed
- **Legacy Migration**: Implemented automatic migration logic that converts old `slope_sensitivity` values to the new min/max threshold model, preserving the user's intended feel on first launch.
- **Slope Logic Robustness**: Fixed edge cases where zero-range thresholds could cause FFB spikes or invalid calculations.

### Testing
- **New Test Cases**: Added comprehensive verification for the new threshold system.
  - `test_inverse_lerp_helper`: Validates math across normal, reversed, and tiny ranges.
  - `test_slope_minmax_dead_zone`: Confirms zero force reduction before the min threshold.
  - `test_slope_minmax_linear_response`: Verifies proportional force mapping.
  - `test_slope_minmax_saturation`: Ensures the effect saturates correctly.
  - `test_slope_sensitivity_migration`: Validates that legacy configs are correctly translated.
- **Regression**: All tests passing.

## [0.7.10] - 2026-02-04
### Added
- **Log Analyzer Tool**:
  - **Standalone Python Tool**: Created a comprehensive diagnostic tool in `tools/lmuffb_log_analyzer/` for processing telemetry logs.
  - **Advanced Diagnostics**: Analyzes slope stability, detects oscillation events, and identifies potential FFB issues (e.g., stuck understeer, frequent floor hits).
  - **Visualizations**: Generates multi-panel time-series plots, tire curves (Slip Angle vs Lat G), and dAlpha/dt histograms using Matplotlib.
  - **Automated Reporting**: Produces formatted text reports with session metadata, physics statistics, and prioritized issue lists.
  - **CLI Interface**: Includes `info`, `analyze`, `plots`, and `report` commands for flexible workflow integration.
- **Documentation**: 
  - Created detailed `README.md` for the Log Analyzer tool.
  - Linked the Log Analyzer in the main project `README.md`.
  - Updated `plan_log_analyzer.md` with implementation notes and final status.
### Testing
- **Unit Tests**: Implemented 6 new test suites in `tools/lmuffb_log_analyzer/tests/` covering parsing, analysis math, and plot generation.
- **Verification**: All 6 Log Analyzer tests passing; manual CLI end-to-end verification completed.

## [0.7.9] - 2026-02-04
### Added
- **Telemetry Logger Integration**:
  - **High-Performance Asynchronous Logging**: Implemented a double-buffered 100Hz CSV logger that captures 40+ physics channels without impacting FFB loop performance.
  - **FFB Engine Integration**: Captures raw telemetry, intermediate slope detection derivatives (dG/dt, dAlpha/dt), and final force components.
  - **GUI Controls**: Added manual Start/Stop/Marker buttons and a recording status indicator ("REC").
  - **Auto-Start Support**: Optional setting to automatically begin logging when entering a driving session.
  - **Persistence**: New settings for `log_path` and `auto_start_logging` saved to `config.ini`.
  - **Context-Aware Filenames**: Log files are automatically named using timestamp, car name, and track name.

## [0.7.8] - 2026-02-04
### Changed
- **Test Suite Refactoring**:
  - **Auto-Registration Pattern**: Migrated the entire test suite (591 tests across 12 files) to use the self-registering `TEST_CASE` macro.
  - **Eliminated "Fixed but not Called" Bug**: Tests are now automatically registered at startup, removing the need to manually add them to `Run_Category()` functions.
  - **Code Cleanup**: Removed all `Run_*` legacy runner functions from `test_ffb_common.cpp` and `test_ffb_common.h`.
  - **Verification**: All 591 tests passing.

## [0.7.7] - 2026-02-04
### Changed
- **Test Suite Refactoring**:
  - Split `test_ffb_features.cpp` further by extracting road texture tests.
  - **New File**: `test_ffb_road_texture.cpp` containing 8 tests:
    - Road texture regression tests (Toggle, Teleport, Persistence)
    - Suspension bottoming tests (Scrape, Spike, Universal)
    - Scrub drag fade tests
  - **Verification**: All 590 tests passing using the new `Run_RoadTexture()` runner.
  - `test_ffb_features.cpp` now focuses on Integration, Speed Gate, and Notch Filter tests.

## [0.7.6] - 2026-02-04
### Changed
- **Test Suite Refactoring**:
  - Split `test_ffb_features.cpp` into two files:
    - `test_ffb_lockup_braking.cpp`: Contains 8 tests related to lockup, ABS, and braking forces.
    - `test_ffb_features.cpp`: Retains remaining road texture and effect integration tests.
  - **Verification**: All 590 tests passing using the new `Run_LockupBraking()` runner.
  - **Organization**: Improved test navigability by grouping braking physics into a dedicated file.

## [0.7.5] - 2026-02-03
### Changed
- **Test Suite Refactoring**: 
  - Completed the modularization of the test suite. The `test_ffb_engine.cpp` file has been fully split into 9 modular files.
  - **Verification**: All tests are passing. The total test count is **591**, successfully restoring the 5 tests that were temporarily missing in v0.7.4.
  - **Restored Tests**:
    - `Rear Force Workaround active` assertion
    - `Rear slip angle` assertions (POSITIVE/NEGATIVE)
    - `Config Safety Validation` assertions for optimal slip ratio and small value resets
  - This completes the infrastructure refactoring with zero physics changes.

## [0.7.4] - 2026-02-03
### Changed
- **Test Suite Refactoring**: 
  - Split the monolithic `test_ffb_engine.cpp` (7,263 lines) into 9 modular category files (`test_ffb_core_physics`, `test_ffb_slip_grip`, etc.) for improved maintainability.
  - Introduced `test_ffb_common.h` and `test_ffb_common.cpp` for shared test infrastructure and helpers.
  - Incomplete implementation was pushed to git due to quota limits. Only 586 tests are currently present (5 less than the original 591).
  - Re-verified some of the regression tests (586?) to ensure physics integrity during the migration.

## [0.7.3] - 2026-02-03
### Fixed
- **Slope Detection Stability Fixes**:
  - **Decay on Straight**: Implemented a decay mechanism that returns the slope to zero when steering movement is below a threshold. This eliminates the "sticky" understeer feel where the wheel would remain light after exiting a corner.
  - **Configurability**: Added `slope_alpha_threshold` and `slope_decay_rate` parameters to allow user fine-tuning of stability vs. response time.
  - **Confidence Gate**: Introduced an optional confidence-based grip scaling. The grip loss effect is now scaled by the magnitude of `dAlpha/dt`, automatically smoothing out random FFB jolts during low-intensity maneuvers.
- **Physics Calculation Robustness**:
  - Added protection against large telemetry discontinuities (e.g., jumps from high slip to zero) which previously could cause garbage derivatives and "FFB explosions."
  - Improved Savitzky-Golay derivative calculations by enforcing a configurable minimum activity threshold (`m_slope_alpha_threshold`).

### Added
- **v0.7.3 Stability Controls**:
  - **Alpha Threshold**: Minimum `dAlpha/dt` (rad/s) required to update the slope calculation.
  - **Decay Rate**: Time constant for how fast the slope "forgets" the previous cornering state.
  - **Confidence Gate Toggle**: Enable/Disable the steering-speed based confidence scaling.
- **Verification Suite (v0.7.3)**:
  - `test_slope_decay_on_straight`: Verifies that `m_slope_current` returns to zero after cornering.
  - `test_slope_alpha_threshold_configurable`: Confirms the calculation only updates when steering movement is sufficient.
  - `test_slope_confidence_gate`: Validates the mathematical scaling of the grip effect based on `dAlpha/dt`.
  - `test_slope_stability_config_persistence`: Ensures new parameters are correctly saved to `config.ini` and presets.
  - `test_slope_no_understeer_on_straight_v073`: End-to-end physics test confirming zero force reduction when driving straight after a slide.
  - `test_slope_decay_rate_boundaries`: Verifies the impact of different decay rates on recovery speed.
  - `test_slope_alpha_threshold_validation`: Conforms that out-of-range parameters are clamped to safe defaults during loading.

### Changed
- **Config Persistence Logic**: Updated `Config::Save`, `Config::Load`, and `Preset` structures to include the new stability parameters, maintaining full backward compatibility.
- **GUI Organization**: Grouped the new stability controls under a dedicated "Stability Fixes (v0.7.3)" sub-header within the Physics section for better discoverability.

## [0.7.2] - 2026-02-03

### Added
- **Smoothstep Speed Gating**:
  - Replaced linear interpolation for speed-based FFB gating with a smooth Hermite S-curve (Smoothstep).
  - Implemented `smoothstep(edge0, edge1, x)` helper function in `FFBEngine` using polynomial `tÂ² * (3 - 2t)`.
  - Provides zero-derivative endpoints for a more natural "fade-in" of FFB forces at low speeds.
  - Improves the pit-lane and stationary-to-moving transition by eliminating the "angular" feel of linear scaling.
- **v0.7.2 Test Suite**:
  - `test_smoothstep_helper_function`: Verifies mathematical correctness of the Hermite polynomial.
  - `test_smoothstep_vs_linear`: Confirms the S-curve characteristic (above/below linear at different points).
  - `test_smoothstep_edge_cases`: Validates boundary safety (below/above thresholds) and zero-range handling.
  - `test_speed_gate_uses_smoothstep`: End-to-end physics test verifying the non-linear force ratio at intermediate speeds.
  - `test_smoothstep_stationary_silence_preserved`: Ensures no regression in stationary noise suppression.

### Changed
- **FFB Speed Gating Logic**: Refactored `calculate_force` to use the new `smoothstep` helper, simplifying the code while enhancing physical fidelity.

## [0.7.1] - 2026-02-02

### Fixed
- **Slope Detection Stability**:
  - Automatically disable **Lateral G Boost (Slide)** when Slope Detection is enabled. This prevents artificial grip-delta oscillations caused by asymmetric calculation methods between axles.
  - Updated default parameters for a more conservative and smoother experience on initial activation (Sensitivity: 0.5, Smoothing Tau: 0.04s).
- **GUI & UX Improvements**:
  - Added a dedicated **Slope (dG/dAlpha) Graph** in the Internal Physics section for real-time monitoring of the tire curve derivative.
  - Implemented detailed **Tuning Tooltips** for the SG Filter Window, providing hardware-specific recommendations for Direct Drive, Belt, and Gear-driven wheels.
  - Added a dashboard warning when both Slope Detection and Lateral G Boost are conceptually enabled, informing the user that the boost has been auto-suppressed.
- **Diagnostic Coverage**:
  - Populated the `slope_current` field in `FFBSnapshot` to ensure slope detection state is visible in telemetry logs and debugging tools.

### Added
- **v0.7.1 Regression Tests**:
  - `test_slope_detection_defaults_v071`: Verifies the new, less aggressive default parameters.
  - `test_slope_detection_diag_v071`: Confirms the slope diagnostic is correctly populated in snapshots.
  - `test_slope_detection_boost_disable_v071`: Ensures Lateral G Boost is properly suppressed when slope detection is active.
  - `test_slope_detection_less_aggressive_v071`: End-to-end physics test verifying the reduced sensitivity compared to v0.7.0.

## [0.7.0] - 2026-02-01

### Added
- **Dynamic Slope Detection (Tire Peak Grip Monitoring)**:
  - Implemented real-time dynamic grip estimation using Lateral G vs. Slip Angle slope monitoring.
  - Replaces static "Grip Loss" thresholds with an adaptive system that detects the tire's saturation point regardless of track conditions or car setups.
  - Added Savitzky-Golay (SG) filtering (window size 15-31) to calculate high-fidelity derivatives of noisy telemetry data.
  - **Front-Axle Specific**: Slope detection is applied exclusively to the front axle for precise understeer communication, while maintaining stability for rear grip effects.

### Fixed
- **Preset-Engine Synchronization**:
  - Fixed a critical regression where newly added parameters (optimal_slip_angle, slope_detection, etc.) were missing from `Preset::Apply()` and `Preset::UpdateFromEngine()`.
  - Resolved "Invalid optimal_slip_angle (0)" warnings that occurred when applying presets.
- **FFBEngine Initialization Order**:
  - Fixed circular dependency between `FFBEngine.h` and `Config.h` by moving the constructor to `Config.h`.

### Improved
- **Understeer Effect Fidelity**:
  - The Understeer Effect now utilizes the dynamic slope to determine force reduction, providing a much more organic and informative "light wheel" feel during front-end slides.
  - Added safety clamping (0.2 floor) and smoothing (configurable tau) to ensure stable feedback during rapid transitions.

### Added
- **Regression Test Suite**:
  - Added 11+ new tests covering slope detection math, noise rejection, SG filter coefficients, and Preset-Engine synchronization.
  - Added `test_preset_engine_sync_regression` to prevent future parameter synchronization omissions.


## Older Versions

For versions 0.6.x and older, see [CHANGELOG_ARCHIVE_v0.6.x_and_older.md](CHANGELOG_ARCHIVE_v0.6.x_and_older.md).
