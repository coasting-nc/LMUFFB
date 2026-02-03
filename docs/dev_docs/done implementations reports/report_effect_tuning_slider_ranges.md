# Report: Effect Tuning & Slider Range Expansion

## 1. Introduction and Context
This report focuses on refining the user adjustability of the FFB effects. The "Troubleshooting 25" list highlights that several sliders are currently limited to ranges that prevent users from fully utilizing the effects (e.g., users needing 200% gain when the limit is 100%). Additionally, some legacy features like "Manual Slip Calculation" are cluttering the UI, and vibration effects lack pitch tuning, making them feel generic on different wheel hardware.

**Problems Identified:**
*   **Slider Ranges**: Many effects (Understeer, Steering Shaft Gain, ABS Pulse, Lockup, etc.) trigger clipping or are too weak at their current maximums. Users request up to 400% or 10x range increases in some cases.
*   **Obsolete Features**: "Manual Slip Calc" is no longer needed as the app now robustly handles slip via game telemetry or internal estimation fallback.
*   **Generic Vibration Frequencies**: Effects like Lockup, ABS, and Spin use hardcoded frequencies (e.g., 20Hz, 50Hz). These frequencies may resonate poorly or feel "toy-like" on high-end Direct Drive bases vs. belt-driven wheels.
*   **Tuning Gaps**: Defaults for Lockup Response Gamma need to be lower (0.1 instead of 0.5) to allow for more aggressive onset.

## 2. Proposed Solution

### 2.1. Slider Range Expansion
We will systematically update the Min/Max values in the GUI for the following:
*   **Understeer Effect**: Max 50 -> **200**.
*   **Steering Shaft Gain**: Max 100% (1.0) -> **200% (2.0)**.
*   **ABS Pulse Gain**: Max 2.0 -> **10.0**.
*   **Lockup Strength**: Max 2.0 -> **3.0** (or higher as requested, e.g. 300%).
*   **Brake Load Cap**: Max 3.0 -> **10.0**.
*   **Lockup Prediction Sensitivity**: Min 20 -> **10**.
*   **Lockup Rear Boost**: Max 3.0 -> **10.0**.
*   **Yaw Kick Gain**: Max 2.0 -> **1.0** (User reports it is "way too much", request to lower cap, or normalized). *Correction*: User asked "max at 100%... down from 200%". We will adjust range to 0.0-1.0 or keep 2.0 but scale internal math.
*   **Lateral G Boost**: Max 2.0 -> **4.0**.
*   **Lockup Gamma**: Min 0.5 -> **0.1**.

### 2.2. Frequency Tuning
*   **Configurable Frequencies**: Add new sliders for "Base Frequency" for:
    *   ABS Pulse (Default 20Hz)
    *   Lockup Vibration (Default based on speed, but add a scalar or base pitch)
    *   Spin Vibration (Default based on slip speed, add a scalar)
*   **Goal**: Allow users to tune the "pitch" of the effect to match their rig's resonant frequency.

### 2.3. Cleanup
*   Remove `m_use_manual_slip` boolean and all associated logic from `FFBEngine` and `GuiLayer`. The engine will always use the best available method.

## 3. Implementation Plan

### 3.1. `src/FFBEngine.h`
1.  **Remove manual slip toggle**.
2.  **Add Frequency Scalars**:
    ```cpp
    float m_abs_freq_hz = 20.0f;
    float m_lockup_freq_scale = 1.0f; // Multiplier for speed-based freq
    float m_spin_freq_scale = 1.0f;
    ```
3.  **Update Effect Logic**: Use these new variables in `calculate_force` instead of hardcoded constants.

### 3.2. `src/GuiLayer.cpp`
1.  **Update `FloatSetting` Calls**: Change the `min` and `max` arguments to the new target values listed in 2.1.
2.  **Add New Frequency Sliders**:
    *   Under **ABS**: "Pulse Frequency" (10Hz - 50Hz).
    *   Under **Baking & Lockup**: "Vibration Pitch" (0.5x - 2.0x).
3.  **Remove**: Hide/Delete the "Manual Slip Calc" checkbox.

## 4. Testing Plan

### 4.1. Range Validation
*   **Setup**: Open the app.
*   **Action**: Drag "Steering Shaft Gain" to the far right.
*   **Verification**: value should reach 200% (2.0).
*   **Action**: Set "Lockup Gamma" to 0.1.
*   **Verification**: Ensure no crash/divide-by-zero in the math (Gamma logic usually uses `pow(x, gamma)`, which is safe for positive x and gamma=0.1).

### 4.2. Frequency Tuning
*   **Setup**: Tune ABS Frequency to 50Hz (high).
*   **Action**: Drive and slam brakes to trigger ABS.
*   **Verification**: The tactile feedback should feel like a "buzz" rather than a "thump".
*   **Setup**: Tune ABS Frequency to 10Hz (low).
*   **Verification**: Feedback should feel like a slow "chug-chug-chug".

## 5. Documentation Updates

The following documents need to be updated to reflect the changes detailed in this report:

*   `docs\dev_docs\FFB_formulas.md`
*   `docs\dev_docs\references\Reference - telemetry_data_reference.md`

## 6. Automated Tests

### 6.1. Slider Limits Tests (`tests/test_ffb_engine.cpp`)
*   **Verify Max Values**:
    *   Although the Engine generally consumes float values without explicit clamping (clamping usually happens in GUI), we should verify that extreme values (e.g. 200% gain, 10.0 ABS gain) do not cause instabilities.
    *   Create `test_high_gain_stability`: Set gains to new maximums, run 1000 iter, ensure no NaNs/Inf.

### 6.2. Frequency Scalar Tests (`tests/test_ffb_engine.cpp`)
*   **ABS Frequency**:
    *   Create `test_abs_frequency_scaling`.
    *   Set `m_abs_freq_hz` to 20Hz. Check 1 full cycle duration in output samples.
    *   Set `m_abs_freq_hz` to 40Hz. Check that cycle duration is halved.
*   **Variable Pitch Tests**:
    *   Create `test_lockup_pitch_scaling`.
    *   Set `m_lockup_freq_scale` to 1.0 vs 2.0.
    *   Simulate a constant lockup condition.
    *   Verify that the output sine wave frequency doubles when scale is 2.0.

### 6.3. Manual Slip Removal Regression
*   **Verify Defaults**:
    *   Ensure `test_manual_slip_calculation` (if it exists) is removed or updated to reflect that manual slip logic is gone.
    *   Ensure standard slip tests still pass (the engine should use the fallback or telemetry data automatically).

## Prompt

Please proceed with the following task:

**Task: Implement Effect Tuning and Slider Range Expansion**

**Context:**
This report identifies limitations in current FFB adjustment ranges and missing features for frequency tuning. The goal is to give users more control over effect strength (up to 200-400% in some cases) and vibration pitch/character.

**References:**
*   `docs\dev_docs\report_effect_tuning_slider_ranges.md` (This Report)
*   `docs\dev_docs\FFB_formulas.md`
*   `docs\dev_docs\references\Reference - telemetry_data_reference.md`
*   `src/FFBEngine.h`
*   `src/GuiLayer.cpp`
*   `tests/test_ffb_engine.cpp`

**Implementation Requirements:**
1.  **Read and understand** the "Proposed Solution" (Section 2) and "Implementation Plan" (Section 3) of this document (`docs\dev_docs\report_effect_tuning_slider_ranges.md`).
2.  **Modify `src/FFBEngine.h`**:
    *   Remove manual slip logic (`m_use_manual_slip`).
    *   Add frequency scalar variables (`m_abs_freq_hz`, `m_lockup_freq_scale`, `m_spin_freq_scale`).
    *   Update `calculate_force` to use these new scalars.
3.  **Modify `src/GuiLayer.cpp`**:
    *   Update Min/Max values for `FloatSetting` calls as specified in Section 2.1.
    *   Add new sliders for ABS Pulse Frequency and Vibration Pitch.
    *   Remove the Manual Slip checkbox.
4.  **Implement Automated Tests**:
    *   Add new test cases to `tests/test_ffb_engine.cpp` as detailed in **Section 6** (Automated Tests).
    *   Ensure all tests pass.
5.  **Update Documentation**:
    *   Reflect removal of Manual Slip in `docs\dev_docs\FFB_formulas.md`.
    *   Document new sliders in `docs\dev_docs\references\Reference - telemetry_data_reference.md`.
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
*   Updated source code files (`FFBEngine.h`, `GuiLayer.cpp`) implementing the range changes and new sliders.
*   Updated test file (`tests/test_ffb_engine.cpp`) with passing tests.
*   Updated `VERSION` and `CHANGELOG.md`.
*   Updated markdown documentation files.


