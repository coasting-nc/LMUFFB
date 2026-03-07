# Implementation Plan - Fix Yaw Kick Signal Rectification (#241)

## Context
Issue #241 reports that driving the LMP2 with stiff dampers causes strange constant pulls in the FFB. Analysis reveals that this is caused by a mathematical flaw in the Yaw Kick effect calculation where a hard threshold is applied *before* a low-pass filter. This "rectifies" high-frequency symmetric noise into a DC offset, perceived as a constant pull (the "diode effect").

**Design Rationale:**
High-frequency vibrations from stiff dampers cause the raw yaw acceleration signal to oscillate rapidly. When a hard threshold is applied to this raw signal, one side of the oscillation might be gated more than the other (if there's a slight bias), leading to a non-zero average. A subsequent low-pass filter then smooths this non-zero average into a steady directional pull. By smoothing *before* thresholding, we extract the true macroscopic movement of the car first, which correctly averages out the high-frequency noise.

## Analysis
- **`src/FFBEngine.cpp`**: The `calculate_sop_lateral` function implements the Yaw Kick logic. It currently gates `raw_yaw_accel` to 0.0 if it's below `m_yaw_kick_threshold` before updating the `m_yaw_accel_smoothed` filter.
- **`src/AsyncLogger.h`**: The `LogFrame` struct and CSV writing logic lack detailed Yaw Kick telemetry, making it difficult to diagnose this issue in user-submitted logs.
- **`tools/lmuffb_log_analyzer`**: The Python tool lacks specific plots to visualize the rectification effect.

**Design Rationale:**
The fix requires a fundamental change in signal processing order: Smoothing -> Thresholding. Additionally, to avoid sudden jumps in force when the threshold is crossed, a continuous deadzone (subtraction) should be used instead of a hard binary gate.

## Proposed Changes

### 1. `src/AsyncLogger.h`
- **Update `LogFrame` struct**: Add `raw_yaw_accel` and `ffb_yaw_kick`.
- **Update `WriteHeader`**: Add "RawYawAccel,FFBYawKick," to the header.
- **Update `WriteFrame`**: Add these fields to the CSV output.

**Design Rationale:**
Providing visibility into the raw signal vs. the smoothed and processed output is critical for verifying that the "diode effect" is gone.

### 2. `src/FFBEngine.cpp`
- **Modify `FFBEngine::calculate_sop_lateral`**:
    - Move the update of `m_yaw_accel_smoothed` to the top of the Yaw Kick section, using the raw `data->mLocalRotAccel.y`.
    - Apply the speed check `MIN_YAW_KICK_SPEED_MS`.
    - Implement the continuous deadzone logic on the *smoothed* value:
      ```cpp
      double processed_yaw = 0.0;
      if (std::abs(m_yaw_accel_smoothed) > (double)m_yaw_kick_threshold) {
          processed_yaw = m_yaw_accel_smoothed - std::copysign((double)m_yaw_kick_threshold, m_yaw_accel_smoothed);
      }
      ```
- **Update `FFBEngine::calculate_force`**: Populate the new `LogFrame` fields in the logging block.

**Design Rationale:**
Smoothing first ensures the noise is zeroed out before it can be rectified. The continuous deadzone ensures that when the car actually snaps, the force ramps up naturally from zero rather than "kicking" the wheel with the full threshold value.

### 3. `tools/lmuffb_log_analyzer/plots.py`
- **Add `plot_yaw_kick_analysis`**: A diagnostic plot showing Raw Yaw Accel, Smoothed Yaw Accel, and the resulting Yaw Kick Force.

**Design Rationale:**
Visual confirmation in the analyzer helps communicate the fix to users and simplifies future debugging.

### 4. `VERSION` and `CHANGELOG.md`
- Increment version to `0.7.124`.
- Document the fix and the improved Yaw Kick formula.

## Test Plan

### 1. Regression Test (`tests/test_issue_241_yaw_kick_rectification.cpp`)
- **Scenario A (Noise Rectification)**:
    - Input: `mean = 0.1`, `noise = +/- 1.0` at 400Hz.
    - Threshold: `0.5`.
    - **Expected**: Output force remains near-zero (representing the 0.1 mean) instead of developing a massive offset.
- **Scenario B (Continuous Deadzone)**:
    - Input: `1.5`, Threshold: `1.0`.
    - **Expected**: Output force corresponds to `0.5` rad/s², not `1.5`.

### 2. Suite Execution
- Run `./build/tests/run_combined_tests` to ensure no regressions in existing `test_ffb_yaw_gyro.cpp`.

## Implementation Notes
*(To be populated during development)*
