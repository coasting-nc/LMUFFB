# Changelog

All notable changes to this project will be documented in this file.

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