# Safety Features Testing & Debugging Guide

This guide explains how to use the debug log to verify the new safety features implemented in version 0.7.157. These features are designed to detect and mitigate FFB spikes, jolts, and unexpected behaviors caused by game stutters or state transitions.

## 1. Monitoring the Debug Log
All safety events are recorded in `lmuffb_debug.log`. You can find this file in the same directory as the LMUFFB executable.

## 2. Expected Log Entries for Safety Scenarios

### Scenario A: You experience a sudden jolt or spike in FFB
If you feel a violent movement of the wheel, check the log for these lines:

*   **`[Safety] High Spike Detected: Requested Rate=XXX (Capped at YYY)`**
    *   **Meaning**: The physics engine requested a force change faster than `500 units/s` for more than 5 consecutive frames.
    *   **Action Taken**: The app capped the rate and entered **Safety Mode** (see below) to blunt the impact.

*   **`[Safety] Entered Safety Mode (Reason: High Spike)`**
    *   **Meaning**: Triggered immediately after a spike is detected.

### Scenario B: The FFB feels "muffled" or "smoothed" for a few seconds
This happens when the app enters **Safety Mode**. During this time (2 seconds), gain is reduced by 50% and extra smoothing is applied to prevent further jolts.

Check the log for the reason why Safety Mode was triggered:

*   **`[Safety] Entered Safety Mode (Reason: Lost Frames)`**
    *   **Meaning**: The app detected a gap in telemetry data (game stutter).
    *   **Perception**: You might notice the wheel become lighter and smoother for 2 seconds immediately following a game hitch.

*   **`[Safety] Entered Safety Mode (Reason: Control Transition)`**
    *   **Meaning**: The player took control from the AI, or vice-versa.
    *   **Perception**: FFB will be attenuated during the handover to ensure it's safe to grab the wheel.

*   **`[Safety] Entered Safety Mode (Reason: FFB Unmuted)`**
    *   **Meaning**: FFB was re-enabled (e.g., leaving the garage).

### Scenario C: FFB Muting and Unmuting
The app now explicitly logs when it stops or starts sending force to the wheel.

*   **`[Safety] FFB Muted (Reason: Game/State Mute)`**
    *   **Meaning**: You entered the garage, paused the game, or the AI took over.
*   **`[Safety] FFB Unmuted`**
    *   **Meaning**: You clicked "Drive" or unpaused.

### Scenario D: The wheel is "pinned" to one side (Runaway Wheel)
If the wheel goes to full lock and stays there with high force, the app will log:

*   **`[Safety] Full Tock Detected: Force XXX at YYY% lock`**
    *   **Meaning**: The wheel was at >95% lock with >80% force for more than 1 second.
    *   **Utility**: This helps identify if a car setup or a physics bug is causing the wheel to "run away".

### Scenario E: Soft Lock Engagement
Soft lock prevents the wheel from turning past the car's physical limits.

*   **`[Safety] Soft Lock Engaged: Steering XXX%`**
    *   **Meaning**: You reached the end of the steering rack.
*   **`[Safety] Soft Lock Significant Influence: XXX Nm`**
    *   **Meaning**: The Soft Lock effect is pushing back with more than 5 Nm.
*   **`[Safety] Soft Lock Disengaged`**
    *   **Meaning**: You turned the wheel back into the valid range.

## 3. How to Provide Feedback
If you experience a jolt that was **NOT** recorded in the log, please report it. It indicates that our detection thresholds (e.g., 500 units/s) might need to be lower to align with user perception.

If you find Safety Mode is triggering too often (constant "muffled" feel), please provide your log so we can see if your system is experiencing frequent frame drops.
