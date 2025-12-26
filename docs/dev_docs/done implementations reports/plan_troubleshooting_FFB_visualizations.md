# Plan: FFB & Telemetry Troubleshooting Visualizations

**Date:** 2025-05-23
**Status:** Proposal

## Goal
Implement comprehensive real-time visualizations and logging to assist users and developers in diagnosing FFB issues (jerking, dead signals, clipping) and telemetry gaps.

## 1. GUI Visualizations (Real-time)

### A. FFB Component Graph ("The Stack")
A real-time stacked area chart or multi-line graph showing the contribution of each effect to the total output.
*   **X-Axis:** Time (last 5-10 seconds sliding window).
*   **Y-Axis:** Force (-1.0 to 1.0).
*   **Lines/Layers (Individual Traces):**
    *   `Base Force` (Game Physics: `mSteeringArmForce` scaled)
    *   `SoP Force` (Smoothed Lateral G)
    *   `Understeer Drop` (Reduction factor)
    *   `Oversteer Boost` (Rear Aligning Torque)
    *   `Road Texture` (Vertical Deflection impulse)
    *   `Slide Texture` (Scrubbing vibration)
    *   `Lockup Vibration` (Brake slip impulse)
    *   `Wheel Spin Vibration` (Throttle slip impulse)
    *   `Bottoming Impulse` (High load limiter)
    *   `Total Output` (Thick white line)
    *   `Clipping` (Red indicator when Total > 1.0)

*Visual Style:* All graphs should be **Rolling Trace Plots** (Oscilloscope style), showing the last 5-10 seconds of data. This helps spot patterns like oscillation or spikes.

**UI Controls:**
*   Checkbox: "Show Debug Graphs" (to save CPU when not needed).
*   Toggles: Enable/Disable individual trace lines to isolate visual noise.

### B. Telemetry Inspector
A set of **Rolling Trace Plots** for each raw input value from Shared Memory. 
*   **Critical Values (Traced):**
    *   `mSteeringArmForce` (Is the game sending physics?)
    *   `mLocalAccel.x` (Is SoP working? Is it noisy?)
    *   `mTireLoad` (Are we airborne? Is bottoming logic valid?)
    *   `mGripFract` (Is it stuck at 0 or 1? Indicates tire model issues).
    *   `mSlipRatio` (Brake/Accel slip)
    *   `mSlipAngle` (Cornering slip)
    *   `mLateralPatchVel` (Slide speed)
    *   `mVerticalTireDeflection` (Road surface)
*   **Indicators:**
    *   Green dot: Value updating.
    *   Red dot: Value static/zero (Dead signal warning).

## 2. Troubleshooting Session Mode (Logger)

A dedicated "Calibration / Diagnostic" mode.

### Workflow
1.  User clicks "Start Diagnostic Session".
2.  App asks user to perform specific maneuvers:
    *   "Center Wheel and Drive Straight (5s)" -> Checks noise floor.
    *   "Turn 90 deg Left and Hold" -> Checks polarity.
    *   "Drive over a curb" -> Checks texture impulse.
3.  **Logging:** App records CSV file (`diagnostic_log_timestamp.csv`) at 400Hz.
    *   Columns: Time, Inputs (Steering, Pedals), Physics (G, Vel, Load), FFB Components, Output.
4.  **Analysis:**
    *   App generates a summary report:
        *   "Max Noise on Straight: 0.05 (Good)"
        *   "SoP Jitter: High (Suggest Smoothing)"
        *   "Telemetry Gaps: Tire Temps missing."

## 3. Implementation Plan

### Phase 1: Basic Graphs (v0.4.0)
*   Integrate `ImGui::PlotLines` or `ImPlot` (extension).
*   Visualize Total Output vs. Game Input.

### Phase 2: Detailed Inspection (v0.4.1)
*   Add Telemetry Value table with "Stuck Value" detection.

### Phase 3: Diagnostic Logger (v0.5.0)
*   Implement CSV writer and Wizard UI.

## 4. Implementation Status (v0.3.13)

**Phase 1 & 2 Completed.**
The GUI now includes a "Show Troubleshooting Graphs" toggle which opens an "FFB Analysis" window containing:
1.  **FFB Components Stack:** Real-time rolling plots for all 11 signal components (Base, SoP, Understeer, Oversteer, Road, Slide, Lockup, Spin, Bottoming, Clipping).
2.  **Telemetry Inspector:** Real-time rolling plots for 8 critical telemetry inputs (Steering Force, Lat G, Load, Grip, Slip Ratio/Angle, Patch Vel, Deflection).

**Technical Implementation:**
- `FFBEngine` populates a debug struct `m_last_debug` and `m_last_telemetry` every frame.
- `GuiLayer` maintains static `RollingBuffer` vectors (1000 samples) for each channel.
- `ImGui::PlotLines` renders the data at 60Hz.
