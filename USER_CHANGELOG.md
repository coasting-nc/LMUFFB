# lmuFFB App Version Releases by ErwinMoss

This document contains all version release posts by ErwinMoss from the [url=https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/]lmuFFB App thread[/url] on Le Mans Ultimate Community.

[b]Note:[/b] This file uses BBCode formatting for easy copy-paste to forums.

---

[size=5][b]February 2, 2026[/b][/size]
[b]Version v0.7.1 - Slope Detection Stability & UI[/b]

[b]New release[/b] (v0.7.1) https://github.com/coasting-nc/LMUFFB/releases

This update addresses several stability issues discovered in the initial Slope Detection release (v0.7.0) and improves the user interface for better tuning.

[b]Fixes & Improvements:[/b]
[list]
[*] [b]Oscillation Fix[/b]: Lateral G Boost (Slide) is now automatically disabled when Slope Detection is ON. This prevents a feedback loop caused by different grip calculation methods between front and rear axles.
[*] [b]UI Tooltips[/b]: Added detailed tooltips for Slope Detection parameters with recommended values for Direct Drive, Belt, and Gear-driven wheels.
[*] [b]Slope Graph[/b]: Added a new live graph in the "Internal Physics" header to monitor the slope (dG/dAlpha) in real-time. This helps in visual tuning and understanding tire behavior.
[*] [b]Smoother Transitions[/b]: Updated default settings for a less aggressive response out-of-the-box (Sensitivity: 0.5, Smoothing Tau: 0.04s).
[*] [b]Diagnostic Improvements[/b]: Added live slope values to the diagnostic snapshot for easier troubleshooting.
[/list]

---

[size=5][b]February 1, 2026[/b][/size]
[b]Version v0.7.0 - Dynamic Slope Detection[/b]

[b]New release[/b] (v0.7.0) https://github.com/coasting-nc/LMUFFB/releases

This is a major update introducing [b]Dynamic Slope Detection[/b] for tire grip estimation.

[b]Major Features:[/b]
[list]
[*] [b]Dynamic Understeer Feel[/b]: Instead of using a fixed "Optimal Slip Angle", the app now monitors the slope of the tire curve (Lateral G vs Slip Angle) in real-time. This allows lmuFFB to detect exactly when the tire starts to saturate, providing a much more precise and natural Understeer feel that adapts to different cars, tires, and track conditions.
[*] [b]Advanced Signal Processing[/b]: Uses Savitzky-Golay filtering to calculate clean derivatives from noisy telemetry data, ensuring smooth and stable feedback.
[*] [b]Safety Hardening[/b]: Added comprehensive regression tests to ensure all settings are correctly synchronized and saved across sessions.
[/list]

[b]Tuning Tip:[/b] You can find the new Slope Detection settings under the "Grip Estimation" section. For most users, the default settings will provide a significantly improved Understeer cue out-of-the-box.

---

[size=5][b]Post #1 - July 24, 2025[/b][/size]
[b]Initial Thread Creation[/b]

lmuFFB early version released: https://github.com/coasting-nc/LMUFFB/releases/

Some of the FFB formulas might need refinements. Adjust the FFB configuration options in lmuFFB to see which of the currently supported effects you prefer.

Post your screenshots of the Graphs, the main lmuFFB window and the console here if you can (use the Save Screenshot button to capture them all).

Your testing and feedback is greatly appreciated!

Project homepage: https://github.com/coasting-nc/LMUFFB
Download latest version: https://github.com/coasting-nc/LMUFFB/releases/

---

[size=5][b]Post #10 - December 6, 2025[/b][/size]
[b]Early Version Release[/b]

I've created an early version of LMUFFB: https://github.com/coasting-nc/LMUFFB/releases

It needs vJoy + the rF2 shared-memory plugin. I haven't fully tested it yet (sorting my wheel), so please give it a spin and drop any bugs/feedback here or on GitHub. Thanks!

---

[size=5][b]Post #41 - December 9, 2025[/b][/size]
[b]Version v0.4.0b - LMU 1.2 Support Added[/b]

Support for LMU 1.2 added, give it a try: https://github.com/coasting-nc/LMUFFB/releases/tag/v0.4.0b

Remember that this is an experimental version, for safety you should first lower you wheel max strength from your device driver configurator, since the app might cause strong force spikes and oscillations. Eg. 10 % max force for DDs, 20-30 % for gear/belt driven wheels.

The app probably still needs refinements to the FFB formulas. Adjust the FFB configuration options in lmuFFB to see if you manage to get a workable FFB signal.
Post your screenshots of the Troubleshooting Graphs window and the main lmuFFB window here if you can.

Your testing and feedback is greatly appreciated!

---

[size=5][b]December 11, 2025[/b][/size]
[b]Version v0.4.4b[/b]

[b]New release[/b]: https://github.com/coasting-nc/LMUFFB/releases/tag/v0.4.4b

Updates to install instructions:
[list]
[*]vJoy might not be necessary, ignore the warning. Just set the ingame FFB to 0% and off.
[*]If FFB feels "backwards" or "inverted" (wheel pushes away from center instead of pulling toward it), check or uncheck the "Invert FFB" checkbox in the lmuFFB GUI to reverse the force direction
[*]See README.txt for the rest of the instructions
[/list]

Known issues:
You will see warnings about Missing Tire Load and Missing Grip Fraction data detected. This is EXPECTED and NOT a bug in lmuFFB. This is a BUG IN LMU 1.2 - the game is currently returning ZERO (0) for all tire load and grip fraction values, even though the shared memory interface includes these fields.

---

[size=5][b]December 15, 2025[/b][/size]
[b]Version v0.4.15+ - Major Stability Fix[/b]

[b]New release[/b] (v0.4.15+) https://github.com/coasting-nc/LMUFFB/releases

This should fix the FFB instability issues (eg. intermittent inverse FFB).
It also removes the messages and instructions about vJoy and the rFactor 2 shared memory dll, since vJoy is no longer needed, and there are no problems if you keep the shared memory dll in the game.

Full changelogs: https://github.com/coasting-nc/LMUFFB/blob/main/CHANGELOG_DEV.md
 
---

[size=5][b]December 15, 2025[/b][/size]
[b]Version v0.4.16 - Yaw Kick Effect[/b]

[b]New release[/b] (v0.4.16) https://github.com/coasting-nc/LMUFFB/releases

This introduces a new Yaw Kick effect within the Seat of Pants effect, that tells when the rear starts to step out. This should help catch early slides.

---

[size=5][b]December 16, 2025[/b][/size]
[b]Version v0.4.18 - Gyroscopic Damping[/b]

[b]New release[/b] (v0.4.18) https://github.com/coasting-nc/LMUFFB/releases

Added Gyroscopic Damping (stabilization effect to prevent "tank slappers" during drifts), and smoothing to the Yaw Kick effect.

---

[size=5][b]December 16, 2025[/b][/size]
[b]Version v0.4.19 - Coordinate System Fix[/b]

[b]New release[/b] (v0.4.19) https://github.com/coasting-nc/LMUFFB/releases

This fixes a few bugs on the use of rF2 / LMU coordinate system. This should fix the issue of the broken slide rumble.

---

[size=5][b]December 19, 2025[/b][/size]
[b]Version v0.4.20 - Force Direction Fixes[/b]

[b]New release[/b] (v0.4.20) https://github.com/coasting-nc/LMUFFB/releases

Fixed two force direction inversions that were causing the wheel to pull in the direction of the turn/slide instead of resisting it, creating unstable positive feedback loops (Positive Feedback Loop in Scrub Drag and Yaw Kick).

Let me know if this new version fixes the issue with the slide rumble. If you still get this or some other issue, please post a screenshot of the UI with at least the main window with the settings (you can use the save screenshot button for convenience).

---

[size=5][b]December 19, 2025[/b][/size]
[b]Version v0.4.24 - Guide Presets[/b]

[b]New release[/b] (v0.4.24) https://github.com/coasting-nc/LMUFFB/releases

Note: make sure to check you have in-game FFB disabled and strength set to 0%, since after an LMU update these values can get enabled again.

Added [b]Guide Presets[/b]: Added 5 new built-in presets corresponding to the "Driver's Guide to Testing LMUFFB".
[list]
[*]Guide: Understeer (Front Grip): Isolates the grip modulation effect.
[*]Guide: Oversteer (Rear Grip): Isolates SoP and Rear Aligning Torque.
[*]Guide: Slide Texture (Scrub): Isolates the scrubbing vibration (mutes base force).
[*]Guide: Braking Lockup: Isolates the lockup vibration (mutes base force).
[*]Guide: Traction Loss (Spin): Isolates the wheel spin vibration (mutes base force).
[/list]

These presets allow users to quickly configure the app for the specific test scenarios described in the documentation.

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.31 - T300 Preset[/b]

[b]New release[/b] (v0.4.31) https://github.com/coasting-nc/LMUFFB/releases

Fixed inverted SoP (Lateral G) effect, and added a new T300 preset that has working default values for understeer and oversteer / SoP effects.

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.37 - Slide Texture Overhaul[/b]

[b]New release[/b] (v0.4.37) https://github.com/coasting-nc/LMUFFB/releases

Fixed Slide Rumble bug (due to "phase explosion" issue that caused massive constant force pulls during frame stutters).

[b]Slide Texture Overhaul[/b]: Now detects rear slip (drifting/donuts), uses optimized frequencies (10-70Hz) for better rumble on belt wheels, and adds a manual Pitch slider.

[b]Tuning[/b]: All defaults and presets have been re-calibrated around a T300 baseline

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.38 - Frame Stutter Fixes[/b]

[b]New release[/b] (v0.4.38) https://github.com/coasting-nc/LMUFFB/releases

More fixes to ensure consistent FFB even in case of physics frames stuttering.

---

[size=5][b]December 20, 2025[/b][/size]
[b]Version v0.4.39 - Improved Tyre Physics[/b]

[b]New release[/b] (v0.4.39) https://github.com/coasting-nc/LMUFFB/releases

Implemented better approximations for tyre grip and load.

---

[size=5][b]December 21, 2025[/b][/size]
[b]Version v0.4.43 - Latency Reduction & Flatspot Suppression[/b]

[b]New release[/b] (v0.4.43) https://github.com/coasting-nc/LMUFFB/releases

Fixed "delay" in FFB: reduced latency to < 15ms due to SoP Smoothing and Slip Angle Smoothing

Added sliders to adjust SoP Smoothing and Slip Angle Smoothing

[b]Eliminating LMU tyre flatspot vibration[/b]:
[list]
[*]Added Dynamic Notch Filter (Flatspot Suppression) with Dynamic Suppression Strength
[*]Added Static Notch Filter
[*]See this guide on how to use them: Dynamic Flatspot Suppression - User Guide
[/list]

Fixes to Yaw Kick noise: Muted for low forces and low speeds.

---

[size=5][b]December 21, 2025[/b][/size]
[b]Version v0.4.44 - Focus Loss Diagnostics[/b]

[b]New release[/b] (v0.4.44) https://github.com/coasting-nc/LMUFFB/releases

Fixes and diagnostics for the wheel disconnecting when lmuffb app is not focused.

If you still get issues with the FFB disappearing or wheel disconnecting when lmuffb app is not focused, try the following:
[list]
[*]Start the lmuFFB app and select your wheel in the drop down menu BEFORE starting LMU (in this way it might keep its priority on the wheel)
[*]In any case, paste in this forum thread the text or screenshot from the lmuFFB app console, since there are new logs that help diagnose what is causing the problem.
[/list]

---

[size=5][b]December 23, 2025[/b][/size]
[b]Version v0.4.45 / v0.4.46 / v0.5.2 - GUI Improvements[/b]

[b]New release[/b] (v0.4.45) https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Added checkbox for [b]"Always on Top" Mode[/b]
[*]Fine adjustments to the sliders with keyboard keys (left and right arrows)
[/list]

Version v0.4.46:
[list]
[*]Reorganized GUI with 10 collapsible sections, including General FFB settings, Understeer/Front Tyres, Oversteer/Rear Tyres, Grip Estimation, Haptics, and Textures.
[/list]

Version 0.5.2:
[list]
[*]Improved visual design and readability of the app.
[*]Normalized most sliders to a 0-100% range
[/list]

---

[size=5][b]December 24, 2025[/b][/size]
[b]Version 0.5.7 - New Sliders[/b]

[b]New release[/b] (0.5.7): https://github.com/coasting-nc/LMUFFB/releases

Added new sliders:
[list]
[*][b]optimal_slip_angle[/b]: used to estimate grip, affects these effects: Understeer, Oversteer Boost, Slide Texture.
[*][b]optimal_slip_ratio[/b]: controls the "Combined Friction Circle" logic. It determines how much Longitudinal Slip (Braking or Acceleration) contributes to the calculated "Grip Loss." It also scales the vibration strength for Slide Texture.
[*][b]steering_shaft_smoothing[/b]: low pass filter smoothing for the steering shaft torque. It could help reduce "vibrations" from the steering shaft, noticeable in DD wheels. Could be similar to the in-game FFB smoothing settig.
[/list]

---

[size=5][b]December 24, 2025[/b][/size]
[b]Version 0.5.8 / 0.5.10 - Error Handling & Smoothing[/b]

[b]New release[/b] (0.5.8): https://github.com/coasting-nc/LMUFFB/releases

Improves Error Handling for no FFB when lmuFFB not on top.
[list]
[*]Aggressive FFB Recovery: Implemented more robust DirectInput connection recovery. Try to re-acquire the wheel even when the error code is "unknown"
[*]Set "Always on Top" on by default
[*]Added more informative prints to the console when FFB / wheel connection is lost
[/list]

If you still get the issue, paste here a screenshot of the console of lmuFFB.

[b]EDIT: New release (0.5.10)[/b]: https://github.com/coasting-nc/LMUFFB/releases

Added 3 additional sliders that expose three pre-existing smoothing settings:
[list]
[*]Yaw Kick Smoothing, Gyroscopic Damping Smoothing, Chassis Inertia (Load) Smoothing
[/list]

---

[size=5][b]December 25, 2025[/b][/size]
[b]Version 0.5.14 - Error Logging[/b]

[b]New release[/b] (0.5.14): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Improved more informative error logs on the console to diagnose FFB lost or wheel disconnected.
[/list]

---

[size=5][b]December 25, 2025[/b][/size]
[b]Version 0.6.1 - Advanced Braking & Lockup Effects[/b]

[b]New release[/b] (0.6.1): https://github.com/coasting-nc/LMUFFB/releases

Added new section of settings for [b]Advanced Braking & Lockup effects[/b] adjustments:

Introduces [b]Predictive Lockup logic[/b] based on [b]Angular Deceleration of the wheels[/b], allowing the driver to feel the onset of a lockup before the tire saturates.

[list]
[*]Added sliders to determine at which level of slip the lockup vibration starts (0% amplitude, defaults at 5% slip) and saturates (100% amplitude, defaults at 12-15% slip).
[*]Added Vibration Gamma for a non linear response from 0% to 100% amplitude: 1.0 = Linear, 2.0 = Quadratic, 3.0 = Cubic (Sharp/Late)
[*]Added [b]ABS Pulse[/b] effect based on Brake Pressure.
[*]Uses Brake Pressure info to ensure that Brake Bias changes and ABS are correctly felt through the wheel.
[*]Modulating the vibration amplitude based on Brake Fade (mBrakeTemp > 800°C).
[/list]

Rear vs Front lockup differentiation:
[list]
[*]Added "Rear Boost" adjustment that multiplies vibration amplitude when the rears lockup
[*]Also using a different vibration frequency for rear vs front lockups.
[/list]

Advanced gating logic to avoid false positives and false negatives in lockup detection:
[list]
[*]Brake Gate: Effect is disabled if Brake Input < 2%.
[*]Airborne Gate: Effect is disabled if mSuspForce < 50N (Wheel in air).
[*]Bump Rejection: Effect is disabled if Suspension Velocity > Threshold (e.g., 1.0 m/s).
[*]Relative Deceleration: The wheel must be slowing down significantly faster than the chassis.
[/list]

Added sliders to adjust some of the main gating mechanisms:
[list]
[*]Prediction sensitivity: angular deceleration threshold
[*]Bump Rejection: suspension velocity threshold to disable prediction. Increase for bumpy tracks (Sebring)
[/list]

---

[size=5][b]December 25, 2025[/b][/size]
[b]Version 0.6.2 - DirectInput Recovery[/b]

[b]New release[/b] (0.6.2): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Improved mechanism to [b]reacquire the wheel device from DirectInput[/b] if LMU regains exclusive control to send FFB.
[/list]

---

[size=5][b]December 27, 2025[/b][/size]
[b]Version 0.6.20 - Tooltip Overhaul[/b]

[b]New release[/b] (0.6.20): https://github.com/coasting-nc/LMUFFB/releases

Changes since v. 0.6.2:
[list]
[*]Overhauled all tooltips
[*]Added Adjustable Yaw Kick Activation Threshold (to remove any noise from the Yaw Kick, and only "kick in" when there is an actual slide starting)
[*]LMU tire vibration removal (Static Notch Filter): Added "Filter Width" (0.1 to 10.0 Hz) for the frequency to block
[*]Renamed "SoP Lateral G" to "Lateral G"
[*]Renamed "Rear Align Torque" to "SoP Self-Aligning Torque"
[*]Expanded the ranges of some sliders
[*]Added sliders to adjust frequency / pitch for vibration effects
[*]Troubleshooting Guide in Readme
[*]Added console prints that detect when some telemetry data (Grip, Tire Load, Suspension) is missing
[*]Save Screenshot button now also include a screenshot of the console
[/list]

---

[size=5][b]December 28, 2025[/b][/size]
[b]Version 0.6.22 - Vibration Fixes[/b]

[b]New release[/b] (0.6.22): https://github.com/coasting-nc/LMUFFB/releases

Fixed vibrations when car still / in the pits:
[list]
[*]Disabled vibration effects when speed below a certain threashold (ramp from 0.0 vibrations at < 0.5 m/s to 1.0 vibrations at > 2.0 m/s).
[*]Automatic Idle Smoothing: Steering Torque is smoothed when car is stationary or moving slowly (< 3.0 m/s). This should remove any remaining engine vibrations.
[*]Road Texture Fallback: alternative method to calculate Road Texture when mVerticalTireDeflection is encrypted from the game (the fallback switches to using Vertical Acceleration  / mLocalAccel.y)
[*]Added console print to diagnose when mVerticalTireDeflection is encrypted
[*]Clarified other console prints about missing data (eg. mTireLoad): this is due to the data being encrypted by LMU on all cars (due to both licensing issues, and, for some data, to avoid esports exploits).
[/list]

---

[size=5][b]December 28, 2025[/b][/size]
[b]Version 0.6.23 - Smoothing Adjustments[/b]

[b]New release[/b] (0.6.23): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Increased default max speed for smoothing of steering torque from 10km/h to 18/km (you should get no more engine / clutch vibrations below 18 km/h).
[*]Added slider to the GUI to adjust this value (It's a new  Advanced Settings at the bottom of the GUI, below the Tactile Textures section).
[/list]

If you still get the issue of vibrations at low speed, please post a screenshot with the Graphs, include all three of the graph sections.

[b]Edit[/b]: Note that you should set Max Torque Ref according to the Car's Peak Output, NOT your wheelbase capability. The current tooltip is wrong about this. A value of 100 Nm often works best, and values of at least ~40-60 Nm should prevent clipping.

---

[size=5][b]January 1, 2026[/b][/size]
[b]Version 0.6.30 - Low-Speed Fixes[/b]

[b]New release[/b] (0.6.30): https://github.com/coasting-nc/LMUFFB/releases

[list]
[*]Fixed Remaining Vibrations at Low-Speed and when in Pits: applied muting and smoothing also to SoP (including Self Aligning Torque) and Base Torque (steering rack).
[*]Fixed: some settings were not being saved to the ini file, so they were been restored to defaults every time you opened the app.
[*]Added auto save feature: every time you change a slider, the configuration gets saved to the ini file.
[*]Updated T300 preset, tested on LMP2 car
[*]Clarified Max Torque Ref tooltip: it represents the expected PEAK torque of the CAR in the game, NOT the wheelbase capability
[/list]

Findings from more testing: the Understeer Effect seems to be working. In fact, with the LMP2 it is even too sensitive and makes the wheel too light. I had to set the understeer effect slider to 0.84, and even then it was too strong. It was not the case with LMGT3 (Porsche).

---

[size=5][b]January 2, 2026[/b][/size]
[b]Version 0.6.31 - Understeer Effect Fixes[/b]

[b]New release[/b] (0.6.31): https://github.com/coasting-nc/LMUFFB/releases

Fixes for Understeer Effect:
[list]
[*]Reduced the previous range from 0-200 to 0-2 (since the useful values seemed to be only between 0-2)
[*]Normalized the slider as a percentage 0-200% (now 200% corresponds to the previous 2.0)
[*]Changed optimal slip angle value in T300 preset to 0.10 (up from 0.06)
[*]Overhauled the tooltips for "Understeer Effect" and "Optimal Slip Angle"
[/list]

---

[size=5][b]January 2, 2026[/b][/size]
[b]Version 0.6.32 - Preset Fix[/b]

[b]New release[/b] (0.6.32): https://github.com/coasting-nc/LMUFFB/releases

Fixed: "Test: Understeer Only" Preset to ensure proper effect isolation.

---

[size=5][b]January 4, 2026[/b][/size]
[b]Version 0.6.35 - DD Presets[/b]

[b]New release[/b] (0.6.35): https://github.com/coasting-nc/LMUFFB/releases

Added new DD presets based on @dISh and Gamer Muscle presets from today:
[list]
[*]GT3 DD 15 Nm (Simagic Alpha)
[*]LMPx/HY DD 15 Nm (Simagic Alpha)
[*]GM DD 21 Nm (Moza R21 Ultra)
[*]GM + Yaw Kick DD 21 Nm (Moza R21 Ultra) - This is a modified version which adds Yaw Kick, which GM did not use and that can give road details.
[/list]

Updated the default preset to match the "GT3 DD 15 Nm (Simagic Alpha)" preset.

---

[size=5][b]January 5, 2026[/b][/size]
[b]Version 0.6.36 - Code Architecture Improvements[/b]

[b]New release[/b] (0.6.36): https://github.com/coasting-nc/LMUFFB/releases

[b]Improved[/b]
[list]
[*][b]Performance Optimization[/b]: Improved code organization and maintainability. Fixed an issue where "Torque Drop" was incorrectly affecting texture effects (Road, Slide, Spin, Bottoming), which now remain distinct during drifts and burnouts.
[/list]

---

[size=5][b]January 31, 2026[/b][/size]
[b]Version 0.6.37 - Application Icon[/b]

[b]New release[/b] (0.6.37): https://github.com/coasting-nc/LMUFFB/releases

[b]Special Thanks[/b] to:
[list]
[*][b]@MartinVindis[/b] for designing the application icon
[*][b]@DiSHTiX[/b] for implementing it
[/list]

[b]Added[/b]
[list]
[*][b]Application Icon[/b]: The app now has a custom icon that appears in Windows Explorer and the Taskbar, making it easier to identify.
[/list]

---

[size=5][b]January 31, 2026[/b][/size]
[b]Version 0.6.38 - LMU Plugin Update[/b]

[b]New release[/b] (0.6.38): https://github.com/coasting-nc/LMUFFB/releases

[b]Special Thanks[/b] to [b]@DiSHTiX[/b] for the LMU Plugin update!

[b]Fixed[/b]
[list]
[*][b]Compatibility with 2025 LMU Plugin[/b]: Fixed build errors after the LMU plugin update, ensuring full compatibility with LMU 1.2/1.3 standards. The fix preserves official files to make future plugin updates easier.
[/list]

---

[size=5][b]January 31, 2026[/b][/size]
[b]Version 0.6.39 - Auto-Connect & Performance[/b]

[b]New release[/b] (0.6.39): https://github.com/coasting-nc/LMUFFB/releases

[b]Special Thanks[/b] to [b]@AndersHogqvist[/b] for the Auto-connect feature!

[b]Added[/b]
[list]
[*][b]Auto-Connect to LMU[/b]: The app now automatically attempts to connect to the game every 2 seconds. No more manual "Retry" clicking! The GUI shows "Connecting to LMU..." in yellow while searching and "Connected to LMU" in green when active.
[/list]

[b]Improved[/b]
[list]
[*][b]Faster Force Feedback[/b]: Reduced internal processing overhead by 33%, improving responsiveness at the critical 400Hz update rate (down from 1,200 to 800 operations per second).
[*][b]Connection Reliability[/b]: Fixed potential resource leaks and improved connection robustness, especially when the game window isn't immediately available.
[/list]

---

[b]Total Releases Found[/b]: 31 version releases from December 6, 2025 to January 31, 2026

[b]Project Links[/b]:
[list]
[*]GitHub Repository: https://github.com/coasting-nc/LMUFFB
[*]GitHub Releases: https://github.com/coasting-nc/LMUFFB/releases
[*]Forum Thread: https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/
[/list]
