# lmuFFB App Version Releases by ErwinMoss

This document contains all version release posts by ErwinMoss from the [lmuFFB App thread](https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/) on Le Mans Ultimate Community.

---

## Post #1 - July 24, 2025
**Initial Thread Creation**

lmuFFB early version released: https://github.com/coasting-nc/LMUFFB/releases/

Some of the FFB formulas might need refinements. Adjust the FFB configuration options in lmuFFB to see which of the currently supported effects you prefer.

Post your screenshots of the Graphs, the main lmuFFB window and the console here if you can (use the Save Screenshot button to capture them all).

Your testing and feedback is greatly appreciated!

Project homepage: https://github.com/coasting-nc/LMUFFB
Download latest version: https://github.com/coasting-nc/LMUFFB/releases/

---

## Post #10 - December 6, 2025
**Early Version Release**

I've created an early version of LMUFFB: https://github.com/coasting-nc/LMUFFB/releases

It needs vJoy + the rF2 shared-memory plugin. I haven't fully tested it yet (sorting my wheel), so please give it a spin and drop any bugs/feedback here or on GitHub. Thanks!

---

## Post #41 - December 9, 2025
**Version v0.4.0b - LMU 1.2 Support Added**

Support for LMU 1.2 added, give it a try: https://github.com/coasting-nc/LMUFFB/releases/tag/v0.4.0b

Remember that this is an experimental version, for safety you should first lower you wheel max strength from your device driver configurator, since the app might cause strong force spikes and oscillations. Eg. 10 % max force for DDs, 20-30 % for gear/belt driven wheels.

The app probably still needs refinements to the FFB formulas. Adjust the FFB configuration options in lmuFFB to see if you manage to get a workable FFB signal.
Post your screenshots of the Troubleshooting Graphs window and the main lmuFFB window here if you can.

Your testing and feedback is greatly appreciated!

---

## December 11, 2025
**Version v0.4.4b**

New release: https://github.com/coasting-nc/LMUFFB/releases/tag/v0.4.4b

Updates to install instructions:
* vJoy might not be necessary, ignore the warning. Just set the ingame FFB to 0% and off.
* If FFB feels "backwards" or "inverted" (wheel pushes away from center instead of pulling toward it), check or uncheck the "Invert FFB" checkbox in the lmuFFB GUI to reverse the force direction
* See README.txt for the rest of the instructions

Known issues:
You will see warnings about Missing Tire Load and Missing Grip Fraction data detected. This is EXPECTED and NOT a bug in lmuFFB. This is a BUG IN LMU 1.2 - the game is currently returning ZERO (0) for all tire load and grip fraction values, even though the shared memory interface includes these fields.

---

## December 15, 2025
**Version v0.4.15+ - Major Stability Fix**

New release (v0.4.15+) https://github.com/coasting-nc/LMUFFB/releases

This should fix the FFB instability issues (eg. intermittent inverse FFB).
It also removes the messages and instructions about vJoy and the rFactor 2 shared memory dll, since vJoy is no longer needed, and there are no problems if you keep the shared memory dll in the game.

Full changelogs: https://github.com/coasting-nc/LMUFFB/blob/main/CHANGELOG_DEV.md
 
---

## December 15, 2025
**Version v0.4.16 - Yaw Kick Effect**

New release (v0.4.16) https://github.com/coasting-nc/LMUFFB/releases

This introduces a new Yaw Kick effect within the Seat of Pants effect, that tells when the rear starts to step out. This should help catch early slides.

---

## December 16, 2025
**Version v0.4.18 - Gyroscopic Damping**

New release (v0.4.18) https://github.com/coasting-nc/LMUFFB/releases

Added Gyroscopic Damping (stabilization effect to prevent "tank slappers" during drifts), and smoothing to the Yaw Kick effect.

---

## December 16, 2025
**Version v0.4.19 - Coordinate System Fix**

New release (v0.4.19) https://github.com/coasting-nc/LMUFFB/releases

This fixes a few bugs on the use of rF2 / LMU coordinate system. This should fix the issue of the broken slide rumble.

---

## December 19, 2025
**Version v0.4.20 - Force Direction Fixes**

New release (v0.4.20) https://github.com/coasting-nc/LMUFFB/releases

Fixed two force direction inversions that were causing the wheel to pull in the direction of the turn/slide instead of resisting it, creating unstable positive feedback loops (Positive Feedback Loop in Scrub Drag and Yaw Kick).

Let me know if this new version fixes the issue with the slide rumble. If you still get this or some other issue, please post a screenshot of the UI with at least the main window with the settings (you can use the save screenshot button for convenience).

---

## December 19, 2025
**Version v0.4.24 - Guide Presets**

New release (v0.4.24) https://github.com/coasting-nc/LMUFFB/releases

Note: make sure to check you have in-game FFB disabled and strength set to 0%, since after an LMU update these values can get enabled again.

Added **Guide Presets**: Added 5 new built-in presets corresponding to the "Driver's Guide to Testing LMUFFB".
* Guide: Understeer (Front Grip): Isolates the grip modulation effect.
* Guide: Oversteer (Rear Grip): Isolates SoP and Rear Aligning Torque.
* Guide: Slide Texture (Scrub): Isolates the scrubbing vibration (mutes base force).
* Guide: Braking Lockup: Isolates the lockup vibration (mutes base force).
* Guide: Traction Loss (Spin): Isolates the wheel spin vibration (mutes base force).

These presets allow users to quickly configure the app for the specific test scenarios described in the documentation.

---

## December 20, 2025
**Version v0.4.31 - T300 Preset**

New release (v0.4.31) https://github.com/coasting-nc/LMUFFB/releases

Fixed inverted SoP (Lateral G) effect, and added a new T300 preset that has working default values for understeer and oversteer / SoP effects.

---

## December 20, 2025
**Version v0.4.37 - Slide Texture Overhaul**

New release (v0.4.37) https://github.com/coasting-nc/LMUFFB/releases

Fixed Slide Rumble bug (due to "phase explosion" issue that caused massive constant force pulls during frame stutters).

**Slide Texture Overhaul**: Now detects rear slip (drifting/donuts), uses optimized frequencies (10-70Hz) for better rumble on belt wheels, and adds a manual Pitch slider.

**Tuning**: All defaults and presets have been re-calibrated around a T300 baseline

---

## December 20, 2025
**Version v0.4.38 - Frame Stutter Fixes**

New release (v0.4.38) https://github.com/coasting-nc/LMUFFB/releases

More fixes to ensure consistent FFB even in case of physics frames stuttering.

---

## December 20, 2025
**Version v0.4.39 - Improved Tyre Physics**

New release (v0.4.39) https://github.com/coasting-nc/LMUFFB/releases

Implemented better approximations for tyre grip and load.

---

## December 21, 2025
**Version v0.4.43 - Latency Reduction & Flatspot Suppression**

New release (v0.4.43) https://github.com/coasting-nc/LMUFFB/releases

Fixed "delay" in FFB: reduced latency to < 15ms due to SoP Smoothing and Slip Angle Smoothing

Added sliders to adjust SoP Smoothing and Slip Angle Smoothing

**Eliminating LMU tyre flatspot vibration**:
* Added Dynamic Notch Filter (Flatspot Suppression) with Dynamic Suppression Strength
* Added Static Notch Filter
* See this guide on how to use them: Dynamic Flatspot Suppression - User Guide

Fixes to Yaw Kick noise: Muted for low forces and low speeds.

---

## December 21, 2025
**Version v0.4.44 - Focus Loss Diagnostics**

New release (v0.4.44) https://github.com/coasting-nc/LMUFFB/releases

Fixes and diagnostics for the wheel disconnecting when lmuffb app is not focused.

If you still get issues with the FFB disappearing or wheel disconnecting when lmuffb app is not focused, try the following:
* Start the lmuFFB app and select your wheel in the drop down menu BEFORE starting LMU (in this way it might keep its priority on the wheel)
* In any case, paste in this forum thread the text or screenshot from the lmuFFB app console, since there are new logs that help diagnose what is causing the problem.

---

## December 23, 2025
**Version v0.4.45 / v0.4.46 / v0.5.2 - GUI Improvements**

New release (v0.4.45) https://github.com/coasting-nc/LMUFFB/releases

* Added checkbox for **"Always on Top" Mode**
* Fine adjustments to the sliders with keyboard keys (left and right arrows)

Version v0.4.46:
* Reorganized GUI with 10 collapsible sections, including General FFB settings, Understeer/Front Tyres, Oversteer/Rear Tyres, Grip Estimation, Haptics, and Textures.

Version 0.5.2:
* Improved visual design and readability of the app.
* Normalized most sliders to a 0-100% range

---

## December 24, 2025
**Version 0.5.7 - New Sliders**

New release (0.5.7): https://github.com/coasting-nc/LMUFFB/releases

Added new sliders:
* **optimal_slip_angle**: used to estimate grip, affects these effects: Understeer, Oversteer Boost, Slide Texture.
* **optimal_slip_ratio**: controls the "Combined Friction Circle" logic. It determines how much Longitudinal Slip (Braking or Acceleration) contributes to the calculated "Grip Loss." It also scales the vibration strength for Slide Texture.
* **steering_shaft_smoothing**: low pass filter smoothing for the steering shaft torque. It could help reduce "vibrations" from the steering shaft, noticeable in DD wheels. Could be similar to the in-game FFB smoothing settig.

---

## December 24, 2025
**Version 0.5.8 / 0.5.10 - Error Handling & Smoothing**

New release (0.5.8): https://github.com/coasting-nc/LMUFFB/releases

Improves Error Handling for no FFB when lmuFFB not on top.
* Aggressive FFB Recovery: Implemented more robust DirectInput connection recovery. Try to re-acquire the wheel even when the error code is "unknown"
* Set "Always on Top" on by default
* Added more informative prints to the console when FFB / wheel connection is lost

If you still get the issue, paste here a screenshot of the console of lmuFFB.

**EDIT: New release (0.5.10)**: https://github.com/coasting-nc/LMUFFB/releases

Added 3 additional sliders that expose three pre-existing smoothing settings:
* Yaw Kick Smoothing, Gyroscopic Damping Smoothing, Chassis Inertia (Load) Smoothing

---

## December 25, 2025
**Version 0.5.14 - Error Logging**

New release (0.5.14): https://github.com/coasting-nc/LMUFFB/releases

* Improved more informative error logs on the console to diagnose FFB lost or wheel disconnected.

---

## December 25, 2025
**Version 0.6.1 - Advanced Braking & Lockup Effects**

New release (0.6.1): https://github.com/coasting-nc/LMUFFB/releases

Added new section of settings for **Advanced Braking & Lockup effects** adjustments:

Introduces **Predictive Lockup logic** based on **Angular Deceleration of the wheels**, allowing the driver to feel the onset of a lockup before the tire saturates.

* Added sliders to determine at which level of slip the lockup vibration starts (0% amplitude, defaults at 5% slip) and saturates (100% amplitude, defaults at 12-15% slip).
* Added Vibration Gamma for a non linear response from 0% to 100% amplitude: 1.0 = Linear, 2.0 = Quadratic, 3.0 = Cubic (Sharp/Late)
* Added **ABS Pulse** effect based on Brake Pressure.
* Uses Brake Pressure info to ensure that Brake Bias changes and ABS are correctly felt through the wheel.
* Modulating the vibration amplitude based on Brake Fade (mBrakeTemp > 800°C).

Rear vs Front lockup differentiation:
* Added "Rear Boost" adjustment that multiplies vibration amplitude when the rears lockup
* Also using a different vibration frequency for rear vs front lockups.

Advanced gating logic to avoid false positives and false negatives in lockup detection:
* Brake Gate: Effect is disabled if Brake Input < 2%.
* Airborne Gate: Effect is disabled if mSuspForce < 50N (Wheel in air).
* Bump Rejection: Effect is disabled if Suspension Velocity > Threshold (e.g., 1.0 m/s).
* Relative Deceleration: The wheel must be slowing down significantly faster than the chassis.

Added sliders to adjust some of the main gating mechanisms:
* Prediction sensitivity: angular deceleration threshold
* Bump Rejection: suspension velocity threshold to disable prediction. Increase for bumpy tracks (Sebring)

---

## December 25, 2025
**Version 0.6.2 - DirectInput Recovery**

New release (0.6.2): https://github.com/coasting-nc/LMUFFB/releases

* Improved mechanism to **reacquire the wheel device from DirectInput** if LMU regains exclusive control to send FFB.

---

## December 27, 2025
**Version 0.6.20 - Tooltip Overhaul**

New release (0.6.20): https://github.com/coasting-nc/LMUFFB/releases

Changes since v. 0.6.2:
* Overhauled all tooltips
* Added Adjustable Yaw Kick Activation Threshold (to remove any noise from the Yaw Kick, and only "kick in" when there is an actual slide starting)
* LMU tire vibration removal (Static Notch Filter): Added "Filter Width" (0.1 to 10.0 Hz) for the frequency to block
* Renamed "SoP Lateral G" to "Lateral G"
* Renamed "Rear Align Torque" to "SoP Self-Aligning Torque"
* Expanded the ranges of some sliders
* Added sliders to adjust frequency / pitch for vibration effects
* Troubleshooting Guide in Readme
* Added console prints that detect when some telemetry data (Grip, Tire Load, Suspension) is missing
* Save Screenshot button now also include a screenshot of the console

---

## December 28, 2025
**Version 0.6.22 - Vibration Fixes**

New release (0.6.22): https://github.com/coasting-nc/LMUFFB/releases

Fixed vibrations when car still / in the pits:
* Disabled vibration effects when speed below a certain threashold (ramp from 0.0 vibrations at < 0.5 m/s to 1.0 vibrations at > 2.0 m/s).
* Automatic Idle Smoothing: Steering Torque is smoothed when car is stationary or moving slowly (< 3.0 m/s). This should remove any remaining engine vibrations.
* Road Texture Fallback: alternative method to calculate Road Texture when mVerticalTireDeflection is encrypted from the game (the fallback switches to using Vertical Acceleration  / mLocalAccel.y)
* Added console print to diagnose when mVerticalTireDeflection is encrypted
* Clarified other console prints about missing data (eg. mTireLoad): this is due to the data being encrypted by LMU on all cars (due to both licensing issues, and, for some data, to avoid esports exploits).

---

## December 28, 2025
**Version 0.6.23 - Smoothing Adjustments**

New release (0.6.23): https://github.com/coasting-nc/LMUFFB/releases

* Increased default max speed for smoothing of steering torque from 10km/h to 18/km (you should get no more engine / clutch vibrations below 18 km/h).
* Added slider to the GUI to adjust this value (It's a new  Advanced Settings at the bottom of the GUI, below the Tactile Textures section).

If you still get the issue of vibrations at low speed, please post a screenshot with the Graphs, include all three of the graph sections.

**Edit**: Note that you should set Max Torque Ref according to the Car's Peak Output, NOT your wheelbase capability. The current tooltip is wrong about this. A value of 100 Nm often works best, and values of at least ~40-60 Nm should prevent clipping.

---

## January 1, 2026
**Version 0.6.30 - Low-Speed Fixes**

New release (0.6.30): https://github.com/coasting-nc/LMUFFB/releases

* Fixed Remaining Vibrations at Low-Speed and when in Pits: applied muting and smoothing also to SoP (including Self Aligning Torque) and Base Torque (steering rack).
* Fixed: some settings were not being saved to the ini file, so they were been restored to defaults every time you opened the app.
* Added auto save feature: every time you change a slider, the configuration gets saved to the ini file.
* Updated T300 preset, tested on LMP2 car
* Clarified Max Torque Ref tooltip: it represents the expected PEAK torque of the CAR in the game, NOT the wheelbase capability

Findings from more testing: the Understeer Effect seems to be working. In fact, with the LMP2 it is even too sensitive and makes the wheel too light. I had to set the understeer effect slider to 0.84, and even then it was too strong. It was not the case with LMGT3 (Porsche).

---

## January 2, 2026
**Version 0.6.31 - Understeer Effect Fixes**

New release (0.6.31): https://github.com/coasting-nc/LMUFFB/releases

Fixes for Understeer Effect:
* Reduced the previous range from 0-200 to 0-2 (since the useful values seemed to be only between 0-2)
* Normalized the slider as a percentage 0-200% (now 200% corresponds to the previous 2.0)
* Changed optimal slip angle value in T300 preset to 0.10 (up from 0.06)
* Overhauled the tooltips for "Understeer Effect" and "Optimal Slip Angle"

---

## January 2, 2026
**Version 0.6.32 - Preset Fix**

New release (0.6.32): https://github.com/coasting-nc/LMUFFB/releases

Fixed: "Test: Understeer Only" Preset to ensure proper effect isolation.

---

## January 4, 2026
**Version 0.6.35 - DD Presets**

New release (0.6.35): https://github.com/coasting-nc/LMUFFB/releases

Added new DD presets based on @dISh and Gamer Muscle presets from today:
* GT3 DD 15 Nm (Simagic Alpha)
* LMPx/HY DD 15 Nm (Simagic Alpha)
* GM DD 21 Nm (Moza R21 Ultra)
* GM + Yaw Kick DD 21 Nm (Moza R21 Ultra) - This is a modified version which adds Yaw Kick, which GM did not use and that can give road details.

Updated the default preset to match the "GT3 DD 15 Nm (Simagic Alpha)" preset.

---

## January 5, 2026
**Version 0.6.36 - Code Architecture Improvements**

New release (0.6.36): https://github.com/coasting-nc/LMUFFB/releases

### Improved
- **Performance Optimization**: Improved code organization and maintainability. Fixed an issue where "Torque Drop" was incorrectly affecting texture effects (Road, Slide, Spin, Bottoming), which now remain distinct during drifts and burnouts.

---

## January 31, 2026
**Version 0.6.37 - Application Icon**

New release (0.6.37): https://github.com/coasting-nc/LMUFFB/releases

**Special Thanks** to:
- **@MartinVindis** for designing the application icon
- **@DiSHTiX** for implementing it

### Added
- **Application Icon**: The app now has a custom icon that appears in Windows Explorer and the Taskbar, making it easier to identify.

---

## January 31, 2026
**Version 0.6.38 - LMU Plugin Update**

New release (0.6.38): https://github.com/coasting-nc/LMUFFB/releases

**Special Thanks** to **@DiSHTiX** for the LMU Plugin update!

### Fixed
- **Compatibility with 2025 LMU Plugin**: Fixed build errors after the LMU plugin update, ensuring full compatibility with LMU 1.2/1.3 standards. The fix preserves official files to make future plugin updates easier.

---

## January 31, 2026
**Version 0.6.39 - Auto-Connect & Performance**

New release (0.6.39): https://github.com/coasting-nc/LMUFFB/releases

**Special Thanks** to **@AndersHogqvist** for the Auto-connect feature!

### Added
- **Auto-Connect to LMU**: The app now automatically attempts to connect to the game every 2 seconds. No more manual "Retry" clicking! The GUI shows "Connecting to LMU..." in yellow while searching and "Connected to LMU" in green when active.

### Improved
- **Faster Force Feedback**: Reduced internal processing overhead by 33%, improving responsiveness at the critical 400Hz update rate (down from 1,200 to 800 operations per second).
- **Connection Reliability**: Fixed potential resource leaks and improved connection robustness, especially when the game window isn't immediately available.

---

**Total Releases Found**: 31 version releases from December 6, 2025 to January 31, 2026

**Project Links**:
* GitHub Repository: https://github.com/coasting-nc/LMUFFB
* GitHub Releases: https://github.com/coasting-nc/LMUFFB/releases
* Forum Thread: https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/
