# Fix: bottoming effect falsely triggering during high-speed cornering and weaving #414

**Opened by coasting-nc on Mar 19, 2026**

## Description
Fix: bottoming effect falsely triggering during high-speed cornering and weaving

An issue with the bottoming effect falsely triggering during high-speed cornering and weaving has been identified.

## Cause
The "Safety Trigger" for the bottoming effect (designed to catch extreme suspension compressions when other methods fail) compares the maximum tire load against a threshold based on the car's static weight (`m_static_front_load * 2.5`). However, at high speeds (like Eau Rouge or long straights), aerodynamic downforce can easily double the effective weight of the car. When you weave or corner at these speeds, the lateral load transfer pushes the outside tire's load well beyond the 2.5x static threshold, falsely triggering the bottoming vibration.

Additionally, the UI was missing the controls to adjust or disable the bottoming effect, even though the underlying engine supported a gain multiplier and an enable toggle.

## Fix
1. **Increase the Safety Threshold**: Increased `BOTTOMING_LOAD_MULT` from 2.5 to 4.0. This ensures the safety trigger only activates during genuine extreme impacts (like hitting a massive curb or bottoming out the suspension) rather than from aero + cornering loads.
2. **Expose UI Controls**: Added the "Bottoming Effect" toggle and "Bottoming Strength" slider to the UI under the "Vibration Effects" section, allowing users to tune the intensity or disable it entirely.
3. **Preset Synchronization**: Updated the configuration and preset systems to properly save and load the new `bottoming_enabled` and `bottoming_gain` parameters.

## Proposed Code Changes
(See issue description for full diff)

--- src/ffb/FFBEngine.h
+++ src/ffb/FFBEngine.h
@@ -311,7 +311,7 @@
     static constexpr double BOTTOMING_RH_THRESHOLD_M = 0.002;
     static constexpr double BOTTOMING_IMPULSE_THRESHOLD_N_S = 100000.0;
     static constexpr double BOTTOMING_IMPULSE_RANGE_N_S = 200000.0;
-    static constexpr double BOTTOMING_LOAD_MULT = 2.5;
+    static constexpr double BOTTOMING_LOAD_MULT = 4.0;
     static constexpr double BOTTOMING_INTENSITY_SCALE = 0.05;
     static constexpr double BOTTOMING_FREQ_HZ = 50.0;
     static constexpr double SPIN_THROTTLE_THRESHOLD = 0.05;

(and other files...)

## Regression Tests
(See issue description for full test code)
