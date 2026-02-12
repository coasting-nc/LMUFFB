# Changelog

All notable changes to this project will be documented in this file.

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
  - Implemented `smoothstep(edge0, edge1, x)` helper function in `FFBEngine` using polynomial `t² * (3 - 2t)`.
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

## [0.6.39] - 2026-01-31

**Special Thanks** to the community contributors for this release:
- **@AndersHogqvist** for the Auto-connect to LMU PR.

### Added
- **Auto-Connect to LMU**:
  - Implemented automatic connection logic that attempts to connect to LMU shared memory every 2 seconds when disconnected.
  - Added robust connection state management: detects if the game process exits and automatically resets the connection state.
  - **Improved UX**: The GUI now displays "Connecting to LMU..." in yellow while searching and "Connected to LMU" in green when active, eliminating the need for manual "Retry" clicks.
- **SafeSharedMemoryLock Wrapper** (`src/lmu_sm_interface/SafeSharedMemoryLock.h`):
  - Created wrapper class for vendor's `SharedMemoryLock` to add timeout support without modifying vendor code.
  - Follows the same pattern as `LmuSharedMemoryWrapper.h` (which adds missing includes).
  - **Benefit**: Avoids maintenance burden of modifying vendor files - easier to update when vendor releases new SDK versions.

### Optimized
- **FFB Loop Performance** (400Hz Critical Path):
  - Reduced lock acquisitions from **3 to 2 per frame** (33% reduction in mutex operations).
  - Modified `CopyTelemetry()` to return `bool` indicating realtime status instead of requiring separate `IsInRealtime()` call.
  - Eliminated redundant O(104) vehicle iteration from the FFB critical section.
  - **Impact**: 800 mutex operations/second (down from 1,200), improved responsiveness.

### Refactored
- **GameConnector Lifecycle**:
  - Introduced `Disconnect()` method to centralize resource cleanup (closing handles, unmapping memory views).
  - Fixed potential resource leaks in `TryConnect()` by ensuring cleanup before every connection attempt.
  - Updated `IsConnected()` with double-checked locking pattern for performance (atomic fast-path, mutex for thorough check).
  - **Process Handle Robustness**: Connection now succeeds even if window handle isn't immediately available or if `OpenProcess` fails, with informative logging.
  - Updated destructor to ensure all handles are properly closed on application exit.
- **Thread Safety**:
  - Added `std::mutex` to protect shared state between FFB thread (400Hz) and GUI thread (60Hz).
  - Added `std::atomic<bool>` for lock-free fast-path checks in `IsConnected()`.
  - All public methods now properly synchronized with appropriate locking strategies.

### Fixed
- **GUI Static Variable**: Moved `last_check_time` initialization outside conditional block to prevent redundant re-initialization every frame.
- **Test Suite**: Updated thread safety test to use new `CopyTelemetry()` return value API.

### Documentation
- **Vendor Code Tracking**: Created `docs/dev_docs/vendor_modifications.md` documenting known issues in vendor headers and our workaround strategies.
- **Implementation Summary**: Detailed review fix implementation in `docs/dev_docs/code_reviews/implementation_summary_v0.6.39_fixes.md`.

## [0.6.38] - 2026-01-31

**Special Thanks** to the community contributors for this release:
- **@DiSHTiX** for the LMU Plugin update PR.

### Fixed
- **LMU Plugin Update Build Break**: Fixed compilation errors in the updated `SharedMemoryInterface.hpp` by creating a header wrapper (`LmuSharedMemoryWrapper.h`).
  - **Wrapper Approach**: Instead of editing the official vendor files provided by Studio 397, we now include the missing standard library headers (`<optional>`, `<utility>`, `<cstdint>`, `<cstring>`) in our wrapper file before including the official header.
  - **Benefit**: This approach preserves the integrity of official source files, making future plugin updates easier to integrate and reducing maintenance burden.
  - **Compatibility**: Ensures full compatibility with the new 2025 plugin interface (LMU 1.2/1.3 standards).

### Changed
- **LMU Plugin Interface**: Updated `InternalsPlugin.hpp` and `PluginObjects.hpp` to the latest 2025 version, aligning with LMU 1.2/1.3 standards.

## [0.6.37] - 2026-01-31

**Special Thanks** to the community contributors for this release:
- **@MartinVindis** for designing and providing the application icon.
- **@DiSHTiX** for the pull request and implementation logic to integrate the icon.

### Added
- **Application Icon**: Added a custom application icon (`lmuffb.ico`) to the executable.
  - The icon is now embedded in the Windows executable and displayed in Explorer and the Taskbar.
  - Added build system support (`CMakeLists.txt`) to compile and link the resource file.
  - **Robust Build Verification**: Added `tests/test_icon_presence` to ensure the icon is correctly staged.
    - **Path Agnostic**: Uses the Windows API to locate the build artifact relative to the running executable, ensuring the test passes regardless of the working directory.
    - **Data Integrity**: Inspects the icon file's binary header (0x00000100) to verify it is a valid `.ico` file and not an empty placeholder.

## [0.6.36] - 2026-01-05
### Refactored
- **FFB Engine Architecture**: Massive refactoring of `FFBEngine::calculate_force` to improve maintainability and scalability.
  - **Context-Based Processing**: Introduced `FFBCalculationContext` struct to pass derived values (speed, load, dt) efficiently between methods.
  - **Modular Helper Methods**: Extracted monolithic logic into focused private methods (`calculate_sop_lateral`, `calculate_gyro_damping`, `calculate_abs_pulse`, etc.).
  - **Improved Readability**: Significantly reduced the complexity of the main calculation loop.

### Fixed
- **Torque Drop Logic Regression**: Fixed a critical issue where the "Torque Drop" (Spin Gain Reduction) was incorrectly attenuating texture effects (Road, Slide, Spin, Bottoming).
  - **Restored Behavior**: Torque Drop now ONLY applies to "Structural" forces (Base, SoP, Rear Torque, Yaw, Gyro, ABS, Lockup, Scrub). Texture forces are added *after* the drop, ensuring vibrations remain distinct even during traction loss (drifting/burnouts).
- **Telemetry Snapshot Regression**: Fixed `sop_force` in debug snapshots incorrectly including the oversteer boost component.
  - **Restored Behavior**: Snapshots now correctly report the unboosted lateral force for `sop_force` and the boost delta for `oversteer_boost`, enabling accurate debugging of the SoP pipeline.
- **ABS Pulse Summation**: Fixed a logic error where the ABS pulse force was not being added to the final FFB sum in some scenarios.

### Code Review Follow-up (Additional Improvements)
- **`calculate_wheel_slip_ratio` Helper**: Extracted duplicated `get_slip` lambda from `calculate_lockup_vibration` and `calculate_wheel_spin` into a unified public helper method. Reduces code duplication and improves testability.
- **`apply_signal_conditioning` Method**: Extracted ~70 lines of signal conditioning logic (idle smoothing, frequency estimation, dynamic/static notch filters) into a dedicated public helper method. Makes the main `calculate_force` method a cleaner high-level pipeline.
- **Unconditional State Update Fix**: Moved `m_prev_vert_accel` update from inside `calculate_road_texture` (conditional) to the unconditional state updates section at the end of `calculate_force`. Prevents stale data issues when road texture is disabled.
- **Build Warning Fixes**: Fixed MSVC warnings C4996 (strncpy unsafe) and C4305 (double-to-float truncation) in test files.
- **New Tests**: Added 8 new regression tests for the extracted helper methods (483 total tests, 0 failures).

## [0.6.35] - 2026-01-04
### Added
- **Three New DD Presets**:
  - **GT3 DD 15 Nm (Simagic Alpha)**: Optimized for GT3 racing with sharp, responsive feedback. Features balanced effects with moderate smoothing for maximum communication of grip changes. Ideal for GT3, GT4, and touring cars.
  - **LMPx/HY DD 15 Nm (Simagic Alpha)**: Optimized for LMP prototypes and hypercars with smooth, refined feedback for high-speed stability. Features increased smoothing parameters (yaw, gyro, slip, chassis) and higher optimal slip angle (0.12 rad) for high-downforce racing. Ideal for endurance racing and high-speed circuits.
  - **GM DD 21 Nm (Moza R21 Ultra)**: "Steering Shaft Purist" preset emphasizing raw mechanical torque over computed effects. Features high master gain (1.454), nearly 2x steering shaft gain (1.989), minimal SoP (0.29), disabled oversteer boost/rear align/yaw kick, strong lockup feedback (0.977), and zero smoothing on SoP/chassis. Represents a fundamentally different FFB philosophy for drivers seeking pure mechanical feel.

### Documentation
- **Preset Comparison Report**: Created comprehensive analysis document `docs/dev_docs/preset_comparison_gt3_vs_lmpx.md` comparing all three presets:
  - Detailed parameter-by-parameter comparison table
  - FFB character analysis for each preset
  - Migration guides for switching between presets
  - Technical deep dive into the "Steering Shaft Purist" philosophy
  - Use case recommendations for each racing category
  - Screenshot analysis confirming GM preset consistency

### Technical Details
- **Preset Differences**:
  - GT3 vs LMPx/HY: Differ in 7 smoothing/physics parameters (11.7% of total)
  - GM vs GT3/LMPx: Differs in 12+ parameters (20% of total), representing a different FFB paradigm
  - All three presets share 38 identical parameters (63.3% of total)
- **Key GM Preset Characteristics**:
  - Combined torque output: ~2.9x stronger than GT3/LMPx (1.454 gain × 1.989 shaft gain)
  - Flatspot suppression enabled (unique among the three)
  - Brake load cap: 81.0 (vs 2.0 for GT3/LMPx) for unrestricted lockup feedback
  - Philosophy: Maximize direct torque, minimize computed effects, zero smoothing

## [0.6.34] - 2026-01-02
### Changed
- **Preset Naming**: Renamed "Default (T300)" preset to "Default" to reflect that it now has different settings from the T300 preset (which was decoupled in v0.6.30).
  - Updated all test files and documentation to reference the new preset name
  - The "Default" preset continues to use the Preset struct defaults from `Config.h` as the single source of truth

## [0.6.33] - 2026-01-02
### Fixed
- **Negative Speed Gate Display in "Test: Understeer Only" Preset**:
  - Fixed confusing negative km/h values in GUI (-36.0 km/h, -18.0 km/h) caused by negative speed gate values.
  - Changed speed gate from `.SetSpeedGate(-10.0f, -5.0f)` to `.SetSpeedGate(0.0f, 0.0f)`.
  - GUI now correctly displays "0.0 km/h" for both thresholds, indicating the speed gate is disabled.

### Added
- **Regression Test**: Added `test_all_presets_non_negative_speed_gate()` to prevent negative speed gate values in any preset:
  - Checks all presets for non-negative `speed_gate_lower` and `speed_gate_upper`
  - Verifies `upper >= lower` (sanity check)
  - Prevents confusing negative km/h displays in GUI
  - Automatically validates all current and future presets

### Documentation
- **Test Documentation**: Created `docs/dev_docs/test_all_presets_non_negative_speed_gate.md` with detailed explanation of the issue, fix, and test behavior.

## [0.6.32] - 2026-01-02
### Fixed
- **"Test: Understeer Only" Preset Isolation**:
  - Completely overhauled the preset to ensure proper effect isolation for diagnostic testing.
  - **Fixed Contamination Issues**: Explicitly disabled all non-understeer effects (lockup vibration, ABS pulse, road texture, oversteer boost, yaw kick, gyro damping) that were previously active due to inherited defaults.
  - **Explicit Physics Parameters**: Added explicit settings for `optimal_slip_angle` (0.10), `optimal_slip_ratio` (0.12), `base_force_mode` (0), and disabled speed gate for complete testing control.
  - **Impact**: The preset now provides a clean, isolated test environment for the understeer effect without interference from other FFB systems.

### Added
- **Regression Test**: Added `test_preset_understeer_only_isolation()` with 17 assertions to verify proper effect isolation:
  - Verifies primary effect is enabled (1 check)
  - Verifies all other effects are disabled (6 checks)
  - Verifies all textures are disabled (5 checks)
  - Verifies critical physics parameters are correct (5 checks)

### Documentation
- **Preset Review**: Created `docs/dev_docs/preset_review_understeer_only.md` with comprehensive analysis of all 50+ preset parameters, identifying missing settings and providing implementation recommendations.
- **Test Documentation**: Created `docs/dev_docs/test_preset_understeer_only_isolation.md` with detailed test rationale, historical context, and maintenance guidelines.

## [0.6.31] - 2026-01-02
### Added
- **Understeer Effect Improvements**:
  - **BREAKING CHANGE - Rescaled Range**: Changed the "Understeer Effect" slider range from **0-200** to **0.0-2.0** for improved usability and precision.
    - **Why**: The old 0-200 range had 99.5% of its values unusable. Mathematical analysis showed the useful range is 0.0-2.0, where values above 2.0 cause near-instant force elimination.
    - **Migration**: Automatic migration logic converts legacy values (e.g., 50.0 → 0.5) when loading old config files.
    - **New Scale Guide**:
      - `0.0` = Disabled (no understeer effect)
      - `0.5` = Subtle (50% of grip loss reflected)
      - `1.0` = Normal (force matches grip) — **New Default**
      - `1.5` = Aggressive (amplified response)
      - `2.0` = Maximum (very light wheel on any slide)
  - **Refined T300 Physics**: Increased default `optimal_slip_angle` from 0.06 to 0.10 rad in the T300 preset. This provides a larger "buffer zone" before grip loss begins, addressing user reports of the wheel feeling too light too early.
  - **Enhanced UI Tooltips**: Overhauled the tooltips for "Understeer Effect" and "Optimal Slip Angle" to provide clearer guidance on when and how to adjust these settings. Added a specific "When to Adjust" guide and a scale guide.
  - **Percentage Formatting**: Updated the "Understeer Effect" slider to display as a percentage (0-200%) for better consistency with other gain settings.
  - **Regression Test Suite**: Added 7 comprehensive unit tests in `test_ffb_engine.cpp` to verify understeer physics:
    - `test_optimal_slip_buffer_zone`: Verifies no force loss below optimal slip threshold
    - `test_progressive_loss_curve`: Verifies smooth, progressive grip loss beyond threshold
    - `test_grip_floor_clamp`: Verifies grip never drops below safety floor (0.2)
    - `test_understeer_output_clamp`: Verifies force clamps to 0.0 (never negative) at maximum effect
    - `test_understeer_range_validation`: Verifies new 0.0-2.0 range enforcement
    - `test_understeer_effect_scaling`: Verifies effect properly scales force output
    - `test_legacy_config_migration`: Verifies automatic migration of legacy 0-200 values

### Code Quality
- **Code Review Implementation**:
  - **Preset Migration Logging**: Added console logging when migrating legacy understeer values in preset loading (matching main config behavior).
  - **Test Constants**: Added `FILTER_SETTLING_FRAMES = 40` constant in test suite for better maintainability.
  - **Test Isolation Documentation**: Added comprehensive warning comment in `InitializeEngine()` for future test authors about breaking changes in default values.
  - **Grip Floor Documentation**: Enhanced documentation of the 0.2 grip floor safety clamp in `test_grip_floor_clamp()`.
  - **Config Versioning Documentation**: Added detailed comments explaining how `ini_version` serves as both app version tracker and implicit config format version.


## [0.6.30] - 2026-01-01
### Changed
- **T300 Preset Refinement**:
  - Decoupled the "T300" preset from the hardcoded "Default" preset. The T300 preset now uses specific optimized values for improved force feedback fidelity on T300 wheelbases.
  - Optimized default parameters for T300: reduced latency (`steering_shaft_smoothing=0`), specific notch filter settings (`notch_q=2`), and adjusted effects gains (`understeer=0.5`, `sop=0.425`).
- **User Experience**:
  - Removed the persistent console success message `[Config] Saved to config.ini` to reduce console spam during auto-save operations.

## [0.6.29] - 2025-12-31
### Added
- **Config File Structure Reordering**:
  - Reordered `config.ini` file structure to mirror GUI hierarchy for improved readability.
  - Added comment headers (e.g., `; --- System & Window ---`, `; --- General FFB ---`) to organize settings into logical sections.
  - Settings now save in this order: System & Window → General FFB → Front Axle → Rear Axle → Physics → Braking & Lockup → Tactile Textures → Advanced Settings.
  - Fixed critical bug where `Config::Load` would overwrite main configuration with preset values by stopping parsing at `[Presets]` section.
  - User presets also follow the same reordered structure for consistency.
  - Maintained backward compatibility with legacy config keys (`smoothing`, `max_load_factor`).
- **Enhanced Persistence Test Suite**:
  - Added comprehensive `test_persistence_v0628.cpp` with 16 new tests covering config reordering, section isolation, legacy support, and comment structure validation.

## [0.6.28] - 2025-12-31
### Added
- **Test Sandbox & Artifact Cleanup**:
  - Implemented a "Sandboxed" test environment that redirects all configuration file I/O to temporary files, preventing tests from overwriting user's `config.ini`.
  - Added automatic artifact cleanup at the end of the test runner to remove all temporary `.ini` files generated during execution.
  - Suppressed `imgui.ini` creation during headless GUI tests.
- **Configurable Config Path**:
  - Updated `Config` class to support a configurable file path (`m_config_path`), allowing the application to load/save settings from arbitrary locations.

## [0.6.27] - 2025-12-31
### Added
- **Reactive Auto-Save**:
  - Implemented automatic persistence of all GUI adjustments. Settings are now saved to `config.ini` the moment an interaction is completed (e.g., releasing a slider or toggling a checkbox).
  - Added auto-save to **Load Preset** actions, ensuring that applying a preset persists it as the current configuration for future sessions.
  - Added auto-save to "Always on Top" and "Stationary Vibration Gate" (Speed Gate) settings.
- **Unified UI Widget Library (`GuiWidgets.h`)**:
  - Extracted UI logic into a reusable library, standardizing behavior (Arrow Keys, Tooltips, Auto-Save) across all controls.
  - Introduced "Decorators" support for sliders, allowing complex info like Latency indicators to be cleanly integrated without code duplication.
- **Automated UI Interaction Tests**:
  - Added `GuiInteractionTests` to verify widget logic, deactivation flags, and decorator execution in a headless environment.

### Changed
- Refactored `src/GuiLayer.cpp` to use the unified `GuiWidgets` library, significantly reducing lines of code and improving maintainability of the Tuning Window.
- Optimized Disk I/O by using deactivation triggers, preventing "thrashing" during real-time slider drags.

## [0.6.26] - 2025-12-31
### Fixed
- **Remaining Low-Speed Vibrations (SoP & Base Torque)**:
  - Extended the "Stationary Vibration Gate" (Speed Gate) to apply to **Base Torque** (Physics) and **SoP (Seat of Pants)** effects.
  - This ensures a completely silent and still steering wheel when the car is stationary or moving at very low speeds (< 5 km/h), eliminating "engine rumble" and noisy sensor data at idle.
  - Added safe thresholds and ramp-up logic to smoothly fade in steering weight as the car begins moving.

### Added
- **Improved Test Coverage**:
  - Added `test_stationary_silence()` to verify all forces are muted at a car speed of 0.0 m/s, even with high noise injected into physics channels.
  - Added `test_driving_forces_restored()` to verify FFB is fully active at driving speeds.
  - Updated legacy test infrastructure (`InitializeEngine`) to ensure physics tests remain valid while speed gating is active.

## [0.6.25] - 2025-12-31
### Added
- **Configuration Versioning**:
  - Implemented `ini_version` field in config files to track which version of LMUFFB created the configuration.
  - Enables future migration logic for handling breaking changes in configuration format.
  - Version is automatically written when saving and logged when loading for diagnostics.

- **Complete Persistence for v0.6.23 Features**:
  - **Speed Gate Persistence**: Added full save/load support for `speed_gate_lower` and `speed_gate_upper` in both main configuration and user presets.
  - **Advanced Physics Settings**: Added persistence for `road_fallback_scale` and `understeer_affects_sop` (reserved for future implementation).
  - **Texture Load Cap**: Completed implementation of `texture_load_cap` persistence in preset system (was partially implemented in v0.6.23).

- **Comprehensive Test Suite** (10 new tests, 414 total passing):
  - **Test 1**: Texture Load Cap in Presets - Verifies preset serialization of texture_load_cap field.
  - **Test 2**: Main Config Speed Gate Persistence - Validates save/load round-trip for speed gate thresholds.
  - **Test 3**: Main Config Advanced Physics - Tests road_fallback_scale and understeer_affects_sop persistence.
  - **Test 4**: Preset Serialization - All New Fields - Comprehensive test of all v0.6.25 fields in user presets.
  - **Test 5-6**: Preset Clamping Regression Tests - Ensures brake_load_cap and lockup_gain are NOT clamped during preset loading (preserving user intent).
  - **Test 7-8**: Main Config Clamping Regression Tests - Verifies safety clamping (1.0-10.0 for brake_load_cap, 0.0-3.0 for lockup_gain) during main config loading.
  - **Test 9**: Configuration Versioning - Validates ini_version is written and read correctly.
  - **Test 10**: Comprehensive Round-Trip Test - End-to-end validation of all persistence mechanisms (main config → preset → load → apply).

### Fixed
- **Test Isolation Bug**: Fixed test failures caused by preset pollution between tests.
  - **Root Cause**: `Config::Save()` writes both main configuration AND all user presets to the file. When tests ran sequentially, presets created in Test 1 (with default values for new fields) were being saved alongside Test 2's main config, causing the default values to overwrite the test values during loading.
  - **Solution**: Added `Config::presets.clear()` at the beginning of Tests 2, 3, and 9 to ensure clean test isolation.
  - **Impact**: All 22 previously failing tests now pass (414 total tests passing, 0 failures).

### Changed
- **Config.h/Config.cpp**:
  - Added `speed_gate_lower`, `speed_gate_upper`, `road_fallback_scale`, and `understeer_affects_sop` to both main config save/load and preset serialization.
  - Added backward compatibility for legacy `max_load_factor` → `texture_load_cap` migration.
  - Ensured all new fields are properly initialized with Preset struct defaults.

### Technical Details
- **Preset System Enhancement**: The `Preset` struct now includes all v0.6.23+ fields with proper defaults:
  - `speed_gate_lower = 1.0f` (3.6 km/h)
  - `speed_gate_upper = 5.0f` (18.0 km/h)
  - `road_fallback_scale = 0.05f`
  - `understeer_affects_sop = false`
- **Test File**: Added `tests/test_persistence_v0625.cpp` with comprehensive coverage of all persistence mechanisms.
- **No Breaking Changes**: Existing configurations load seamlessly. New fields default to safe values if not present in older config files.

## [0.6.24] - 2025-12-28
### Changed
- **Max Torque Ref Documentation Update**:
  - **Updated Tooltip**: Clarified that Max Torque Ref represents the expected PEAK torque of the CAR in the game (30-60 Nm for GT3/LMP2), NOT the wheelbase capability.
  - **New Guidance**: Added clear explanation of the tradeoff between clipping and steering weight:
    - Higher values = Less Clipping, Less Noise, Lighter Steering
    - Lower values = More Clipping, More Noise, Heavier Steering
  - **Recommended Range**: Set this to ~40-60 Nm to prevent clipping for modern race cars.
  - **README Tuning Tip**: Added troubleshooting entry explaining that Max Torque Ref is the primary way to fix violent oscillations if Smoothing/Gate settings don't catch them.
  - **Default Value**: Maintained at 100.0 Nm for T300 compatibility (existing users' configs unchanged).

## [0.6.23] - 2025-12-28
### Added
- **Configurable Speed Gate**:
  - Introduced the **"Stationary Vibration Gate"** in Advanced Settings, allowing manual control over where vibrations fade out.
  - Added **"Mute Below"** (0-20 km/h) and **"Full Above"** (1-50 km/h) sliders to tune the transition between idle smoothing and full FFB.
  - Implemented safety clamping to ensure the upper threshold always remains above the lower threshold.
- **Improved Idle Shaking Elimination**:
  - Increased the default speed gate to **18.0 km/h (5.0 m/s)**. This ensures that the violent engine vibrations common in LMU/rF2 below 15 km/h are surgically smoothed out by default.
  - Updated the automatic idle smoothing logic to utilize the user-configured thresholds with a 3.0 m/s safety floor.
- **Advanced Physics Configuration**:
  - Added support for `road_fallback_scale` and `understeer_affects_sop` settings in the `Preset` system and FFB engine.
- **Improved Test Coverage**:
  - Added `test_speed_gate_custom_thresholds()` to verify dynamic threshold scaling and default initializations.
  - Updated `test_stationary_gate()` to align with the new 5.0 m/s default speed gate.

## [0.6.22] - 2025-12-28
### Added
- **Automatic Idle Smoothing**:
  - Implemented a dynamic Low Pass Filter (LPF) for the steering shaft torque that automatically increases smoothing when the car is stationary or moving slowly (< 3.0 m/s).
  - This surgically removes high-frequency engine vibration (idle "buzz") while preserving the heavy static weight required to turn the wheel at a standstill.
  - The smoothing gracefully fades out as speed increases, returning to the user-defined raw setting by 10 kph.
- **Improved Test Coverage**:
  - Added `test_idle_smoothing()` to verify vibration attenuation at idle and raw pass-through while driving.

## [0.6.21] - 2025-12-28
### Added
- **Stationary Signal Gate**:
  - Implemented a "Speed Gate" that automatically fades out high-frequency vibration effects (Road Texture, ABS Pulse, Lockup, and Bottoming) when the car is stationary or moving at very low speeds (< 2.0 m/s). Ramp from 0.0 vibrations (at < 0.5 m/s) to 1.0 vibrations (at > 2.0 m/s).
  - This eliminates the "Violent Shaking at Stop" issue caused by engine idle vibrations and sensor noise being amplified while the car is in the pits or parked.
- **Road Texture Fallback (Encrypted Content Support)**:
  - Implemented a "Vertical G-Force" fallback mechanism for Road Texture specifically for DLC/Encrypted cars where suspension telemetry is blocked by the game.
  - The engine now automatically detects "dead" deflection signals while moving fast (> 5.0 m/s) and switches to using **Vertical Acceleration** (`mLocalAccel.y`) to generate road noise, ensuring bumps and curbs are felt on all cars.
- **Improved Telemetry Diagnostics**:
  - Added native detection for missing `mVerticalTireDeflection` data with hysteresis.
  - Unified all missing telemetry warnings (`mTireLoad`, `mGripFract`, `mSuspForce`, etc.) to explicitly include **"(Likely Encrypted/DLC Content)"**, helping users identify why fallback logic is active.
- **Improved Test Coverage**:
  - Added `test_stationary_gate()` and updated `test_missing_telemetry_warnings()` to verify the new vibration suppression and deflection diagnostic logic.

### Changed
- **Warning Clarity**: Updated the `mTireLoad` missing data warning to explicitly mention "(Likely Encrypted/DLC Content)" to help users understand why the kinematic fallback is being used.

## [0.6.20] - 2025-12-27
### Added
- **Effect Tuning & Slider Range Expansion**:
  - **ABS Pulse Frequency**: Added a dedicated slider (10Hz - 50Hz) to tune the vibrational pitch of the ABS pulse effect, allowing users to match the haptic feel of their specific hardware.
  - **Vibration Pitch Tuning**: Added "Vibration Pitch" sliders for both **Lockup** and **Wheel Spin** vibrations (0.5x - 2.0x). Users can now customize the "screech" or "judder" characteristic of these effects.
  - **Expanded Slider Ranges**: Significant range increases for professional-grade hardware and extreme feedback scenarios:
    - **Understeer Effect**: Max increased to 200% (was 50%).
    - **Steering Shaft Gain**: Max increased to 2.0x (was 1.0x).
    - **ABS Pulse Gain**: Max increased to 10.0x (was 2.0x).
    - **Lockup Strength**: Max increased to 3.0x (was 2.0x).
    - **Brake Load Cap**: Max increased to 10.0x (was 3.0x).
    - **Lockup Prediction Sensitivity**: Min threshold lowered to 10.0 (more sensitive).
    - **Lockup Rear Boost**: Max increased to 10.0x (was 3.0x).
    - **Lateral G Boost**: Max increased to 4.0x (was 2.0x).
    - **Lockup Gamma**: Range expanded to 0.1 - 3.0 for ultra-fine onset control.
    - **Yaw Kick Gain**: Consolidated max to 1.0 (optimized for noise immunity).

### Changed
- **Core Logic Cleanup**:
  - **Removed "Manual Slip" Toggle**: The engine now always uses the most accurate native telemetry data for slip calculations. The manual calculation fallback remains as an automatic internal recovery mechanism for encrypted content.
  - **Unified Frequency Math**: Synchronized all vibration oscillators to use time-corrected phase accumulation for perfect stability during frame stutters.
- **Documentation**:
  - Updated **FFB_formulas.md** and **Reference - telemetry_data_reference.md** to reflect the new frequency tuning math and expanded physics ranges.

### Fixed
- **Test Suite Alignment**: Resolved all regression test failures caused by the removal of the manual slip toggle and the expansion of safety clamping limits.

## [0.6.10] - 2025-12-27
### Added
- **Signal Processing Improvements**:
  - **Dynamic Static Notch Filter**: Replaced the fixed Q-factor notch filter with a variable bandwidth filter. Users can now adjust the "Filter Width" (0.1 to 10.0 Hz) to surgically suppress hardware resonance or floor noise.
  - **Adjustable Yaw Kick Threshold**: Implemented a user-configurable activation threshold (0.0 to 10.0 rad/s²) for the Yaw Kick effect. This allows users to filter out micro-corrections and road noise while maintaining sharp reaction cues for actual car rotation.
- **GUI Enhanced Controls**:
  - Added "Filter Width" slider to the Signal Filtering section.
  - Added "Activation Threshold" slider to the Yaw Kick effect section for better noise immunity tuning.
- **Improved Test Coverage**:
  - Added `test_notch_filter_bandwidth()` and `test_yaw_kick_threshold()` to the physics verification suite.
  - Added `test_notch_filter_edge_cases()` and `test_yaw_kick_edge_cases()` for comprehensive edge case validation.
### Changed
- **Default Static Notch Frequency**: Changed from 50.0 Hz to 11.0 Hz to better target the 10-12 Hz baseline vibration range identified in user feedback.

## [0.6.9] - 2025-12-26
### Changed
- **GUI Label Refinements**:
  - Renamed "SoP Lateral G" to **"Lateral G"** for clarity and conciseness.
  - Renamed "Rear Align Torque" to **"SoP Self-Aligning Torque"** to better reflect the physical phenomenon (Self-Aligning Torque) being simulated by the Seat of Pants (SoP) model.

## [0.6.8] - 2025-12-26
### Documentation
- **Troubleshooting Guide**: Expanded README with common FFB tuning solutions:
  - "FFB too weak": Suggests adjusting Master Gain or Max Torque Ref.
  - "Baseline vibration": Explains the 10-12Hz Steering Shaft frequency fix.
  - "Strange pull": Advises reducing Rear Align Torque.
- **Developer Guide**: Updated build instructions to reflect the new unified test runner workflow.
- **Feedback & Support**: Added instructions on the "Basic Mode" roadmap and how to effectively report issues using the Screenshot feature.

## [0.6.7] - 2025-12-26
### Changed
- **Unified Test Runner**: Consolidated `test_ffb_engine`, `test_windows_platform`, and `test_screenshot` into a single executable (`run_combined_tests`). This significantly reduces compilation time and provides a comprehensive pass/fail summary for all test suites at once.
- **Security Check**: Replaced `strcpy` with `strcpy_s` in test files to resolve MSVC build warnings and improve safety.

## [0.6.6] - 2025-12-26
### Added
- **Missing Telemetry Warnings**:
  - Added smart console warnings that detect when critical telemetry (Grip, Tire Load, Suspension) is missing or invalid.
  - Warnings now include the **Vehicle Name** to help users identify potentially broken car mods.
  - Implemented hysteresis (persistence check) to prevent false positives during momentary telemetry gaps.

### Fixed
- **Test Suite Integrity**: Resolved a "duplicate main" compilation error in `tests/test_ffb_engine.cpp` and consolidated all regression tests into a single unified runner.

## [0.6.5] - 2025-12-26
### Added
- **Composite Screenshot Feature**:
  - The "Save Screenshot" button now captures both the GUI window and console window in a single image.
  - **Side-by-Side Layout**: Windows are arranged horizontally with a 10px gap for easy viewing.
  - **Automatic Detection**: Console window is automatically detected and included if present.
  - **Graceful Fallback**: If console is not available, captures GUI window only.
  - **Implementation**: Uses Windows `PrintWindow` API to properly capture console windows and other special window types.
  - **Benefits**: Makes it easier to share debugging information and application state with the community. Forum posts and bug reports can now include both GUI settings and console output in a single screenshot.
  - **Documentation**: Added comprehensive user guide (`docs/composite_screenshot.md`) and developer reference (`docs/dev_docs/console_to_gui_integration.md`) for future console integration.

### Fixed
- **Console Window Capture**: Fixed screenshot capture to use `PrintWindow` API instead of `BitBlt`, which properly captures console windows. The previous implementation using `GetDC` only worked for standard windows but produced blank/black images for console windows.

## [0.6.4] - 2025-12-26
### Documentation
- **Enhanced Tooltips**:
  - Overhauled all GUI tooltips in `GuiLayer.cpp` to provide deep technical context, tuning advice, and physical explanations for every FFB parameter.
  - Added specific examples for common hardware (e.g., T300 vs DD) and guidance on how settings like "Steering Shaft Smoothing" or "Slip Angle Smoothing" affect latency and feel.
  - Clarified complex interactions (e.g., Lateral G Boost vs Rear Align Torque) to help users achieve their desired handling balance.

## [0.6.3] - 2025-12-26
### Documentation
- **FFB Formulas Update**:
  - Rewrote `docs/dev_docs/FFB_formulas.md` to perfectly match the current v0.6.2+ codebase.
  - Documented new **Quadratic Lockup** math (`pow(slip, gamma)`).
  - Detailed **Predictive Lockup Logic** (Deceleration triggers) and **Axle Differentiation** (Frequency shift).
  - Added new **ABS Pulse** oscillator formulas.
  - Documented **Split Load Caps** theory (Brake vs Texture).
  - Clarified **Signal Conditioning** steps (Time-Corrected Smoothing, Frequency Estimator, Notch Filters).

## [0.6.2] - 2025-12-25
### Added
- **Dynamic Promotion (DirectInput Recovery)**:
  - Implemented an aggressive recovery mechanism for "Muted Wheel" issues caused by focus loss or the game stealing device priority.
  - When `DIERR_NOTEXCLUSIVEACQUIRED` is detected, the app now explicitly unacquires the device and re-requests **Exclusive Access** before re-acquiring.
  - **FFB Motor Restart**: Now explicitly calls `m_pEffect->Start(1, 0)` immediately after successful re-acquisition, fixing cases where the device is acquired but the haptic motor remains inactive.
  - **Real-time State Tracking**: The internal exclusivity state is now dynamically updated during recovery, ensuring the GUI reflects the actual hardware status.
- **Linux Mock Improvement**: Updated non-Windows device initialization to default to "Exclusive" mode, allowing UI logic (colors/tooltips) to be verified in development environments without physical hardware.
- **First Recovery Notification**: Added a one-time console banner that displays when Dynamic Promotion successfully recovers exclusive access for the first time, confirming to users that the feature is working correctly.
- **User Testing Guide**: Added comprehensive manual verification procedure to `docs/EXCLUSIVE_ACQUISITION_GUIDE.md`, providing step-by-step instructions for users to test the Dynamic Promotion feature.

### Changed
- **GUI Indicator Refinement**:
  - Updated Mode indicator labels ("Mode: EXCLUSIVE (Game FFB Blocked)" / "Mode: SHARED (Potential Conflict)").
  - Added detailed troubleshooting tooltips to the Mode indicator to guide users on how to resolve Force Feedback conflicts with the game.
  - Fixed typo: "reaquiring" → "reacquiring" in tooltip text.

## [0.6.1] - 2025-12-25
### Changed
- **Default Preset Values Updated**:
  - Updated all default values in the `Preset` struct to reflect optimized settings
  - Key changes include:
    - `sop = 1.47059f` (increased from 0.193043f for stronger lateral G feedback)
    - `sop_smoothing = 1.0f` (reduced latency from 0.92f)
    - `slip_smoothing = 0.002f` (reduced from 0.005f for faster response)
    - `oversteer_boost = 2.0f` (increased from 1.19843f)
    - `lockup_start_pct = 1.0f` (earlier activation, was 5.0f)
    - `lockup_full_pct = 5.0f` (tighter range, was 15.0f)
    - `lockup_rear_boost = 3.0f` (increased from 1.5f)
    - `lockup_gamma = 0.5f` (linear response, was 2.0f)
    - `lockup_prediction_sens = 20.0f` (more sensitive, was 50.0f)
    - `lockup_bump_reject = 0.1f` (tighter threshold, was 1.0f)
    - `brake_load_cap = 3.0f` (increased from 1.5f)
    - `abs_gain = 2.0f` (increased from 1.0f)
    - `spin_enabled = false` (disabled by default)
    - `road_enabled = true` (enabled by default)
    - `scrub_drag_gain = 0.0f` (disabled by default, was 0.965217f)
    - `yaw_smoothing = 0.015f` (increased from 0.005f for stability)
    - `optimal_slip_angle = 0.1f` (increased from 0.06f)
    - `steering_shaft_smoothing = 0.0f` (disabled by default)
    - `gyro_smoothing = 0.0f` (disabled by default)
    - `chassis_smoothing = 0.0f` (disabled by default)

### Fixed
- **Test Suite Resilience**:
  - Refactored `test_single_source_of_truth_t300_defaults()` to verify consistency across initialization paths without hardcoding specific values
  - Updated `test_preset_initialization()` to read expected values from Preset struct defaults instead of hardcoding them
  - Widened tolerance in `test_yaw_accel_gating()` to accommodate different yaw_smoothing defaults
  - Tests now automatically adapt to future default value changes, improving maintainability



## [0.6.0] - 2025-12-25
### Added
- **Predictive Lockup Logic (Hybrid Thresholding)**:
  - **Latency Reduction**: The engine now calculates wheel angular deceleration to "foresee" a lockup before the slip ratio actually hits the threshold.
  - **Gating System**: Prevents false triggers by cross-referencing brake pressure (>2%), tire load (>50N), and suspension stability.
  - **Bump Rejection**: Automatically disables predictive triggers during high suspension velocity (curbs/bumps) to prevent erratic vibration.
- **ABS Haptics Simulation**:
  - **Hardware Pulse**: Detects high-frequency brake pressure modulation (ABS activity) from the game and injects a dedicated 20Hz pulse into the steering wheel.
  - **Gain Control**: Independent slider for ABS pulse intensity.
- **Advanced Response Curve (Gamma)**:
  - Added a configurable Gamma curve (0.5 to 3.0) for lockup vibrations.
  - Allows for "Linear" feel (1.0) or sharp, "Late-onset" vibration (2.0-3.0) for better physical fidelity.
- **Physical Pressure Scaling**:
  - Lockup vibration intensity is now physically scaled by internal **Brake Pressure** (Bar) instead of raw pedal position.
  - **Engine Braking Support**: Falling back to 50% intensity for high-slip lockups with zero brake pressure (e.g., downshift lockups).
- **GUI Organization (Advanced Braking)**:
  - Expanded the **"Braking & Lockup"** section with dedicated subsections for "Response Curve", "Prediction (Advanced)", and "ABS & Hardware".

### Changed
- **FFB Engine Refactoring**:
  - Upgraded derivative tracking to process all 4 wheels for rotation, pressure, and deflection.
  - Consolidated lockup logic into a unified 4-wheel worst-case selector with axle frequency differentiation.

### Migration Notes
- **Existing Configurations**: Users with existing `config.ini` files will automatically receive the new default values for v0.6.0 parameters on next save:
  - `lockup_gamma = 2.0` (quadratic response curve)
  - `lockup_prediction_sens = 50.0` (moderate sensitivity)
  - `lockup_bump_reject = 1.0` (1 m/s threshold)
  - `abs_pulse_enabled = true` (enabled by default)
  - `abs_gain = 1.0` (100% strength)
- **No Manual Configuration Required**: The new parameters will be automatically added to your config file when you adjust any setting in the GUI.
- **Validation**: Invalid values loaded from corrupted config files will be automatically clamped to safe ranges and logged to the console.

### Code Quality
- **Code Review Recommendations Implemented** (from v0.6.0 review):
  - Extracted magic numbers to named constants (`ABS_PEDAL_THRESHOLD`, `ABS_PRESSURE_RATE_THRESHOLD`, `PREDICTION_BRAKE_THRESHOLD`, `PREDICTION_LOAD_THRESHOLD`)
  - Added safety comment explaining radius division-by-zero prevention
  - Optimized axle differentiation by pre-calculating front slip ratios outside the loop
  - Added range validation for v0.6.0 parameters in `Config::Load` (gamma, prediction sensitivity, bump rejection, ABS gain)
  - Added precision formatting rationale comment in GUI code
  - Updated CHANGELOG migration notes for v0.6.0

## [0.5.15] - 2025-12-25
### Changed
- **Device Wheel Dynamic Exclusivity Awareness**:
  - The application now detects if device wheel exclusive access is lost at runtime (e.g., via Alt-Tab or focus stealing).
  - Automatically updates the internal `m_isExclusive` state upon detecting `DIERR_OTHERAPPHASPRIO` or `DIERR_NOTEXCLUSIVEACQUIRED`.
  - This ensures the GUI correctly transitions from Red/Green "EXCLUSIVE" status to Yellow "SHARED" warning in real-time when a conflict is detected.

## [0.5.14] - 2025-12-25

### Changed
- **Improved FFB Error Handling**:
  - Implemented `GetDirectInputErrorString` helper to provide verbose, official Microsoft descriptions for all DirectInput success and error codes.
  - Explicitly handles `DIERR_OTHERAPPHASPRIO` (0x80040205) with a clear, actionable warning: "Game has stolen priority! DISABLE IN-GAME FFB".
  - Consolidated duplicate DirectInput error macros (e.g., `E_ACCESSDENIED`, `S_FALSE`) to ensure robust error identification across different Windows SDKs.
  - Maintained connection recovery logic while providing deeper diagnostic insight into why FFB commands might fail.
- **Project Structure Reorganization**: Moved `main.cpp` and `FFBEngine.h` from project root to `src/` directory for better organization and cleaner project structure.
  - All source code now consolidated in the `src/` directory
  - Updated all include paths across the codebase
  - Follows standard C++ project conventions




## [0.5.13] - 2025-12-25

### Added
- **Quadratic Lockup Ramp**: Replaced the linear lockup severity ramp with a quadratic curve for a more progressive and natural-feeling onset of vibration during brake modulation.
- **Split Load Caps**: Introduced separate safety limiters for Textures vs. Braking:
    - **Texture Load Cap**: Specifically limits Road and Slide vibration intensity.
    - **Brake Load Cap**: A dedicated limiter for Lockup vibration, allowing for stronger feedback during high-downforce braking events (~3.0x).
- **Advanced Lockup Tuning**:
    - **Dynamic Thresholds**: Added "Start Slip %" and "Full Slip %" sliders to customize the vibration trigger window.
    - **Rear Lockup Boost**: Added a multiplier (1.0 - 3.0x) to amplify vibrations when the rear axle is the dominant lockup source.
- **GUI Organization**:
    - New **"Braking & Lockup"** collapsible section grouping all related sliders and checkboxes.
    - Renamed "Load Cap" in the Textures section to **"Texture Load Cap"** to clarify its specific scope.

### Fixed
- **Manual Slip Calculation**: Corrected a sign error in the manual slip ratio calculation by properly handling forward velocity direction in `get_slip_ratio`.
- **Axle Differentiation Refinement**: Improved the detection logic for dominant lockup source to ensure "Heavy Judder" triggers reliably when rear wheels lock harder than front wheels.
### Improved
- **Code Quality Enhancements**:
  - **Extracted Magic Number**: Replaced hardcoded `0.01` hysteresis value in axle differentiation logic with named constant `AXLE_DIFF_HYSTERESIS` for better maintainability and documentation.
  - **Test Baseline Alignment**: Updated `test_progressive_lockup` to use production defaults (5%/15% thresholds) instead of test-specific values, ensuring tests validate actual user experience.
  - **Enhanced Test Precision**: Improved `test_split_load_caps` with explicit 3x ratio verification and separate assertions for road texture and brake load cap validation, providing better diagnostic output.

## [0.5.12] - 2025-12-25
### Changed
- **FFB Engine Single Source of Truth (SSOT)**:
    - Refactored `FFBEngine` to eliminate hardcoded default values, following a DRY (Don't Repeat Yourself) approach.
    - Centralized all physics defaults within `Config.h` (`Preset::ApplyDefaultsToEngine`), ensuring the main application and test suite share the exact same configuration baseline.
    - Standardized default initialization on the calibrated T300 physics preset.
- **Preset Calibration & Normalization**:
    - Updated `Default (T300)` and `T300` presets to align with the normalized 0-100 slider ranges (percentage-based) introduced in previous versions.
    - This migration ensures presets no longer rely on legacy raw Newton-meter intensities, providing a consistent feeling across different wheel hardwares.
- **Test Suite Revamp**:
    - **Full Stabilization**: Fixed and verified all 157 FFB engine tests following the SSOT refactor.
    - **Modernized Expectations**: Updated legacy test assertions to align with the improved T300 physics baseline (e.g., Scrub Drag gain of 0.965, Lockup frequency ratio of 0.3).
    - **Robust Telemetry Mocking**: Improved `test_ffb_engine.cpp` with comprehensive wheel initialization to prevent silent failures in multi-axle calculations.
    - **Test Helper**: Introduced `InitializeEngine()` to provide consistent, stable baselines for legacy tests while allowing specific physics overrides for regression verification.

## [0.5.11] - 2025-12-24
### Fixed
- **Lockup Vibration Ignoring Rear Wheels**: Fixed a bug where locking the rear brakes (common in LMP2 or under heavy engine braking) would not trigger any vibration feedback.
- **Improved Axle Differentiation**: Added tactile cues to distinguish between front and rear lockups using frequency:
    - **Front Lockup**: Remains at a higher pitch ("Screech") for standard understeer feedback.
    - **Rear Lockup**: Uses a 50% lower frequency ("Heavy Judder") to warn clearly of rear axle instability.
    - **Intensity Boost**: Rear lockups now receive a 1.2x amplitude boost to emphasize the danger of a potential spin.

### Added
- **Unit Testing**: Added `test_rear_lockup_differentiation()` to the verification suite to ensure both axles trigger feedback and maintain correct frequency ratios.

## [0.5.10] - 2025-12-24
### Added
- **Exposed Contextual Smoothing Sliders**:
    - **Kick Response**: Added smoothing slider for Yaw Acceleration (Kick) effect, placed immediately after the effect gain.
    - **Gyro Smooth**: Added smoothing slider for Gyroscopic Damping, placed immediately after the effect gain.
    - **Chassis Inertia (Load)**: Added smoothing slider for simulated tire load, placed in the Grip & Slip Estimation section.
- **Visual Latency Indicators**:
    - Real-time latency readout (ms) for smoothing parameters.
    - **Red/Green Color Coding** for Yaw Kick (>15ms) and Gyro (>20ms) to warn against excessive lag.
    - **Blue Info Text** for Chassis Inertia to indicate "Simulated" time constant.

### Changed
- **FFB Engine Refactoring**:
    - Moved hardcoded time constants for Yaw and Chassis Inertia into configurable member variables.
    - Standardized Gyro Smoothing to use the same Time Constant (seconds) math as other filters.
- **Config Persistence**: New smoothing parameters are now saved to `config.ini` and supported in user presets.

## [0.5.9] - 2025-12-24
### Changed
- **Improved Load Cap widget**:
    - Moved the slider under the  "Tactile Textures" section, since it only affects Texture and Vibration effects: Road Textures (Bumps/Curbs), Slide, Lockup.
    - More informative Tooltip text.
- **Improved Slip Angle Smoothing tooltip**: Added detailed technical explanation of the filter behavior and influenced effects.
- **Optimized Yaw Kick Smoothing**: Reduced default smoothing latency from 22.5ms (7Hz) to **10.0ms (~16Hz)**.
    - **Stability**: Prevents "Slide Texture" vibration (40-200Hz) from being misinterpreted by physics as Yaw Acceleration spikes, which previously caused feedback loops/explosions.
    - **Responsiveness**: Improved reaction time to snap oversteer. 10ms provides the optimal balance: fast enough for car rotation (<5Hz) while effectively filtering high-frequency noise (>40Hz).
    - **Detailed Technical Comments**: Added comprehensive documentation in `FFBEngine.h` regarding the impact of different smoothing levels (3.2ms to 31.8ms) on feedback loops and "raw" feel.
- **Expanded Rear Axle (Oversteer) Tooltips**:
    - **Lateral G Boost (Slide)** (formerly Oversteer Boost): Expanded to explain the relationship with car mass inertia and momentum.
    - **Rear Align Torque**: Added guidance on buildup speed and its role as the active "pull" during counter-steering.
    - **Yaw Kick**: Clarified its role as the sharp, momentary impulse signaling the onset of rotation.
    - **Tuning Goals**: Integrated explicit tuning goals into the tooltips to help users balance the "active pull" (Rear Align) against the "sustained effort" (Lateral G Boost).
- **Renamed "Oversteer Boost" to "Lateral G Boost (Slide)"**:
    - Updated GUI label and Troubleshooting graphs for better clarity on the effect's physical mechanism.
    - Synchronized all internal documentation, code comments, and unit tests with the new nomenclature.




## [0.5.8] - 2025-12-24
### Added
- **Aggressive FFB Recovery with Smart Throttling**: Implemented more robust DirectInput connection recovery.
    - **Universal Detection**: The engine now treats *all* `SetParameters` failures as recoverable, ensuring that "Unknown" DirectInput errors (often caused by focus loss) trigger a re-acquisition attempt.
    - **Smart Cool-down**: Recovery attempts are now throttled to once every 2 seconds to prevent CPU spam and "Tug of War" issues when the game has exclusive control of the device. This eliminates the 400Hz retry loop that could cause stuttering.
    - **Immediate Re-Acquisition**: Logs `HRESULT` error codes in hexadecimal (e.g., `0x80070005`) to assist with deep troubleshooting of focus-stealing apps.
    - **FFB Motor Restart**: Explicitly calls `m_pEffect->Start(1, 0)` upon successful recovery, ensuring force feedback resumes immediately without requiring an app restart.
- **Configuration Safety Validation**: Added `test_config_safety_validation_v057()` to verify that invalid grip parameters (e.g., zero values that would cause division-by-zero) are automatically reset to safe defaults when loading corrupted config files.

### Changed
- **Default "Always on Top"**: Changed `m_always_on_top` to `true` by default. This ensures the LMUFFB window remains visible and prioritized by the OS scheduler out-of-the-box, preventing background deprioritization and focus loss during gameplay.

## [0.5.7] - 2025-12-24
### Added
- **Steering Shaft Smoothing**: New "Steering Shaft Smooth" slider in the GUI.
    - **Signal Conditioning**: Applies a Time-Corrected Low Pass Filter specifically to the `mSteeringShaftTorque` input, reducing mechanical graininess and high-frequency "fizz" from the game's physics engine.
    - **Latency Awareness**: Displays real-time latency readout (ms) with color-coding (Green for < 15ms, Red for >= 15ms) to guide tuning decisions.
- **Configurable Optimal Slip Parameters**: Added sliders to customize the tire physics model in the "Grip Estimation" section.
    - **Optimal Slip Angle**: Allows users to define the peak lateral grip threshold (radians). Tunable for different car categories (e.g., lower for Hypercars, higher for GT3).
    - **Optimal Slip Ratio**: Allows defining the peak longitudinal grip threshold (percentage).
    - **Enhanced Grip Reconstruction**: The underlying grip approximation logic (used when telemetry is blocked or missing) now utilizes these configurable parameters instead of hardcoded defaults.
- **Improved Test Coverage**: Added `test_grip_threshold_sensitivity()` and `test_steering_shaft_smoothing()` to verify physics integrity and filter convergence.


## [0.5.6] - 2025-12-24
### Changed
- **Graphs Window Cleanup**:
    - **Removed Telemetry Warnings**: Removed "Missing Tire Load (Check shared memory)" and "Missing Grip Data (Ice or Error)" bullet points from the Troubleshooting Graphs window. These warnings were often distracting during normal gameplay with certain car classes.
    - **Visual Optimization**: Eliminated the red "(MISSING)" status text from the "Raw Front Load" and "Raw Front Grip" graph labels for a cleaner interface.
    - **Header Logic**: The "TELEMETRY WARNINGS:" section now only appears if there is a critical timing issue (Invalid DeltaTime), reducing visual noise.

## [0.5.5] - 2025-12-24
### Added
- **"Smart Container" Dynamic Resizing**: The OS window now automatically resizes based on the GUI state.
    - **Reactive Layout**: Toggling "Graphs" expands the window to a wide "Analysis" view and contracting it back to a narrow "Config" view.
    - **Independent Persistence**: Saves and restores the window position and dimensions for both "Small" (Config) and "Large" (Graphs) states independently.
- **Docked Window Management**: Implemented "hard-docking" for internal ImGui windows.
    - **Auto-Fill**: Tuning and Debug windows now automatically dock to the edges of the OS window, filling all available space without floating title bars or borders.
    - **Zero Clutter**: Removed overlapping window borders and unnecessary window decorations for a native-app feel.
- **Regression Tests**: Added `test_window_config_persistence()` to verify that window states (x, y, width, height, graphs-on/off) are correctly saved and loaded.

### Changed
- **Code Quality Improvements** (Post-Review Refinements):
    - **Minimum Window Size Enforcement**: Added validation to prevent window dimensions from falling below 400x600, ensuring UI remains usable even if config file is corrupted.
    - **Window Position Validation**: Implemented bounds checking to detect and correct off-screen window positions (e.g., after monitor configuration changes).
    - **Eliminated Magic Number Duplication**: Defined `CONFIG_PANEL_WIDTH` as a file-level constant to eliminate duplication between `DrawTuningWindow` and `DrawDebugWindow`.
    - **Enhanced Documentation**: Improved inline comments for helper functions with detailed descriptions and parameter documentation.

## [0.5.3] - 2025-12-24
### Fixed
- **Restored Latency Display**: Re-implemented the missing latency indicators for "SoP Smoothing" and "Slip Angle Smoothing" sliders that were accidentally removed in the v0.5.0 overhaul.
    - **Enhanced Layout**: Moved latency text (e.g., "Latency: 15 ms - OK") to the right column above the slider for better readability, preventing clutter.
    - **Improved Precision**: Added rounding logic to latency calculations so that values like 0.85 smoothing correctly display as "15 ms" instead of truncating to "14 ms".
    - **Color Coding**: Restored green (<15ms) vs red (>=15ms) visual warnings.

### Changed
- **GUI Organization**: Converted the "Signal Filtering" static header into a collapsible section, matching the behavior of other groups like "Advanced SoP" and "Textures".

### Added
- **Regression Tests**: Added `test_latency_display_regression()` to the verification suite.
    - Verifies accurate latency calculation (including rounding).
    - Checks color coding thresholds.
    - Validates display string formatting.

## [0.5.2] - 2025-12-24
### Fixed
- **CRITICAL: Understeer Effect Slider Stuck**: Fixed slider being completely unresponsive to mouse and arrow key inputs
    - **Root Cause**: Was using pre-calculated percentage format string that ImGui couldn't properly interpret
    - **Fix**: Simplified to use direct `%.2f` format on the 0-50 range instead of percentage calculation
    - **Impact**: Slider is now fully functional and responsive, shows values like "25.00" → "25.01" with fine precision
- **Slider Precision Issues**: Fixed additional sliders where arrow key adjustments weren't visible
    - **Load Cap**: Updated format from `%.1fx` to `%.2fx` (now shows 1.50x → 1.51x instead of 1.5x → 1.5x)
    - **Target Frequency**: Updated format from `%.0f Hz` to `%.1f Hz` (now shows 50.0 → 50.1 instead of 50 → 50)
- **Tooltip Covering Slider During Adjustment**: Fixed tooltip appearing immediately when pressing arrow keys and covering the slider being adjusted
    - **Fix**: Tooltip now only displays when NOT actively adjusting with arrow keys
    - **Benefit**: Users can now see the slider value change in real-time without obstruction

### Added
- **Regression Tests**: Added `test_slider_precision_regression()` with 9 assertions to prevent slider bugs from reoccurring
    - Test Case 1: Load Cap precision verification
    - Test Case 2: Target Frequency precision verification
    - Test Case 3: Understeer Effect static buffer persistence
    - Test Case 4: Step size and display precision alignment for all ranges
- **Build Warning Fix**: Added `DIRECTINPUT_VERSION` definition to `test_windows_platform.cpp` to eliminate compiler warning

### Test Coverage
- **Windows Platform Tests**: 38 passing (increased from 29)
- **Total Test Suite**: 184 passing (146 FFB Engine + 38 Windows Platform)

## [0.5.1] - 2025-12-24
### Fixed
- **Slider Precision Display Issues**: Fixed sliders where arrow key adjustments weren't visible due to insufficient decimal places.
    - **Filter Width (Q)**: Updated format from `%.1f` to `%.2f` to show 0.01 step changes
    - **Slide Pitch**: Updated format from `%.1fx` to `%.2fx` for better precision visibility
    - **Understeer Effect**: Updated to show 1 decimal place (`%.1f%%`) instead of 0 decimals
    - **All Percentage Sliders**: Updated `FormatDecoupled` and `FormatPct` to use `%.1f%%` instead of `%.0f%%`
    - **Improved Step Size Logic**: Added finer 0.001 step for small ranges (<1.0) to ensure precise adjustments on sliders like Slip Smoothing
    - **Affected Sliders**: 15 total sliders now provide immediate visual feedback for arrow key adjustments
- **Build Error**: Added missing `GripResult` struct definition to `FFBEngine.h` that was causing compilation failures

### Added
- **Test Coverage**: Added `test_slider_precision_display()` with 5 test cases to verify slider format strings have sufficient decimal places
- **Code Quality**: Made all test functions in `test_windows_platform.cpp` static to generate compiler warnings if not called

## [0.5.0] - 2025-12-24
### Changed
- **Code Quality Improvements**:
    - **Eliminated Hardcoded Base Nm Values**: Refactored GUI layer to reference centralized physics constants from `FFBEngine.h` instead of duplicating magic numbers.
        - All `FormatDecoupled()` calls in `GuiLayer.cpp` now use `FFBEngine::BASE_NM_*` constants (e.g., `BASE_NM_SLIDE_TEXTURE`, `BASE_NM_REAR_ALIGN`).
        - **Benefit**: Single source of truth for physics multipliers. If base force values change in the engine, the GUI automatically reflects those changes without manual updates.
        - **Maintainability**: Eliminates the risk of GUI and physics constants drifting out of sync.
    - **GUI Layout Refinement**: Moved connection status ("Disconnected from LMU" text and "Retry" button) to a separate line in the main window.
        - **Benefit**: Allows the overall window to be narrower, improving usability on smaller screens.

## [0.4.50] - 2025-12-24
### Added
- **FFB Signal Gain Compensation (Decoupling)**: Implemented automatic scaling for Generator effects to resolve "signal compression" on high-torque wheels.
    - **Effect Decoupling**: "Generator" effects (SoP, Rear Align, Yaw Kick, Textures) are now automatically scaled up when `Max Torque Ref` increases. This ensures that a 10% road texture feel remains equally perceptible whether using a 2.5 Nm G29 or a 25 Nm DD wheel.
    - **Physical Force Estimation**: GUI sliders now display estimated real-world torque in Newton-meters (e.g., `~2.5 Nm`) based on current gain and wheel calibration.
    - **Modifier Protection**: Modifiers like "Understeer Effect" and "Oversteer Boost" remain unscaled to avoid double-amplification, maintaining predictable physics behavior.
- **GUI Standardization**:
    - **Standardized Ranges**: Updated all effect sliders to use a common `0% - 200%` range (0.0 - 2.0 internal) for better consistency.
    - **Percentage Display**: Switched all gain sliders to use percentage formatting (e.g., `85%`) for more intuitive tuning.
- **Unit Tests**: Added `test_gain_compensation` to verify mathematical decoupling and differentiate between Generators and Modifiers.

### Changed
- **Optimized Slider Ranges**:
    - Reduced extreme 20.0x multipliers to a more manageable 2.0x (200%) baseline, as the new decoupling logic handles the heavy lifting for high-torque hardware.

## [0.4.49]
### Changed
- **Visual Design Overhaul (Dark Theme & Grid Layout)**:
    - Improved visual design and readability of the app.
    - **Professional "Deep Dark" Theme**: Replaced the default ImGui style with a custom flat dark theme. Features a deep grey background and high-contrast teal/blue accents for interactive controls.
    - **2-Column Grid Layout**: Refactored the Tuning Window to a strict 2-column layout (Labels on the Left, Controls on the Right). This eliminates the "ragged edge" and makes it significantly easier to scan settings and values.
    - **Clean Section Headers**: Replaced solid-colored title bars with transparent headers and accent lines. This removes the distracting "zebra striping" effect and reduces visual noise.
    - **Improved Hierarchy**: Added logical groupings and cleaner spacing between functional units (General, Front Axle, Rear Axle, Textures, etc.).
    - **Developer Architecture**: Promoted `SetupGUIStyle()` to a public static method for external testing and flexible initialization.
### Added
- **UI Verification Test**: Added `test_gui_style_application` to the platform test suite. This headless test verifies that theme colors and layout constants are applied correctly to the ImGui style object without needing a physical window.

## [0.4.48] - 2025-12-23
### Fixed
- **"Always on Top" Reliability**:
    - Resolved issue where the window state would not correctly persist or reflect in system style bits on some Windows configurations.
    - Added `SWP_FRAMECHANGED` and `SWP_NOACTIVATE` flags to `SetWindowPos` to ensure immediate UI refresh and prevent focus stealing.
    - Optimized initialization order to apply the Window-on-Top state after the window has been fully shown.
- **Test Suite Hardening**:
    - Updated `test_window_always_on_top_behavior` to use visible windows and explicit return value validation, ensuring the platform-level verification is robust against environment variations.

## [0.4.47] - 2025-12-23
### Changed
- **GUI Refinement**:
    - Renamed the **"General"** section to **"General FFB Settings"** to better reflect its purpose.
    - Reordered widgets in the General section: **"Invert FFB Signal"** is now the first control, followed by **"Master Gain"**.

## [0.4.46] - 2025-12-23
### Added
- **Major GUI Reorganization**: Completely restructured the Tuning Window for professional ergonomics and logical flow.
    - **Logical Grouping**: Parameters are now grouped into 10 collapsible sections: *Core Settings, Game Status, App Controls, Presets, General FFB Settings, Understeer/Front Tyres, Oversteer/Rear Tyres, Grip Estimation, Haptics,* and *Textures*.
    - **Focused SoP Management**: Grouped all rear-end and rotation effects (Lateral G, Rear Align Torque, Yaw Kick, Gyro) into a dedicated SoP hierarchy.
    - **Compact App Controls**: Consolidated system controls (Always on Top, Graphs, Screenshots) onto a single functional line.
    - **Visual Cleanup**: Removed obsolete vJoy monitoring tools and development placeholders to declutter the user interface.
- **Enhanced Test Suite**: Added 2 new platform-level verification tests (bringing total to 14 passing tests in the platform suite):
    - `test_window_always_on_top_behavior`: Verifies correct application of Win32 `WS_EX_TOPMOST` style bits.
    - `test_preset_management_system`: Verifies the integrity of the engine-to-preset state capture and memory management.

## [0.4.45] - 2025-12-23
### Added
- **"Always on Top" Mode**: New checkbox in the Tuning Window to keep the application visible over the game or other windows.
    - Prevents losing sight of telemetry or settings when clicking back into the game.
    - Setting is persisted in `config.ini` and reapplied on startup.
- **Keyboard Fine-Tuning for Sliders**: Enhanced slider control for precise adjustments.
    - **Hover + Arrow Keys**: Simply hover the mouse over any slider and use **Left/Right Arrow** keys to adjust the value by small increments.
    - **Dynamic Stepping**: Automatically uses `0.01` for small-range effects (Gains) and `0.5` for larger-range effects (Max Torque).
    - **Tooltip Integration**: Added a hint to all sliders explaining the arrow key and Ctrl+Click shortcuts.
- **Persistence Logic**: Added unit tests to ensure window settings are correctly saved and loaded.

## [0.4.44] - 2025-12-21
### Added
- **Device Selection Persistence**: The application now remembers your selected steering wheel across restarts.
    - Automatically scans and matches the last used device GUID on startup.
    - Saves selections immediately to `config.ini` when changed in the GUI.
- **Connection Hardening (Smart Reconnect)**: Implemented robust error handling for DirectInput failures.
    - **Physical Connection Recovery**: Explicitly restarts the FFB motor using `Start(1, 0)` upon re-acquisition, fixing the "silent wheel" issue after Alt-Tab or driver resets.
    - **Automatic Re-Acquisition**: Detects `DIERR_INPUTLOST` and `DIERR_NOTACQUIRED` to trigger immediate recovery.
    - **Diagnostics**: Added foreground window logging to the console (rate-limited to 1s) when FFB is lost, helping identify if other apps (like the game) are stealing exclusive priority.
- **Console Optimization**: Removed the frequent "FFB Output Saturated" warning to declutter the console for critical connection diagnostics.

## [0.4.43] - 2025-12-21
### Added
- **Static Notch Filter**: Implemented a surgical static notch filter to eliminate constant-frequency mechanical hum or vibration.
    - **Customizable Frequency**: Users can now target specific noise frequencies between 10Hz and 100Hz.
    - **Surgical Precision**: Uses a fixed Q-factor of 5.0 for minimal interference with surrounding road detail.
    - **Safety Tooltips**: Added warnings regarding potential loss of road detail at high frequencies.
- **Dynamic Suppression Strength**: Added a "Suppression Strength" slider to the Dynamic Flatspot Suppression effect.
    - Enables linear blending between raw and filtered forces, allowing users to fine-tune the balance between comfort and flatspot feedback.
- **Unit Tests**: Added `test_static_notch_integration` to verify the mathematical integrity and attenuation performance of the new filter.
- **Technical Details**:
    - **FFBEngine.h**: Added manual control and second `BiquadNotch` instance for static noise.
    - **Config.cpp**: Added persistence for `static_notch_enabled`, `static_notch_freq`, and `flatspot_strength`.
    - **GuiLayer.cpp**: Integrated new controls into the "Signal Filtering" section.

## [0.4.42] - 2025-12-21
### Added
- **Yaw Kick Signal Conditioning**: Implemented filters to eliminate constant "physics noise" from the Yaw Kick effect.
    - **Low Speed Cutoff**: Mutes the effect when moving slower than 5 m/s (18 kph) to prevent engine idle vibration and parking lot jitters.
    - **Noise Gate (Deadzone)**: Filters out micro-rotations below 0.2 rad/s² to ensure the "Kick" only triggers during significant events (like slide initiation).
    - **Technical Impact**: Resolves the "muddy" FFB feeling caused by constant background noise, making the counter-steering cue much clearer.
- **Unit Tests**: Added `test_yaw_kick_signal_conditioning` to verify the new filtering logic handling.

## [0.4.41] - 2025-12-21
### Added
- **Dynamic Notch Filter (Flatspot Suppression)**: Implemented a speed-tracking notch filter to surgically remove vibrations linked to wheel rotation frequency (e.g., flat spots, unbalanced tires).
    - **Tracking Logic**: Automatically calculates the notch center frequency based on longitudinal car speed and tire radius ($f = v / 2\pi r$).
    - **Zero Latency**: Uses a high-precision Biquad IIR filter that removes the offending frequency without adding overall group delay (lag) to the steering signal.
    - **Configurable Precision**: Added "Notch Width (Q)" slider to control how "narrow" the filter is. High Q values (e.g. 5.0) are surgical; lower values (e.g. 1.0) are softer.
- **Frequency Estimator (Signal Analysis)**: Added a real-time vibration analyzer using zero-crossing detection.
    - **Diagnostics**: Displays the "Estimated Vibration Freq" in the Debug Window, allowing users to verify if their FFB vibrations match the wheel's rotational frequency.
    - **Theoretical Comparison**: Displays the expected wheel frequency based on current speed for quick verification.
- **Signal Filtering UI**: Added a new "Signal Filtering" section to the Tuning Window.
- **User Guide**: `docs\Dynamic Flatspot Suppression - User Guide.md`.
- **Enhanced Test Suite**: Added 2 new signal processing tests:
    - `test_notch_filter_attenuation`: Verifies that the notch filter correctly kills the target frequency while passing steering inputs (2Hz) untouched.
    - `test_frequency_estimator`: Verifies that the analyzer accurately detects a simulated 20Hz vibration.

### Technical Details
- **FFBEngine.h**: Added `BiquadNotch` utility struct and integrated tracking logic into the main force calculation.
- **Config.cpp**: Added persistence for `flatspot_suppression` and `notch_q` settings.
- **GuiLayer.cpp**: Integrated frequency diagnostics into the "Signal Analysis" debug section.

### Added
- **Configurable Slip Angle Smoothing**: Exposed the internal physics smoothing time constant (tau) as a user setting in the "Advanced Tuning" section.
    - Allows users to balance "Physics Response Time" against signal noise for Understeer and Rear Align Torque effects.
    - Added a new slider with real-time latency readout (ms).
- **GUI Latency Readouts**: Added dynamic, color-coded latency indicators for smoothing filters.
    - **Green Labels**: Indicators show "(Latency: XX ms - OK)" for settings <= 20ms.
    - **Red Labels**: Indicators warn "(SIGNAL LATENCY: XX ms)" for settings > 20ms.
    - Tooltips now explicitly explain the trade-offs: "High Latency = Smooth but Slow; Low Latency = Fast but Grainy."

### Changed
- **Optimized Default Latency**: Reduced default filter latency from ~95ms to **15ms** to address user reports of "FFB delay."
    - **SoP Smoothing**: Changed default from 0.05 (95ms) to **0.85** (15ms).
    - **Slip Angle Smoothing**: Changed default from 0.0225 (22.5ms) to **0.015** (15ms).
- **Preset Synchronization**: Updated "Default (T300)" and "T300" presets to use the new 15ms target values.

### Technical Details
- **FFBEngine.h**: Promoted `tau` to `m_slip_angle_smoothing` with a safety clamp (`0.0001s`).
- **Config.cpp**: Added persistence for `slip_angle_smoothing` in `config.ini` and updated preset builders.

## [0.4.39] - 2025-12-20
### Added
- **Advanced Physics Reconstruction (Encrypted Content Fix)**: Implemented a new physics modeling layer to restore high-fidelity FFB for cars with blocked telemetry (DLC/LMU Hypercars).
    - **Adaptive Kinematic Load**: Reconstructs vertical tire load using chassis kinematics (Acceleration, Weight Transfer) and Aerodynamics ($v^2$) when suspension sensors are blocked. This restores dynamic weight feel (braking dive, aero load) that was previously missing.
    - **Combined Friction Circle**: Grip calculation now accounts for **Longitudinal Slip** (Braking/Acceleration) in addition to Lateral Slip. The steering will now correctly lighten during straight-line braking lockups.
    - **Chassis Inertia Simulation**: Applied Time-Corrected Smoothing (~35ms latency) to accelerometer inputs to simulate physical roll and pitch, preventing "digital" or jerky weight transfer feel.
- **Work-Based Scrubbing**: Refined Slide Texture to scale based on `Load * (1.0 - Grip)`. Vibration is now physically linked to the energy dissipated by the contact patch.
- **Physics Test Suite (v0.4.39 Expansion)**: Added 5 new high-fidelity physics tests (bringing total to 134 passing tests):
    - `test_chassis_inertia_smoothing_convergence`: Verifies time-corrected filter response and chassis decay timing.
    - `test_kinematic_load_cornering`: Verifies lateral weight transfer directions (+X = Left) and magnitude (~2400N @ 1G).
    - Updated `test_slide_texture` to account for new Work-Based Scrubbing physics.

### Fixed
- **Coordinate System Alignment**: Explicitly verified and documented LMU coordinate conventions (+X = Left, +Z = Rearward) for all lateral weight transfer and counter-steering torque calculations.
- **Telemetry Gap Documentation**: Identified and documented potential "Gap A" (Silent Road Texture) and "Gap B" (Constant Scraping) fallback strategies for future encrypted content updates.

### Changed
- **Code Hardening**: Eliminated "magic numbers" in physics calculations, replacing them with named constants (`WEIGHT_TRANSFER_SCALE`, `MIN_VALID_SUSP_FORCE`) for better transparency and tunability.
- **Fallback Logic**: The engine now automatically switches to the Kinematic Model if `mSuspForce` is detected as invalid (static/zero), ensuring support for all vehicle classes.

## [0.4.38] - 2025-12-20
### Added
- **Time-Corrected Smoothing Filters (v0.4.37/38)**: Re-implemented core smoothing filters to use real-time coefficients (tau) instead of fixed frame-based alpha.
    - **Consistent Feel**: FFB responsiveness (lag/smoothing) now remains identical regardless of whether the game is running at 400Hz, 60Hz, or experiencing a stutter.
    - **Affected Effects**: Slip Angle (Understeer), Yaw Acceleration (Kick), SoP Lateral G, and Gyroscopic Damping.
    - **Optimization**: Standardized on $\tau = 0.0225s$ (approx 0.1 legacy alpha at 400Hz) for the ideal balance of physics clarity and noise rejection.
- **Physics Stability & Oscillator Hardening**:
    - **Phase Explosion Protection**: All oscillators (Slide, Lockup, Spin, Bottoming) now use `std::fmod` for phase accumulation. This fixes the "Permanent Full-Force Texture" bug that occurred during large frame stutters.
    - **Gyro Damping Safety**: Added internal clamps [0.0, 0.99] to the gyroscopic smoothing factor to prevent mathematical instability from invalid configuration.
- **Enhanced Regression Tests**: Added and expanded unit tests to verify physics integrity during extreme conditions:
    - `test_regression_phase_explosion`: Now covers all oscillators during simulated 50ms stutters.
    - `test_time_corrected_smoothing`: Verifies filter convergence consistency between High-FPS and Low-FPS updates.
    - `test_gyro_stability`: Verifies safety clamps against malicious/malformed configuration inputs.

### Changed
- **Documentation Refinement (Safety Fix)**: Updated `Yaw, Gyroscopic Damping... implementation.md` and `FFB_formulas.md` to reflect the new time-corrected math and robust phase wrapping. Fixed unsafe code examples in dev docs that suggested simple subtraction for phase wrapping.
- **Test Suite Alignment**: Updated `test_rear_force_workaround` expectations to match the new, faster dynamics of the time-corrected smoothing filters.

## [0.4.37] - 2025-12-20
### Changed
- **Calibrated Default Presets**: Updated the "Default (T300)" and internal defaults to match the latest calibrated values for belt-driven wheels.
    - **SoP Scale**: Reduced from 5.0 to **1.0**.
    - **Understeer Gain**: Adjusted to **0.61**.
    - **SoP Gain**: Adjusted to **0.08**.
    - **Oversteer Boost**: Adjusted to **0.65**.
    - **Rear Align Torque**: Adjusted to **0.90**.
    - **Slide Texture**: Now **Enabled by default** with Gain **0.39**.
    - **Max Torque Ref**: Adjusted to **98.3 Nm**.

## [0.4.36] - 2025-12-20
### Added
- **Slide Rumble Frequency Slider**: Added a "Slide Pitch (Freq)" slider to the GUI to allow manual customization of the vibration frequency.
    - **Optimization**: This allows both Belt/Gear-driven users (who need low-frequency rumble, 10-60Hz) and Direct Drive users (who prefer high-frequency fine texture, 100-250Hz) to tune the effect to their hardware's sweet spot.
    - **Range**: 0.5x to 5.0x multiplier.
    - **Default**: 1.0x (Rumble optimized).

## [0.4.35] - 2025-12-20
### Changed
- **Slide Texture Frequency Optimization**: Re-mapped the vibration frequency for Slide Rumble to the "Tactile Sweet Spot" for belt-driven wheels (10Hz - 70Hz).
    - **Previous Behavior**: Frequencies ranged from 40Hz to 250Hz. High frequencies (above 100Hz) are often dampened by rubber belts and interpreted as a subtle "fizz" rather than a gritty rumble.
    - **New Behavior**: Frequency starts at 10Hz (chunky grind) and ramps to 70Hz (fast buzz) based on slip speed. This provides significantly better tactile feedback on hardware like the T300 and G29.
    - **Aliasing Protection**: Lowering the frequency range also improves signal stability relative to the 400Hz physics loop (improving Nyquist headroom).
- **Refined Effect Gain Ranges**: Increased the maximum slider limits for dynamic effects and textures in the GUI from 1.0 to **5.0**.
    - This provides enough headroom to "punch through" belt friction on high-torque settings while maintaining high precision for fine-tuning. Previously tried 20.0, but found 5.0 to be the ideal balance.

### Fixed
- **Slide Texture Scope Expansion**: Updated "Slide Rumble" effect to trigger based on the **maximum** lateral slip of either axle (Front OR Rear).
    - **Previous Behavior**: Only monitored front wheels (Understeer). Doing a donut or drift (Rear Slide) resulted in no vibration, making the car feel "floating."
    - **New Behavior**: Calculates front and rear average slip velocities independently and uses the greater of the two to drive the vibration effect.
    - **Impact**: You now feel the gritty tire scrub texture during donuts, power slides, and extensive oversteer, solving the "silent drift" issue.

## [0.4.34] - 2025-12-20
### Fixed
- **Slide Texture Scope Expansion** (Superceded by v0.4.35 logic)


## [0.4.33] - 2025-12-20
### Fixed
- **CRITICAL: Oscillator Phase Explosion Fix**: Fixed a major bug where dynamic effects (Slide Texture, Progressive Lockup, Wheel Spin, and Suspension Bottoming) would produce massive constant forces or "flatlined" signals during frame stutters or telemetry lag.
    - **Root Cause**: The phase accumulation logic used a simple `if` check for wrapping (`if (phase > TWO_PI) phase -= TWO_PI`), which failed if the phase jumped by more than $2\pi$ in a single step (e.g., during a 50ms stutter at high frequencies). This caused the phase to grow indefinitely, leading the sawtooth/sine formulas to output impossible values.
    - **Fix**: Replaced simple check with `std::fmod(phase, TWO_PI)` for all oscillators to ensure robust wrapping regardless of the time step size.
    - **Impact**: Resolves the "Slide Texture strong pull" bug, ensuring a consistent vibration feel even during system hitches.
    - **Regression Test**: Added `test_regression_phase_explosion` to the test suite to simulate high delta-time stutters and verify phase wrapping integrity.

## [0.4.32] - 2025-12-20
### Changed
- **System-Wide T300 Standardization**: The "T300" tuning is now the project-wide baseline for all force-related defaults.
    - **Startup Defaults**: Updated the FFB engine to initialize with T300 values (Gain=1.0, Understeer=38.0, MaxTorque=100Nm) so the app is optimized for belt-driven wheels on the very first run.
    - **Preset Template**: Updated the `Preset` structure so that newly created user presets inherit T300 values instead of legacy defaults.
    - **Test & Guide Presets**: Updated all 15 built-in Test and Guide presets to use T300-standard intensities. For example, "Guide: Understeer" now uses 38.0 intensity to ensure the effect is clearly perceptible on all hardware.
    - **Renaming**: Renamed the primary preset to **"Default (T300)"**.
- **Newtonian Force Rebalancing (SoP Scale)**: Adjusted SoP (Lateral G) scaling to resolve the "100 Nm scaling issue" and improve texture visibility.
    - **Balanced Default**: Changed default `sop_scale` from 20.0 to **5.0**. This produces ~10Nm of force at 2G, which is a strong but reasonable overlay relative to the car's base steering weight (~20Nm).
    - **Texture Protection**: Lowering the SoP scale allows users to lower their `Max Torque Ref` (e.g., to 30-40 Nm). This "zooms in" on micro-forces like Slide Rumble, making them much more perceptible on belt-driven wheels.
    - **Slider Range Refinement**: Reduced the **SoP Scale** GUI slider maximum from 200.0 to **20.0**. The previous range was disproportionately large for the new Newtonian math.
    - **Calibration Tooltip**: Added a tooltip to the SoP Scale slider explaining the math: *"5.0 = Balanced (10Nm at 2G), 20.0 = Heavy (40Nm at 2G)."*
    - **Preset Synchronization**: Updated all built-in Test and Guide presets that use SoP to use the new 5.0 scale baseline.
- **Enhanced Testing Guide**: Significantly expanded `docs\Driver's Guide to Testing LMUFFB.md` to help users verify FFB effects more effectively.
    - Added **"Extreme Car Setup"** recommendations for every test (e.g., maximum stiffness, specific brake bias, extreme tire pressures) to isolate and amplify specific physics behaviors.
    - Standardized terminology on the new **"Default (T300)"** baseline.
    - Recommended the **Porsche 911 GTE** at **Paul Ricard** as the primary reference car/track combination for testing.
    - Improved instructions for ABS, Traction Loss, and SoP Yaw tests with car-setup-specific advice.

### Fixed
- **Reset Defaults Synchronization**: Refactored the "Reset Defaults" button in the GUI. It now correctly applies the modern "Default (T300)" preset instead of using legacy hardcoded values from v0.3.13. This fixes the issue where clicking Reset would erroneously set Understeer to 1.0.
- **Unit Test Suite Synchronization**: Updated `tests\test_ffb_engine.cpp` to align with the new T300 default configurations.
    - Updated `test_preset_initialization` to expect the renamed "Default (T300)" preset.
    - Added explicit `engine.m_invert_force = false` to all coordinate system regression tests to ensure physics validation is independent of application-level inversion defaults.
    - Adjusted `test_grip_modulation` and `test_rear_force_workaround` logic to account for updated default intensities, ensuring no false-positive test failures.
    - Verified all 123 tests pass with the new default state.

## [0.4.31] - 2025-12-20

## [0.4.30] - 2025-12-20
### Fixed
- **SoP (Lateral G) Direction Inversion**: Fixed the SoP (lateral G) effect pulling in the wrong direction, causing it to fight against Base Torque and Rear Align Torque.
    - Removed the sign inversion introduced in v0.4.19.
    - **Root Cause**: SoP was inverted to match DirectInput coordinates, but the internal engine actually uses Game Coordinate System (+ = Left). Base Torque and Rear Align Torque were already aligned correctly.
    - **Impact**: In the reported screenshots, SoP was pulling into the turn (-10.6 Nm) when it should have been adding counter-steering weight (+10.6 Nm). This fix resolves the instability where SoP fought against the base aligning torque.
    - **Telemetry Analysis**: Confirmed that `mLocalAccel.x` aligns correctly with the desired FFB direction without inversion.
    - **Note**: Yaw Kick (v0.4.20) remains inverted as manual testing confirmed it provides correct counter-steering behavior.

## [0.4.29] - 2025-12-20
### Added
- **Saveable Custom Presets**: Users can now save their custom FFB configurations as named presets that persist across sessions.
    - Added text input field and "Save as New Preset" button in the GUI
    - Presets are saved to `config.ini` under `[Presets]` section
    - User presets appear in the preset dropdown alongside built-in presets
    - Overwriting existing user presets is supported (built-in presets are protected)
    - Auto-selects newly created preset after saving
    - **Implementation**: Added `is_builtin` flag to distinguish built-in from user presets, `UpdateFromEngine()` method to capture current settings, and `AddUserPreset()` method to manage user presets

- **Dirty State Tracking**: Preset dropdown now displays "Custom" when any setting is modified.
    - Implemented helper lambdas (`FloatSetting`, `BoolSetting`, `IntSetting`) that automatically detect changes
    - Prevents confusion about which preset is actually active
    - The moment any slider or checkbox is touched, the display switches to "Custom"
    - Loading a preset resets the state to show the preset name

### Changed
- **Oversteer Effect Range Expansion**: Unlocked slider ranges for oversteer-related effects to compensate for high `Max Torque Ref` values on belt-driven wheels:
    - **SoP (Lateral G)**: 2.0 → **20.0** (10x increase)
    - **SoP Yaw (Kick)**: 2.0 → **20.0** (10x increase)
    - **Oversteer Boost**: 1.0 → **20.0** (20x increase)
    - **Rear Align Torque**: 2.0 → **20.0** (10x increase)
    - **Rationale**: With `Max Torque Ref` at 100Nm (for T300), signals are compressed to ~4% of range. Default effect values (0.15-2.0) produce forces of only 0.3-4.0 Nm, which belt friction (~0.2 Nm) masks. The 10-20x multipliers compensate for this compression.

- **T300 Preset Enhancement**: Updated the T300 preset with boosted oversteer values for a complete, balanced FFB experience:
    - **SoP (Lateral G)**: 0.0 → **5.0** (feel lateral weight transfer)
    - **Rear Align Torque**: 0.0 → **15.0** (strong counter-steer pull during oversteer)
    - **Oversteer Boost**: 0.0 → **2.0** (amplification during rear slip)
    - **SoP Yaw (Kick)**: 0.0 → **5.0** (predictive rotation cue)
    - The T300 preset now provides both understeer detection (38.0) and oversteer detection (15.0) with properly scaled forces

### Technical Details
- **Config.h**: Added `is_builtin` flag, updated constructors, added `UpdateFromEngine()` and `AddUserPreset()` methods
- **Config.cpp**: Marked all 17 built-in presets with `is_builtin = true`, implemented user preset loading/saving logic, added `gyro_gain` parsing
- **GuiLayer.cpp**: Added preset save UI, implemented dirty state tracking with helper lambdas

## [0.4.28] - 2025-12-19
### Added
- **New Preset: T300**: Added "T300" preset tuned specifically for Thrustmaster T300RS wheels.
    - Features high `Max Torque Ref` (100Nm) and aggressive `Understeer Effect` (38.0) to overcome belt friction and provide clear grip loss cues.
    - `Invert FFB` enabled by default for this preset.
    - Positioned as preset #2 for easy access.

### Changed
- **Understeer Range Expansion**: Increased maximum `Understeer Effect` slider range from 10.0 to **50.0** to allow for "Binary Grip Switch" behavior on belt-driven wheels.
    - **Physics Explanation**: Belt-driven wheels (T300, G29) have internal friction that masks subtle force changes. At high Max Torque Ref values (e.g., 100Nm), the signal is compressed to ~4% of range, making small percentage drops imperceptible.
    - **Solution**: Values of 20.0-50.0 create a binary effect where grip loss causes an instant drop to zero force, which is strong enough to overcome belt friction.
    - Updated tooltip to explain: "High values (10-50) create a 'Binary' drop for belt-driven wheels."
- **Default Values**: Updated default preset values for better out-of-box experience:
    - `Max Torque Ref`: 40.0 → **60.0 Nm** (lighter default feel, safer for T300/G29 users)
    - `Understeer Effect`: 1.0 → **2.0** (more pronounced grip drop for better communication)

## [0.4.27] - 2025-12-19
### Fixed
- **CRITICAL SAFETY: FFB Mute During Pause/Menu**: Fixed a dangerous bug where the steering wheel would maintain the last force command indefinitely when the game was paused or in menu states.
    - **Problem**: DirectInput drivers are stateful and hold the last command they receive. If you paused mid-corner while the force was 10Nm, the wheel would keep pulling at 10Nm indefinitely, potentially causing injury or equipment damage.
    - **Solution**: Restructured the FFB loop logic to explicitly send a **zero force command** whenever the game is not in "Realtime" (driving) mode. Added a `should_output` flag to track whether FFB calculation should be active.
    - **Impact**: The wheel now immediately releases all tension when you pause the game or enter menus, making the application safe to use in all scenarios.
    - **Technical Details**:
        - Moved force calculation inside a conditional block that checks `in_realtime && playerHasVehicle`
        - Added explicit zero force assignment when `should_output` is false (lines 86-89 in main.cpp)
        - Enhanced console logging to show "(FFB Muted)" message when exiting to menu
    - **User Experience**: Console now logs "[Game] User exited to menu (FFB Muted)" and "[Game] User entered driving session" for clear state visibility.

## [0.4.26] - 2025-12-19
### Fixed
- **Debug Window: Crisp Text Rendering**: Fixed blurry text in plot overlays by removing font scaling (was 70% size, now full resolution). All numerical readouts are now sharp and readable.
- **Debug Window: Missing Readouts**: Added numerical readouts to multi-line plots that were bypassing the `PlotWithStats` helper:
    - **Calc Load (Front/Rear)**: Now displays `Front: XXX N | Rear: XXX N` above the overlaid cyan/magenta plot.
    - **Combined Input (Throttle/Brake)**: Now displays `Thr: X.XX | Brk: X.XX` next to the title for the overlaid green/red plot.

### Changed
- **Tuning Window: Max Torque Ref Range**: Unlocked the lower bound of the `Max Torque Ref` slider from 10.0 Nm to **1.0 Nm** (range now 1.0-100.0 Nm). This provides users with very strong wheels or specific tuning preferences more flexibility for fine-tuning FFB scaling.

## [0.4.25] - 2025-12-19
### Added
- **New Guide Presets**: Added isolation presets for advanced effects:
    - **Guide: SoP Yaw (Kick)**: Isolates the yaw acceleration impulse (mutes base force).
    - **Guide: Gyroscopic Damping**: Isolates the speed-dependent damping force (mutes base force).
- **Documentation**: Updated `Driver's Guide to Testing LMUFFB.md` with test procedures for Yaw Kick and Gyro Damping.

## [0.4.24] - 2025-12-19
### Added
- **Guide Presets**: Added 5 new built-in presets corresponding to the "Driver's Guide to Testing LMUFFB".
    - **Guide: Understeer (Front Grip)**: Isolates the grip modulation effect.
    - **Guide: Oversteer (Rear Grip)**: Isolates SoP and Rear Aligning Torque.
    - **Guide: Slide Texture (Scrub)**: Isolates the scrubbing vibration (mutes base force).
    - **Guide: Braking Lockup**: Isolates the lockup vibration (mutes base force).
    - **Guide: Traction Loss (Spin)**: Isolates the wheel spin vibration (mutes base force).
    - These presets allow users to quickly configure the app for the specific test scenarios described in the documentation.

## [0.4.23] - 2025-12-19
### Changed
- **Debug Window: Compact Plot Redesign**: Redesigned troubleshooting graphs to be more space-efficient.
    - **Overlay Statistics**: Numerical readouts (Current, Min, Max) are now overlaid directly on the plots as a legend, instead of being appended to the title.
    - **Vertical Layout**: Moved plot titles to their own lines above the graphs, significantly reducing the minimum window width required to see all data.
    - **Enhanced Readability**: Added semi-transparent black backgrounds to the overlaid statistics to ensure they are readable against any graph color.
    - **Optimized UI**: Plots now take up less horizontal space, allowing more detailed monitoring on standard monitors.

## [0.4.22] - 2025-12-19
### Added
- **Exclusive Device Acquisition Visibility**: Implemented visual feedback to show whether LMUFFB successfully acquired the FFB device in exclusive mode or is sharing it with other applications.
    - **Acquisition Strategy**: LMUFFB now attempts to acquire devices in Exclusive mode first (`DISCL_EXCLUSIVE | DISCL_BACKGROUND`), automatically falling back to Non-Exclusive mode (`DISCL_NONEXCLUSIVE | DISCL_BACKGROUND`) if exclusive access is denied.
    - **GUI Status Display**: Added color-coded acquisition mode indicator in the Tuning Window:
        - **Green "Mode: EXCLUSIVE (Game FFB Blocked)"**: LMUFFB has exclusive control. The game can read steering inputs but cannot send FFB commands, automatically preventing "Double FFB" conflicts.
        - **Yellow "Mode: SHARED (Potential Conflict)"**: LMUFFB is sharing the device. Users must manually disable in-game FFB to avoid conflicting force signals.
    - **Informative Tooltips**: Hover over the mode indicator for detailed explanations and recommended actions.
    - **Technical Details**: Added `IsExclusive()` method and `m_isExclusive` member to `DirectInputFFB` class to track acquisition state. Updated `SelectDevice()` to implement exclusive-first strategy with proper state tracking.
    - **Benefits**:
        - Automatic conflict prevention when exclusive mode succeeds
        - Clear visibility of potential FFB conflicts
        - Better troubleshooting for "Double FFB" issues
        - No manual configuration needed when exclusive mode is acquired
    - **Documentation**: Added comprehensive user guide (`docs/EXCLUSIVE_ACQUISITION_GUIDE.md`) and technical implementation summary (`docs/dev_docs/implementation_summary_exclusive_acquisition.md`).

## [0.4.21] - 2025-12-19
### Added
- **Debug Window: Numerical Readouts**: Added precise numerical diagnostics to all troubleshooting graphs. Each plot now displays:
    - **Current Value**: The most recent value (4 decimal precision for detecting tiny values like 0.0015)
    - **Min**: Minimum value in the 10-second history buffer
    - **Max**: Maximum value in the 10-second history buffer
    - **Purpose**: Diagnose "flatlined" channels to determine if values are truly zero (logic bug) or just very small (scaling issue). Essential for troubleshooting effects like SoP, Understeer, and Road Texture that may appear dead but are actually producing micro-forces.

## [0.4.20] - 2025-12-19
### Fixed
- **CRITICAL: Positive Feedback Loop in Scrub Drag and Yaw Kick**: Fixed two force direction inversions that were causing the wheel to pull in the direction of the turn/slide instead of resisting it, creating unstable positive feedback loops.
    - **Scrub Drag**: Inverted force direction to provide counter-steering (stabilizing) torque. Previously, when sliding left, the force would push left (amplifying the slide). Now it pulls left (resisting the slide).
        - **Fix**: Changed `drag_dir = (avg_lat_vel > 0.0) ? 1.0 : -1.0` to `drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0` in FFBEngine.h line 858.
        - **Impact**: Lateral slides now feel properly damped with stabilizing counter-steering torque.
    - **Yaw Kick**: Inverted force direction to provide predictive counter-steering cue. Previously, when rotating right, the force would pull right (amplifying rotation). Now it pulls left (counter-steering).
        - **Fix**: Changed `yaw_force = m_yaw_accel_smoothed * m_sop_yaw_gain * 5.0` to `yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * 5.0` in FFBEngine.h line 702.
        - **Impact**: Yaw acceleration now provides natural counter-steering cues that help stabilize the car during rotation.
    - **Root Cause**: Both effects were not accounting for the coordinate system mismatch between rFactor 2/LMU (left-handed, +X = left) and DirectInput (+Force = right). The fixes ensure forces provide negative feedback (stability) instead of positive feedback (instability).

### Added
- **New Test**: `test_sop_yaw_kick_direction()` to verify that positive yaw acceleration produces negative FFB output (counter-steering).

### Changed
- **Updated Test**: `test_coordinate_scrub_drag_direction()` now verifies that the Scrub Drag force provides counter-steering torque (left slide → left pull) instead of the previous incorrect behavior (left slide → right push).

## [0.4.19] - 2025-12-16
### Fixed
- **CRITICAL: Coordinate System Inversions**: Fixed three fundamental bugs caused by mismatched coordinate systems between rFactor 2/LMU (left-handed, +X = left) and DirectInput (standard, +X = right). These inversions caused FFB effects to fight the physics instead of helping, creating positive feedback loops and unstable behavior.
    - **Seat of Pants (SoP)**: Inverted lateral G calculation to match DirectInput convention. Previously, in a right turn, SoP would lighten the wheel instead of making it heavy, fighting against the natural aligning torque.
        - **Fix**: Changed `lat_g = raw_g / 9.81` to `lat_g = -(raw_g / 9.81)` in FFBEngine.h line 571.
        - **Impact**: Steering now feels properly weighted in corners, with the wheel pulling in the correct direction to simulate load transfer.
    - **Rear Aligning Torque**: Inverted calculated rear lateral force AND fixed slip angle calculation to provide counter-steering (restoring) torque in BOTH directions. This was the root cause of the user-reported bug: "Slide rumble throws the wheel in the direction I am turning."
        - **Problem**: When the rear slid left during oversteer, the torque would pull the wheel RIGHT (into the slide), creating a catastrophic positive feedback loop that made the car uncontrollable.
        - **Fix 1**: Changed `rear_torque = calc_rear_lat_force * ...` to `rear_torque = -calc_rear_lat_force * ...` in FFBEngine.h line 666.
        - **Fix 2 (CRITICAL)**: Removed `std::abs()` from slip angle calculation (line 315) to preserve sign information. Changed `std::atan2(std::abs(w.mLateralPatchVel), v_long)` to `std::atan2(w.mLateralPatchVel, v_long)`.
        - **Impact**: Oversteer now provides natural counter-steering cues in BOTH left and right turns, making the car stable and predictable. The initial fix only worked for right turns; the slip angle fix ensures left turns also get proper counter-steering.
    - **Scrub Drag**: Fixed direction to oppose motion instead of amplifying it. Previously acted as negative damping, pushing the car faster into slides.
        - **Problem**: When sliding left, friction would push LEFT (same direction), accelerating the slide instead of resisting it.
        - **Fix**: Changed `drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0` to `drag_dir = (avg_lat_vel > 0.0) ? 1.0 : -1.0` in FFBEngine.h line 840.
        - **Impact**: Lateral slides now feel properly damped, with friction resisting the motion as expected.

### Added
- **Comprehensive Regression Tests**: Added four new test functions to prevent recurrence of coordinate system bugs:
    - `test_coordinate_sop_inversion()`: Verifies SoP pulls in the correct direction for left and right turns.
    - `test_coordinate_rear_torque_inversion()`: Verifies rear torque provides counter-steering during oversteer.
    - `test_coordinate_scrub_drag_direction()`: Verifies friction opposes slide direction.
    - `test_regression_no_positive_feedback()`: Simulates the original bug scenario (right turn with oversteer) and verifies all forces work together instead of fighting.

### Technical Details
- **Root Cause**: The rFactor 2/LMU physics engine uses a left-handed coordinate system where +X points to the driver's left, while DirectInput uses the standard convention where +Force means right. Without proper sign inversions, lateral vectors (position, velocity, acceleration, force) are mathematically inverted relative to the wheel's expectation.
- **Documentation**: See `docs/bug_reports/wrong rf2 coordinates use.md` for detailed analysis and derivation of the fixes.


### Fixed
- **Critical Stability Issue**: Fixed a noise feedback loop between Slide Rumble and Yaw Kick effects that caused violent wheel behavior.
    - **Problem**: Slide Rumble vibrations caused the yaw acceleration telemetry (a derivative value) to spike with high-frequency noise. The Yaw Kick effect amplified these spikes, creating a positive feedback loop where the wheel would shake increasingly harder and feel like it was "fighting" the user.
    - **Solution**: Implemented a Low Pass Filter (Exponential Moving Average with alpha=0.1) on the yaw acceleration data before calculating the Yaw Kick force. This filters out high-frequency vibration noise while preserving the low-frequency "actual rotation kick" signal.
    - **Impact**: Users can now safely use Slide Rumble and Yaw Kick effects simultaneously without experiencing unstable or violent FFB behavior.
    - **Technical Details**: Added `m_yaw_accel_smoothed` state variable to `FFBEngine` class. The filter uses 10% new data and 90% history, effectively removing noise above ~1.6 Hz while keeping the intended rotation cues intact.

## [0.4.17] - 2025-12-15
### Added
- **Synthetic Gyroscopic Damping**: Implemented stabilization effect to prevent "tank slappers" during drifts.
    - Added `Gyroscopic Damping` slider (0.0 - 1.0) to Tuning Window.
    - Added "Gyro Damping" trace to Debug Window FFB Components graph.
    - Force opposes rapid steering movements and scales with car speed.
    - Uses Low Pass Filter (LPF) to smooth noisy steering velocity derivative.
    - Added `m_gyro_gain` and `m_gyro_smoothing` settings to configuration system.

### Changed
- **Physics Engine**: Updated total force calculation to include gyroscopic damping component.
- **Documentation**: Updated `FFB_formulas.md` with gyroscopic damping formula and tuning parameter.

### Testing
- Added `test_gyro_damping()` unit test to verify force direction and magnitude.


## [0.4.16] - 2025-12-15
### Added
- **SoP Yaw Kick**: Implemented "Yaw Acceleration Injection" to provide a predictive kick when rotation starts.
    - Added `m_sop_yaw_gain` slider (0.0 - 2.0) to Tuning Window.
    - Added "Yaw Kick" trace to Debug Window.
    - Updated physics engine to mix Yaw Acceleration with Lateral G-Force in SoP calculation.

## [0.4.15] - 2025-12-15
### Changed
- **User Experience Improvements**: Removed all vJoy and Joystick Gremlin-related annoyances for users.
    - **Removed Startup Popups**: Eliminated vJoy DLL not found error popup, vJoy version mismatch warning, and rF2 shared memory plugin conflict warning. The app now starts silently without bothering users about optional components.
    - **Simplified Documentation**: Completely rewrote `README.md` and `README.txt` to focus on DirectInput-only setup. Removed all references to vJoy installation, Joystick Gremlin configuration, and rFactor 2 shared memory plugin setup.
    - **Streamlined Setup**: Installation now requires only: (1) Reduce wheel strength in device driver, (2) Configure LMU to disable in-game FFB, (3) Select your wheel in lmuFFB. No third-party tools needed.

### Technical Notes
- vJoy code infrastructure remains in place for backward compatibility and potential future use, but runs silently without user interaction
- Existing config files with vJoy settings will continue to work without errors
- This is a user-facing cleanup only; complete code removal is planned for v0.5.0+

## [0.4.14] - 2025-12-14
### Fixed
- **Critical Physics Instability**: Fixed a major bug where physics state variables (Slip Angle, Road Texture history, Bottoming history) were only updated conditionally. This caused violent "reverse FFB" kicks and spikes when effects were toggled or when telemetry dropped frames.
    - Moved `calculate_slip_angle` outside the conditional block in `calculate_grip` to ensure LPF state is always current.
    - Moved `m_prev_vert_deflection` and `m_prev_susp_force` updates to the end of `calculate_force` to ensure unconditional updates.
- **Refactoring**: Updated `Config` system to use the Fluent Builder Pattern for cleaner preset definitions.

### Added
- **Regression Tests**: Added a suite of regression tests (`test_regression_road_texture_toggle`, `test_regression_bottoming_switch`, `test_regression_rear_torque_lpf`) to prevent recurrence of state-related bugs.
- **Stress Test**: Added a fuzzing test (`test_stress_stability`) to ensure stability under random inputs.

## [0.4.13] - 2025-12-14
### Added
- **Base Force Debugging Tools**: Added advanced controls for isolating and tuning the primary steering force.
    - **Steering Shaft Gain**: A slider to attenuate the raw game force (Base Force) without affecting the telemetry data used by other effects (like Oversteer Boost). Useful if the base FFB is too strong but you want to keep effect calculations accurate.
    - **Base Force Mode**: A new selector in "Advanced Tuning" to change how the base force is generated:
        - **Native (Physics)**: Uses raw game physics (Default).
        - **Synthetic (Constant)**: Uses a constant force with the game's direction. Isolates the Grip Modulation effect from suspension noise for precise tuning.
        - **Muted (Off)**: Forces zero output. Useful for testing SoP or Texture effects in isolation.
- **Updated Presets**: Updated built-in presets to use the new Debug Modes (e.g., "Test: SoP Only" now uses "Muted" mode for cleaner isolation).

### Changed
- **Preset Structure**: Updated `config.ini` format to include `steering_shaft_gain` and `base_force_mode`. Old config files will be automatically upgraded.

## [0.4.12] - 2025-12-14
### Added
- **Screenshot Feature**: Added "Save Screenshot" button to the Tuning Window. Saves PNG files with timestamps to the application directory using `stb_image_write.h` and DirectX 11 buffer mapping.
- **New Test Preset**: Added "Test: No Effects" preset (Gain 1.0, all effects 0.0) to verify zero signal leakage.
- **Verification Tests**: Added `test_zero_effects_leakage` to the test suite to ensure no ghost forces persist when effects are disabled.

### Changed
- **Physics Tuning**:
    - **Grip Calculation**: Tightened optimal slip angle threshold from `0.15` (8.5 deg) to **`0.10` (5.7 deg)** and increased falloff multiplier from `2.0` to **`4.0`**. This makes grip loss start earlier and drop off faster, reducing the "on/off" feeling.
- **GUI Organization**: Completely reorganized the Troubleshooting Graphs (Debug Window) into three logical groups for better usability:
    - **Header A (Output)**: Main Forces, Modifiers, Textures.
    - **Header B (Brain)**: Internal Physics (Loads, Grip/Slip, Forces).
    - **Header C (Input)**: Raw Game Telemetry (Driver Input, Vehicle State, Tire Data, Velocities).
- **Code Structure**: Moved `vendor/stb_image_write.h` to `src/stb_image_write.h` for simpler inclusion.

## [0.4.11] - 2025-12-13
### Added
- **Rear Align Torque Slider**: Added a dedicated slider for `Rear Align Torque` (0.0-2.0) to the GUI. This decouples the rear-end force from the generic `Oversteer Boost`, allowing independent tuning.
- **New Presets**: Added "Test: Rear Align Torque Only", "Test: SoP Base Only", and "Test: Slide Texture Only" to the configuration dropdown for easier troubleshooting.

### Changed
- **Physics Tuning**: Adjusted coefficients to produce meaningful forces in the Newton-meter domain.
    - **Rear Align Torque**: Increased coefficient 4x (0.00025 -> 0.001) to boost max torque from ~1.5 Nm to ~6.0 Nm.
    - **Scrub Drag**: Increased base multiplier from 2.0 to 5.0.
    - **Road Texture**: Increased base multiplier from 25.0 to 50.0.
- **GUI Visualization**: "Zoomed in" the Y-axis scale for micro-texture plots (Road, Slide, Vibrations) from ±20.0 to **±10.0** for better visibility of subtle effects.
- **Documentation**: Updated `FFB_formulas.md` with the new coefficients.

## [0.4.10] - 2025-12-13
### Added
- **Rear Physics Workaround**: Implemented a calculation fallback for Rear Aligning Torque to address the LMU 1.2 API issue where `mLateralForce` reports 0.0 for rear tires.
    - **Logic**: Approximates rear load from suspension force (+300N) and calculates lateral force using `RearSlipAngle * CalculatedLoad * Stiffness(15.0)`.
    - **Visualization**: Added `Calc Rear Lat Force` to the Telemetry Inspector graph (Header C) to visualize the workaround output.
    - **Safety**: Clamped the calculated rear lateral force to ±6000N to prevent physics explosions.
- **GUI Improvements**:
    - **Multi-line Plots**: Updated Header B "Calc Load" graph to show both Front (Cyan) and Rear (Magenta) calculated loads simultaneously.
    - **Slider Fix**: Corrected `SoP Scale` slider range to `0.0 - 200.0` (was 100-5000), allowing proper tuning for the new Nm-based math.
    - **Plot Scaling**: Updated all FFB Component plots to use a **±20.0 Nm** scale (instead of ±1000N) to match the engine's output units, fixing "flat line" graphs.

### Changed
- **Defaults**: Increased default `SoP Scale` from 5.0 to **20.0** to provide a perceptible baseline force given the new Nm scaling.
- **Documentation**: Updated `FFB_formulas.md` to document the new Rear Force Workaround logic and updated scaling constants.

## [0.4.9] - 2025-12-11
### Added
- **Finalized Troubleshooting Graphs**: Updated the internal FFB Engine and GUI to expose deeper physics data for debugging.
    - **Rear Tire Physics**: Added visualization for `Rear Slip Angle (Smoothed)` and `Rear Slip Angle (Raw)` to troubleshoot oversteer/SoP logic.
    - **Combined Slip Plot**: Merged `Calc Front Slip Ratio` and `Raw Front Slip Ratio` into a single combined plot (Cyan=Game, Magenta=Calc) for easier comparison.
    - **Patch Velocities**: Added explicit plots for `Avg Front Long PatchVel`, `Avg Rear Lat PatchVel`, and `Avg Rear Long PatchVel` to help diagnose slide/spin effects.
- **Explicit Naming**: Updated documentation formulas to be explicit about Front vs Rear variables (e.g., `Front_Load_Factor`, `Front_Grip_Avg`).

### Changed
- **GUI Labels**: Renamed `Raw Rear Lat Force` to `Avg Rear Lat Force` in the Telemetry Inspector.

## [0.4.7] - 2025-12-11
### Added
- **Expanded Troubleshooting Graphs**: Major reorganization of the "Troubleshooting" window to facilitate physics debugging.
    - **New Layout**: Organized plots into three collapsible headers: "FFB Components (Output)", "Internal Physics (Calculated)", and "Raw Game Telemetry (Input)".
    - **Raw Data Inspector**: Added explicit visualization of raw telemetry inputs (e.g., `raw_front_susp_force`, `raw_front_ride_height`) completely separated from internal calculations. This allows users to confirm if game data is missing/broken vs. engine calculation errors.
    - **New Channels**: Added visualizations for Rear Aligning Torque, Scrub Drag, Calculated Front Load/Grip, and Calculated Slip Ratio.
- **Diagnostics**: Expanded `FFBSnapshot` to capture raw input values before any fallback logic is applied.

## [0.4.6] - 2025-12-11
### Added
- **Stability Safeguards**: Implemented a comprehensive suite of mathematical clamps and mitigations to prevent physics instabilities.
    - **Grip Approximation Hardening**: Added Low Pass Filter (LPF) to calculated Slip Angle and a Low Speed Cutoff (< 5.0 m/s) to force full grip, preventing "parking lot jitter". Safety clamp ensures calculated grip never drops below 20%.
    - **Scrub Drag Fade-In**: Added linear fade-in window (0.0 - 0.5 m/s lateral velocity) to prevent "ping-pong" oscillation around zero.
    - **Load Clamping**: Hard-clamped the calculated Load Factor to a maximum of 2.0x (regardless of user config) to prevent violent jolts during aerodynamic load spikes or crashes.
    - **Road Texture Clamping**: Limited frame-to-frame suspension deflection delta to +/- 0.01 meters to eliminate massive force spikes during car teleports (e.g., reset to pits).
    - **SoP Input Clamping**: Clamped lateral G-force input to +/- 5G to protect against physics glitches or wall impacts.
    - **Manual Slip Trap**: Forced Slip Ratio to 0.0 when car speed is < 2.0 m/s to avoid division-by-zero singularities.

### Fixed
- **Grip Calculation**: Implemented consistent fallback logic for rear wheels when telemetry is missing (previously only front wheels had fallback).
- **Diagnostics**: Added `GripDiagnostics` struct to track grip calculation source (telemetry vs approximation) and original values.
- **Data Integrity**: Preserved original telemetry values in diagnostics even when approximation is used.
- **Refactoring**: Extracted grip calculation logic into a reusable helper function `calculate_grip` for better maintainability and consistency.
- **Tire Radius Precision**: Fixed potential integer truncation issue by explicitly casting tire radius to double before division.

## [0.4.5] - 2025-12-11
### Added
- **Manual Slip Calculation**: Added option to calculate slip ratio from wheel rotation speed vs. car speed instead of relying on game telemetry. Useful when game slip data is broken or unavailable. Accessible via "Use Manual Slip Calc" checkbox in GUI.
- **Bottoming Detection Methods**: Added two bottoming detection methods selectable via GUI combo box:
  - Method A (Scraping): Triggers when ride height < 2mm
  - Method B (Suspension Spike): Triggers on rapid suspension force changes
- **Scrub Drag Effect**: Added resistance force when sliding sideways (tire dragging). Configurable via "Scrub Drag Gain" slider (0.0-1.0).
- **Comprehensive Documentation**: Created detailed technical analysis document (`docs/dev_docs/grip_calculation_analysis_v0.4.5.md`) documenting grip calculation logic, fallback mechanisms, known issues, and recommendations for future improvements.
- **Regression Test**: Added `test_preset_initialization()` to verify all built-in presets properly initialize v0.4.5 fields, preventing uninitialized memory bugs.

### Changed
- **Preset System**: All built-in and user presets now include three new v0.4.5 fields: `use_manual_slip` (bool), `bottoming_method` (int), and `scrub_drag_gain` (float).
- **Code Documentation**: Added extensive inline comments to `FFBEngine.h` and `tests/test_ffb_engine.cpp` explaining grip calculation paths, approximation formulas, and test limitations.

### Fixed
- **Test Expectation**: Corrected `test_sanity_checks()` grip approximation test to expect `0.1` instead of `0.5`. The grip fallback mechanism applies a floor of `0.2` (20% minimum grip), not full correction to `1.0`.
- **Critical Bug - Preset Initialization**: Fixed uninitialized memory bug where all 5 built-in presets were missing initialization for v0.4.5 fields (`use_manual_slip`, `bottoming_method`, `scrub_drag_gain`). This caused undefined behavior when users selected any built-in preset. All presets now properly initialize these fields with safe defaults (false, 0, 0.0f).

### Documentation
- **Grip Calculation Analysis**: Documented two calculation paths (telemetry vs. slip angle approximation), identified inconsistencies between front and rear wheel handling, and provided recommendations for future improvements.
- **Known Issues**: Documented that rear wheels lack fallback mechanism (unlike front wheels), potentially causing false oversteer detection when rear telemetry is missing. See analysis document for details.

## [0.4.4] - 2025-12-11
### Added
- **Invert FFB Option**: Added checkbox in GUI to invert force direction for wheels that require it (e.g., Thrustmaster T300). Fixes "backwards" or "inverted" FFB feel where wheel pushes away from center instead of pulling toward it.
- **Configurable Max Torque Reference**: Exposed `max_torque_ref` parameter in GUI (Advanced Tuning section) to allow fine-tuning of force normalization. Default: 20Nm. Users with high-torque DD wheels can increase this for better dynamic range.
- **Session-Level Statistics**: Enhanced `ChannelStats` to track both session-wide min/max (persistent across entire driving session) and interval-level averages (1-second windows). Enables better telemetry diagnostics.

### Changed
- **Preset System**: All built-in and user presets now include `invert_force` (bool) and `max_torque_ref` (float) fields. Existing `config.ini` files will auto-upgrade on save.
- **DirectInput Logging**: Improved console output clarity when acquiring FFB device, now explicitly states Exclusive vs. Non-Exclusive mode success.

### Fixed
- **Test Suite Stability**: All unit tests now explicitly set `max_torque_ref = 20.0f` to prevent dependency on default values, ensuring consistent test results across configuration changes.
- **Build System**: Added `winmm.lib` to linker dependencies to fix `timeBeginPeriod` unresolved external symbol error in CMake builds.

## [0.4.3] - 2025-12-11
### Added
- **Test Coverage**: Added unit tests for Smoothing Step Response and Configuration Persistence, bringing coverage of critical physics logic to >85%.
- **Architecture**: Enhanced `ChannelStats` to support non-blocking retrieval of telemetry statistics (latching), enabling future GUI diagnostic improvements without stalling the physics thread.

## [0.4.2] - 2025-12-08
### Added
- **Configuration Presets**: Added a new "Load Preset" dropdown in the GUI with built-in presets (Default, SoP Only, Understeer Only, Textures Only) and support for user-defined presets in `config.ini`.
- **Robust Device Acquisition**: DirectInput now attempts Exclusive Mode first, falling back to Non-Exclusive mode if access is denied (fixing potential "Device Busy" errors).
- **Game State Logic**: FFB is now automatically muted when the game is in a Menu state (not driving), preventing unwanted wheel movement.
- **Connection Diagnostics**: Added a red "Game Not Connected" status and "Retry Connection" button to the GUI if shared memory is unavailable.

## [0.4.1] - 2025-12-08
### Added
- **Unbind Device**: Added a button in the GUI to release the DirectInput device without closing the app.
- **Diagnostic Logging**: Implemented non-blocking telemetry stats logging (Torque, Load, Grip, LatG) to the console every second.
- **Hysteresis Logic**: Added a stability filter for telemetry dropouts. Fallback values (e.g., for missing tire load) now only trigger after 20 frames (~50ms) of consistent missing data, preventing rapid FFB oscillation.
- **Safety**: Added rate-limited console warnings when FFB output saturates (>99%).

### Fixed
- **FFB Scaling**: Corrected all effect amplitudes to properly account for the LMU 1.2 API change from Force (Newtons) to Torque (Newton-meters) introduced in v0.4.0. This fixes the excessively strong FFB that some users may have experienced. **Users upgrading from v0.4.0 may need to increase their gain settings** (try 2-3x previous values) as the forces are now physically accurate.

## [0.4.0] - 2025-12-08
### Added
- **LMU 1.2 Support**: Refactored the entire shared memory interface to support the new Le Mans Ultimate 1.2 layout.
    - Replaced `rFactor2SMMP_Telemetry` with `LMU_Data` shared memory map.
    - Implemented mandatory Shared Memory Locking mechanism (`SharedMemoryLock`) to prevent data corruption.
    - Added Player Indexing logic to locate the correct vehicle in the 104-slot array.
- **Physics Enhancements**:
    - **Real Tire Load**: Now uses native `mTireLoad` from the new interface (replacing estimates/fallbacks).
    - **Real Grip**: Now uses native `mGripFract` for accurate understeer simulation.
    - **Real Slip Speed**: Uses `mLateralPatchVel` and `mLongitudinalPatchVel` for precise texture frequency.
- **Refactoring**:
    - Deprecated `rF2Data.h`.
    - Renamed internal steering force variable to `mSteeringShaftTorque` to match new API.

## [0.3.20] - 2025-12-08
### Fixed
- **Configurable Plot History**: Replaced the hardcoded 2.5-second buffer size for GUI plots with a configurable parameter (currently set to 10 seconds), ensuring consistent visualization regardless of frame rate.

## [0.3.19] - 2025-12-08
### Added
- **Telemetry Robustness**: Implemented sanity checks to detect and mitigate missing telemetry data.
    - **Load Fallback**: If `mTireLoad` is 0 while moving, defaults to 4000N.
    - **Grip Fallback**: If `mGripFract` is 0 but load exists, defaults to 1.0.
    - **DeltaTime Correction**: Detects invalid `dt` and defaults to 400Hz.
    - **GUI Warnings**: Added visual alerts in the Debug Window when data is missing.

## [0.3.18] - 2025-05-23
### Added
- **Decoupled Plotting**: Refactored the FFB Engine and GUI to use a Producer-Consumer pattern. This decouples the physics update rate (400Hz) from the GUI refresh rate (60Hz), allowing all physics samples to be captured and visualized without aliasing.
- **Configurable Plot History**: Plots now show a rolling history defined by a code parameter (default 10s), ensuring consistent visualization regardless of frame rate.

## [0.3.16] - 2025-05-23
### Fixed
- **vJoy Startup Check**: Fixed a logic bug where the vJoy DLL was used before verifying if the driver was enabled, potentially causing instability. Added explicit `DynamicVJoy::Get().Enabled()` check in the FFB loop.

## [0.3.13] - 2025-05-23
### Added
- **Complete FFB Visualizations**: Expanded the Troubleshooting Graphs to include individual plots for *all* FFB components (Understeer, Oversteer, Road, Slide, Lockup, Spin, Bottoming, Clipping) and 8 critical Telemetry channels.
- **Refactoring**: Split `SoP Force` (Lateral G) from `Oversteer Boost` (Rear Aligning Torque) in the internal engine debug state for clearer analysis.

## [0.3.12] - 2025-05-23
### Added
- **Visual Troubleshooting Tools**: Added real-time **Rolling Trace Plots** (Oscilloscope style) for FFB components (Base, SoP, Textures) and Telemetry inputs. Accessible via "Show Troubleshooting Graphs" in the main GUI.
- **Internal**: Refactored `FFBEngine` to expose internal calculation states for visualization.

## [0.3.11] - 2025-05-23
### Documentation
- **Direct Mode Priority**: Updated `README` to recommend "Direct Method" (binding physical wheel with 0% FFB) as the primary configuration, demoting "vJoy Bridge" to compatibility mode.
- **Feeder Clarification**: Explicitly documented that "vJoy Demo Feeder" is insufficient for driving; **Joystick Gremlin** is required if using the vJoy bridge method.

## [0.3.10] - 2025-05-23
### Fixed
- **Wheel Spinning Loop**: Implemented a safety switch (`Monitor FFB on vJoy`) which is **Disabled by default**. This prevents the app from writing FFB to vJoy Axis X, which caused a feedback loop if users bound Game Steering to that axis.
- **Steering Input Confusion**: Updated documentation to explicitly state LMUFFB does not bridge steering input; users must use external Feeders or alternative bindings.

## [0.3.9] - 2025-05-23
### Added
- **Smoothing & Caps**: Added configuration sliders for `SoP Smoothing` (Low Pass Filter) and `Load Cap` (Max Tire Load scale) in the GUI ("Advanced Tuning" section). This allows users to fine-tune signal noise vs. responsiveness.
- **Documentation**:
    - Updated `README` files with precise Le Mans Ultimate in-game settings (Force Feedback Strength 0%, Effects Off, Smoothing 0, Borderless Mode).
    - Clarified vJoy links and troubleshooting steps.

## [0.3.8] - 2025-05-23
### Added
- **vJoy Version Check**: Startup check ensures vJoy driver version is compatible (>= 2.1.8). Warnings can be suppressed via checkbox logic (persisted in config).
- **Licensing**: Added `licenses/vJoy_LICENSE.txt` to comply with MIT attribution.
- **Documentation**: Added investigation regarding bundling vJoy DLLs.

## [0.3.7] - 2025-05-23
### Added
- **Priority Check**: Implemented logic to detect if Le Mans Ultimate (LMU) has locked the FFB device in Exclusive Mode ("Double FFB"). If detected, a warning is logged to the console.
- **Documentation Updates**:
    - Updated `README.md` and `README.txt` to be LMU-specific (replaced "Game" references).
    - Clarified that LMU lacks a "None" FFB option; advised setting Strength to 0% as a workaround.
    - Updated `investigation_vjoyless_implementation.md` with LMU-specific experiments.
    - Updated `plan_troubleshooting_FFB_visualizations.md` to specify "Rolling Trace Plots" for all telemetry/physics values.

## [0.3.6] - 2025-05-23
### Documentation
- **Troubleshooting**: Added comprehensive plans for "FFB Visualizations" (`docs/plan_troubleshooting_FFB_visualizations.md`) and "Guided Configurator" (`docs/plan_guided_configurator.md`).
- **Clarification**: Updated README.md and README.txt to clarify that LMUFFB does not bridge steering input, requiring external "Feeder" tools if vJoy is used for input binding.

## [0.3.5] - 2025-05-23
### Added
- **Safety Defaults**: Changed default settings to Gain 0.5 and SOP 0.0 to prevent violent wheel jerking on first run (especially for Direct Drive wheels).
- **SoP Smoothing**: Implemented a Low Pass Filter (exponential moving average) for lateral G-force data to reduce signal noise and vibration on straights.
- **Improved Error Handling**: Added a clear popup message when `vJoyInterface.dll` is missing.
- **Documentation**:
    - Added "Guided Install Plan" and "vJoy-less Investigation" documents.
    - Updated README with critical "Double FFB" troubleshooting tips and Borderless Window warnings.

## [0.3.4] - 2025-05-23
### Added
- **Test Suite**: Significantly expanded test coverage (approx 85%) covering Oversteer Boost, Phase Wraparound, Multi-effect interactions, and Safety Clamps.

## [0.3.3] - 2025-05-23
### Fixed
- **Suspension Bottoming**: Fixed a logic bug where the bottoming effect force direction depended on current steering torque, causing unexpected pulls on straights. Now uses a 50Hz vibration pulse (crunch).

## [0.3.2] - 2025-05-23
### Added
- **Suspension Bottoming**: Added a new haptic effect that triggers when tire load exceeds thresholds (simulating bump stops/heavy compression).
- **Physics Refinement**: Updated Slide Texture to use `mLateralPatchVel` for more accurate scrubbing sensation.
- **Documentation**: Added `docs/telemetry_logging_investigation.md` for future CSV logging features.

### Optimized
- **DirectInput**: Removed redundant parameter updates and `DIEP_START` calls in the high-frequency loop to reduce driver overhead.
- **Thread Safety**: Added mutex locking to prevent race conditions when GUI modifies physics engine parameters.

## [0.3.1] - 2025-05-23
### Fixed
- **vJoy Build Issue**: Fixed an import error in `src/DynamicVJoy.h` or `main.cpp` that was causing build failures on some systems (user contribution).

## [0.3.0] - 2025-05-23
### Added
- **Dynamic Physics Engine**: Major overhaul of FFB synthesis.
    - **Phase Integration**: Solved audio-like clicking/popping in vibration effects by using phase accumulators.
    - **Advanced Telemetry**: Integrated `mLateralPatchVel` (Slide Speed) and `mTireLoad` (Vertical Load) into calculations.
    - **Dynamic Frequencies**:
        - Lockup frequency now scales with Car Speed.
        - Spin/Traction Loss frequency now scales with Slip Speed.
        - Slide Texture frequency now scales with Lateral Slide Speed.
- **Documentation**: Added `docs/implementation_report_v0.3.md`.

## [0.2.2] - 2025-05-23
### Added
- **Dynamic Effects (Initial)**:
    - **Oversteer**: Added Rear Aligning Torque integration (Counter-steer cue).
    - **Progressive Lockup**: Replaced binary on/off rumble with scaled amplitude based on slip severity.
    - **Torque Drop**: Added "Floating" sensation when traction is lost.
- **GUI**: Added sliders for Oversteer Boost, Lockup Gain, and Spin Gain.

## [0.2.0] - 2025-05-22
### Added
- **DirectInput Support**: Implemented native FFB output to physical wheels (bypassing vJoy for forces).
- **Device Selection**: Added GUI dropdown to select specific FFB devices.
- **vJoy Optionality**: Made vJoy a soft dependency via dynamic loading (`LoadLibrary`). App runs even if vJoy is missing.
- **Installer**: Added Inno Setup script (`installer/lmuffb.iss`).
- **Configuration**: Added `config.ini` persistence (Save/Load buttons).
- **Error Handling**: Added GUI Popups for missing Shared Memory.

## [0.1.0] - 2025-05-21
### Added
- **C++ Port**: Initial release of the native C++ application (replacing Python prototype).
- **Core FFB**: Basic Grip Modulation (Understeer) and Seat of Pants (SoP) effects.
- **GUI**: Initial implementation using Dear ImGui.
- **Architecture**: Multi-threaded design (400Hz FFB loop / 60Hz GUI loop).
