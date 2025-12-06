# Changelog

All notable changes to this project will be documented in this file.

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
