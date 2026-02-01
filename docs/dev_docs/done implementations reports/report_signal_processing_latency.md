# Report: Signal Processing ptimization

## 1. Introduction and Context
This report addresses several related issues found in the "Troubleshooting 25" list concerning the quality, timing, and filtering of the Force Feedback signals.

**Problems Identified:**
*   **Static Noise / Notch Filter Limitations**: The current implementation uses a fixed Q-factor (width) for the static notch filter. User reports (specifically user "Neebula") indicate that a broader range (10-12 Hz) is more effective at blocking baseline vibration than the current surgical filter. The default center frequency should also be adjusted to ~11 Hz to cover this band.

*   **Yaw Kick Noise & Ineffectiveness**: The Yaw Kick effect is reported to be "too noisy" and sometimes fails to trigger when the rear actually steps out. It triggers on road noise instead of useful slide data.
*   **Gyro Compensating Yaw Kick**: There is a suggestion to experiment with using Gyro Damping to compensate or balance the Yaw Kick effect.

## 2. Proposed Solution

### 2.1. Static Notch Filter Improvements
*   **Variable Width (Q-Factor/Bandwidth)**: We will upgrade the `BiquadNotch` implementation or its configuration to accept a "Width" or "Bandwidth" parameter.
*   **UI Exposure**: Add a slider in the GUI for "Notch Width" (in Hz) alongside the Frequency slider.
*   **Defaults**: Set the default Frequency to **11 Hz** and the default Width to **2.0 Hz** (effectively filtering 10-12 Hz).



### 2.3. Yaw Kick Enhancements
*   **Threshold Slider**: Expose the existing "Noise Gate" logic (currently hardcoded at 0.2 rad/s²) to the UI. Implement a "Yaw Kick Threshold" slider (Range 0.0 - 10.0 rad/s²). The kick will only apply if `mLocalRotAccel` magnitude exceeds this user-defined value. This allows users to raise the floor to prevent road noise (which usually has low acceleration amplitude but high frequency) from shaking the wheel, while allowing genuine slides (high acceleration) to pass through.
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

## 5. Documentation Updates

The following documents need to be updated to reflect the changes detailed in this report:

*   `docs\dev_docs\FFB_formulas.md`
*   `docs\dev_docs\references\Reference - telemetry_data_reference.md`

## 6. Automated Tests

We must ensure these signal processing changes are robust and do not regress.

### 6.1. Notch Filter Tests (`tests/test_ffb_engine.cpp`)
*   **Variable Width Test**:
    *   Create a test case `test_notch_filter_bandwidth`.
    *   Configure the engine with a specific notch frequency (e.g., 50Hz) and width (e.g., 10Hz).
    *   Inject signal at exactly 50Hz -> Verify near-zero output (high attenuation).
    *   Inject signal at 44Hz (within the 10Hz width) -> Verify significant attenuation.
    *   Inject signal at 60Hz (outside the 10Hz width) -> Verify minimal attenuation (near input magnitude).
    *   Repeat with a narrower width (e.g., 2Hz) and verify that 44Hz is now *not* attenuated significantly.

### 6.2. Yaw Kick Threshold Tests (`tests/test_ffb_engine.cpp`)
*   **Threshold Gates Noise**:
    *   Create a test case `test_yaw_kick_threshold`.
    *   Set `m_yaw_kick_threshold` to 5.0.
    *   Feed `mLocalRotAccel.y` = 2.0 (below threshold).
    *   Verify calculated Yaw Force is 0.0.
*   **Threshold Allows Signal**:
    *   Feed `mLocalRotAccel.y` = 6.0 (above threshold).
    *   Verify calculated Yaw Force is non-zero (proportional to gain).

## Prompt

Please proceed with the following task:

**Task: Implement Signal Processing Improvements**

**Context:**
This report outlines critical optimizations for the Force Feedback signal chain, specifically improving the Static Notch Filter's flexibility and the Yaw Kick effect's noise immunity. These changes are derived from user feedback "Troubleshooting 25".

**References:**
*   `docs\dev_docs\report_signal_processing_latency.md` (This Report)
*   `docs\dev_docs\FFB_formulas.md`
*   `docs\dev_docs\references\Reference - telemetry_data_reference.md`
*   `src/FFBEngine.h`
*   `src/GuiLayer.cpp`
*   `tests/test_ffb_engine.cpp`

**Implementation Requirements:**
1.  **Read and understand** the "Proposed Solution" (Section 2) and "Implementation Plan" (Section 3) of this document (`docs\dev_docs\report_signal_processing_latency.md`).
2.  **Modify `src/FFBEngine.h`**:
    *   Add `m_static_notch_width` and `m_yaw_kick_threshold` variables.
    *   Update `calculate_force` to use the bandwidth-based Q calculation for the notch filter.
    *   Update `calculate_force` to apply the threshold check for Yaw Kick.
3.  **Modify `src/GuiLayer.cpp`**:
    *   Add the "Filter Width" slider (0.1 - 10.0 Hz).
    *   Add the "Activation Threshold" slider (0.0 - 10.0 rad/s²).
4.  **Implement Automated Tests**:
    *   Add new test cases to `tests/test_ffb_engine.cpp` as detailed in **Section 6** (Automated Tests).
    *   Ensure all tests pass.
5.  **Update Documentation**:
    *   Update logic descriptions in `docs\dev_docs\FFB_formulas.md`.
    *   Update parameter lists in `docs\dev_docs\references\Reference - telemetry_data_reference.md`.
6.  **Update Version & Changelog**:
    *   Increment the version number in `VERSION`.
    *   Add a detailed entry in `CHANGELOG.md`.

**Build & Test Instructions:**
Use the following commands to build and test your changes. **ALL TESTS MUST PASS.**

*   **Update app version, compile main app, compile all tests (including windows tests), all in one single command:**
    ```powershell
    & 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release --clean-first
    ```

*   **Run all tests that had already been compiled:**
    ```powershell
    .\build\tests\Release\run_combined_tests.exe
    ```

**Deliverables:**
*   Updated source code files (`FFBEngine.h`, `GuiLayer.cpp`) that strictly follow the implementation plan.
*   Updated test file (`tests/test_ffb_engine.cpp`) with passing tests.
*   Updated `VERSION` and `CHANGELOG.md`.
*   Updated markdown documentation files.







