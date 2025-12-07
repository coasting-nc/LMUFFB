# Changelog

All notable changes to this project will be documented in this file.

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
