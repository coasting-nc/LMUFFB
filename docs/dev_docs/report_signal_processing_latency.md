# Report: Signal Processing & Latency Optimization

## 1. Introduction and Context
This report addresses several related issues found in the "Troubleshooting 25" list concerning the quality, timing, and filtering of the Force Feedback signals.



**Problems Identified:**
*   **Static Noise / Notch Filter Limitations**: The current implementation uses a fixed Q-factor (width) for the static notch filter. User reports (specifically user "Neebula") indicate that a broader range (10-12 Hz) is more effective at blocking baseline vibration than the current surgical filter. The default center frequency should also be adjusted to ~11 Hz to cover this band.
*   **Perceived Latency**: Users report a "delay" and "disconnect from game physics" even when smoothing is disabled. We need to investigate if this is inherent to the specific game/wheel combination or if the app's processing loop introduces avoidable lag.
*   **Yaw Kick Noise & Ineffectiveness**: The Yaw Kick effect is reported to be "too noisy" and sometimes fails to trigger when the rear actually steps out. It triggers on road noise instead of useful slide data.
*   **Gyro Compensating Yaw Kick**: There is a suggestion to experiment with using Gyro Damping to compensate or balance the Yaw Kick effect.

## 2. Proposed Solution

### 2.1. Static Notch Filter Improvements
*   **Variable Width (Q-Factor/Bandwidth)**: We will upgrade the `BiquadNotch` implementation or its configuration to accept a "Width" or "Bandwidth" parameter.
*   **UI Exposure**: Add a slider in the GUI for "Notch Width" (in Hz) alongside the Frequency slider.
*   **Defaults**: Set the default Frequency to **11 Hz** and the default Width to **2.0 Hz** (effectively filtering 10-12 Hz).

### 2.2. Latency Investigation & Monitoring
*   **Timestamp Logging**: To investigate latency, we will add high-precision timestamps to the console output when `DirectInput` packets are sent vs. when Game Telemetry is received.
*   **Processing Loop Check**: Verify that the main loop sleep times are not causing jitter.

### 2.3. Yaw Kick Enhancements
*   **Threshold Slider**: Implement a "Noise Gate" or Threshold calculation for the Yaw Kick. The kick should only apply if `mLocalRotAccel` magnitude exceeds a user-defined value (e.g., 2.0 rad/s²). This prevents road noise (which usually has low acceleration amplitude but high frequency) from shaking the wheel, while allowing genuine slides (high acceleration) to pass through.
*   **Smoothing Tuning**: Re-evaluate the default smoothing factor for Yaw Kick in light of the new threshold.

## 3. Implementation Plan

### 3.1. `src/FFBEngine.h`
1.  **Add Configuration Variables**:
    ```cpp
    float m_static_notch_width = 2.0f; // Width in Hz
    float m_yaw_kick_threshold = 1.0f; // Threshold in rad/s^2
    ```
2.  **Update `calculate_force` Logic**:
    *   **Yaw Kick**: Implement the threshold check before applying smoothing or gain.
        ```cpp
        double raw_yaw_accel = data->mLocalRotAccel.y;
        // Apply Threshold (Soft knee or hard cut)
        if (std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) {
            raw_yaw_accel = 0.0;
        }
        ```
    *   **Notch Filter**: Calculate 'Q' based on Center Frequency and Width.
        ```cpp
        // Q = CenterFreq / Bandwidth
        double bw = (double)m_static_notch_width;
        if (bw < 0.1) bw = 0.1; // Safety
        double q = m_static_notch_freq / bw;
        m_static_notch_filter.Update((double)m_static_notch_freq, 1.0/dt, q);
        ```

### 3.2. `src/GuiLayer.cpp`
1.  **Expose sliders**:
    *   Under "Signal Filtering": Add "Filter Width (Hz)" slider (Range 0.1 - 10.0).
    *   Under "Rear Axle / Yaw Kick": Add "Activation Threshold" slider (Range 0.0 - 10.0 rad/s²).

## 4. Testing Plan

### 4.1. Notch Filter Test
*   **Setup**: Enable Static Notch. Set Freq = 50Hz, Width = 10Hz.
*   **Action**: Use a signal generator (or "Synth Mode") to inject 50Hz noise.
*   **Verification**: Ensure output is near 0. Inject 44Hz noise. Ensure output is attenuated but not zero. Inject 60Hz noise (outside 45-55 band). Ensure output is near full strength.

### 4.2. Yaw Kick Threshold Test
*   **Setup**: Set Yaw Kick Threshold to 5.0 (high).
*   **Action**: Drive straight over bumps (road noise).
*   **Verification**: The "Yaw Kick" graph line should remain flat (0.0), reducing "rattling".
*   **Action**: Initiate a drift.
*   **Verification**: The graph line should spike as soon as rotation acceleration exceeds 5.0.

### 4.3. Latency Check
*   **Setup**: Enable the new timestamp logging.
*   **Action**: Correlate game physics update time (from `mElapsedTime`) with the wall-clock time of the FFB packet submission.
*   **Verification**: Calculate the delta. If > 10ms, investigate thread scheduling or VSync settings.


## TODO, updates to this document:
* move the **Perceived Latency** discussion and implementation to a separate document; this will be addressed / implemented later; we will also first need confirmation from testing by other users with DDs that the issue is still present before proceeding with this.
* regarding yaw kick: verify that there is already a threshold implemented based on yaw acceleration; we need to expose this in the UI and allow users to adjust it. Veryfy if the proposed implementation described in this document (`mLocalRotAccel` magnitude exceeds a user-defined value (e.g., 2.0 rad/s²)) it is an additional gating mechanism or it is the same threshold used in the current implementation. If it is the same threshold, we need to expose it in the UI and allow users to adjust it. If it is an additional gating mechanism, we need to expose both the exiting and the new threshold in the UI and allow users to adjust both.
