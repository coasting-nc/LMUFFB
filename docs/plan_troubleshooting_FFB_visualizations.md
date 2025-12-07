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
*   **Lines/Layers:**
    *   `Base Force` (Game Physics)
    *   `SoP Force` (Lateral G)
    *   `Understeer Cut` (Negative contribution)
    *   `Texture Noise` (Road + Slide + Lockup high freq)
    *   `Total Output` (Thick white line)
    *   `Clipping` (Red indicator when Total > 1.0)

**UI Controls:**
*   Checkbox: "Show Debug Graphs" (to save CPU when not needed).
*   Toggles: Enable/Disable individual lines to isolate visual noise.

### B. Telemetry Inspector
A table or bar chart showing raw inputs from Shared Memory.
*   **Critical Values:**
    *   `mSteeringArmForce` (Is the game sending physics?)
    *   `mLocalAccel.x` (Is SoP working? Is it noisy?)
    *   `mTireLoad` (Are we airborne? Is bottoming logic valid?)
    *   `mGripFract` (Is it stuck at 0 or 1? Indicates tire model issues).
    *   `mSlipRatio` / `mSlipAngle`.
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
