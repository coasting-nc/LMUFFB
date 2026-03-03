


```json
{
  "task_id": "upsample_ffb_part1",
  "plan_file": "docs/dev_docs/plans/plan_upsample_ffb_part1.md"
}
```

---

# Implementation Plan: Up-sample FFB Part 1 (100Hz to 400Hz)

## Context
The goal of this task is to up-sample the 100Hz telemetry data provided by Le Mans Ultimate (LMU) to match the 400Hz internal physics loop of `lmuFFB`. This is Part 1 of a two-stage upsampling initiative, focusing strictly on **Input Conditioning**. 

## Design Rationale (Overall)
Currently, the 100Hz telemetry signal appears as a "staircase" to the 400Hz FFB loop (one frame of change followed by three frames of identical data). This causes severe mathematical issues for algorithms relying on derivatives ($d/dt$), specifically the **Slope Detection** algorithm, which oscillates wildly between infinite and zero slope. By up-sampling the inputs, we convert these staircases into continuous ramps or smooth curves, restoring mathematical continuity, eliminating derivative spikes, and significantly improving the tactile smoothness of the Force Feedback.

## Reference Documents
*   [Upsampling Research Report](docs/dev_docs/reports/upsampling.md)

---

## Codebase Analysis Summary

### Current Architecture Overview
*   **`main.cpp`**: Runs a high-priority thread at 400Hz (`target_period(2500)` microseconds). It polls `GameConnector` and passes the `TelemInfoV01` struct to `FFBEngine::calculate_force`.
*   **`FFBEngine.cpp`**: Consumes the telemetry data. Currently, it reads `data->mWheel[i]` directly. Because the game updates at 100Hz, 3 out of 4 calls to `calculate_force` receive identical telemetry values.
*   **`MathUtils.h`**: Contains DSP utilities like `BiquadNotch` and `apply_adaptive_smoothing`.

### Impacted Functionalities
1.  **`FFBEngine::calculate_force`**: Needs to intercept raw telemetry, detect when a *new* 100Hz frame has arrived, apply upsampling filters, and use the upsampled values for all subsequent physics calculations.
2.  **`MathUtils.h`**: Needs new DSP classes for upsampling (`LinearExtrapolator` and `HoltWintersFilter`).

### Design Rationale
Intercepting the data at the very beginning of `calculate_force` and creating a local, upsampled copy of the `TelemWheelV01` array ensures that all downstream functions (like `calculate_grip`, `calculate_road_texture`) automatically benefit from the high-resolution data without needing to be individually rewritten.

---

## FFB Effect Impact Analysis

| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| **Slope Detection** | Inputs (`mLateralPatchVel`, `mLongitudinalPatchVel`) will be upsampled using Linear Extrapolation (Method C). | **Massive stability improvement.** Eliminates "Singularities" and "Binary Residence" issues. Grip loss detection will be much more accurate and less prone to false positives. |
| **Road Texture** | Input (`mVerticalTireDeflection`) will be upsampled using Linear Extrapolation (Method C). | **Removes 100Hz "buzz".** The texture will feel like actual road surface rather than a digital, robotic stepping artifact. |
| **Base Steering Torque** | Input (`mSteeringShaftTorque`) will be upsampled using Holt-Winters (Method B). | **Smoother, organic feel.** The wheel will load up in corners with a fluid, rubber-like resistance rather than a grainy, stepped resistance. |

### Design Rationale
*   **Method C (Linear Extrapolation / Inter-Frame)** is chosen for patch velocities and deflections because it guarantees a constant, non-zero derivative across the 400Hz sub-frames, which is a mathematical necessity for the Slope Detection's $d/dt$ calculations.
*   **Method B (Holt-Winters)** is chosen for Steering Shaft Torque because it acts as both an upsampler and a low-pass filter, rounding off the sharp edges of the 100Hz signal to create a more organic, less robotic feel in the user's hands.

---

## Proposed Changes

### 1. Add Upsampling DSP Classes to `src/MathUtils.h`
Add two new classes to the `ffb_math` namespace:

*   **`LinearExtrapolator`**:
    *   *State:* `m_last_input`, `m_current_output`, `m_rate`, `m_time_since_update`.
    *   *Logic:* When `is_new_frame` is true, calculate `m_rate = (raw_input - m_last_input) / 0.01`. When false, extrapolate: `m_current_output += m_rate * dt`. Clamp extrapolation time to 1.5x the game tick (15ms) to prevent runaway values if the game pauses.
*   **`HoltWintersFilter`**:
    *   *State:* `m_level`, `m_trend`, `m_initialized`.
    *   *Parameters:* `m_alpha = 0.4`, `m_beta = 0.2` (Hardcoded constants for now, as this is low-level DSP tuning, not a user preference).
    *   *Logic:* Predicts the current state based on the previous trend. Corrects the level and trend only when `is_new_frame` is true.

### 2. Update `src/FFBEngine.h`
Add state variables to track the filters and frame timing:
```cpp
// Inside FFBEngine class:
double m_last_telemetry_time = -1.0;

// Upsampler Instances
ffb_math::LinearExtrapolator m_upsample_lat_patch_vel[4];
ffb_math::LinearExtrapolator m_upsample_long_patch_vel[4];
ffb_math::LinearExtrapolator m_upsample_vert_deflection[4];
ffb_math::HoltWintersFilter  m_upsample_shaft_torque;
```

### 3. Modify `src/FFBEngine.cpp` (`calculate_force`)
At the very beginning of the function, before any physics calculations:
1.  **Detect New Frame:**
    ```cpp
    bool is_new_frame = (data->mElapsedTime != m_last_telemetry_time);
    if (is_new_frame) m_last_telemetry_time = data->mElapsedTime;
    ```
2.  **Upsample Shaft Torque:**
    ```cpp
    double upsampled_shaft_torque = m_upsample_shaft_torque.Process(data->mSteeringShaftTorque, ctx.dt, is_new_frame);
    // Use upsampled_shaft_torque instead of data->mSteeringShaftTorque for raw_torque_input (if source == 0)
    ```
3.  **Create Local Upsampled Wheels Array:**
    ```cpp
    TelemWheelV01 upsampled_wheels[4];
    for (int i = 0; i < 4; i++) {
        upsampled_wheels[i] = data->mWheel[i]; // Copy all raw data first
        
        // Overwrite specific channels with upsampled data
        upsampled_wheels[i].mLateralPatchVel = m_upsample_lat_patch_vel[i].Process(data->mWheel[i].mLateralPatchVel, ctx.dt, is_new_frame);
        upsampled_wheels[i].mLongitudinalPatchVel = m_upsample_long_patch_vel[i].Process(data->mWheel[i].mLongitudinalPatchVel, ctx.dt, is_new_frame);
        upsampled_wheels[i].mVerticalTireDeflection = m_upsample_vert_deflection[i].Process(data->mWheel[i].mVerticalTireDeflection, ctx.dt, is_new_frame);
    }
    ```
4.  **Update References:** Change `const TelemWheelV01& fl = data->mWheel[0];` to `const TelemWheelV01& fl = upsampled_wheels[0];` (and similarly for `fr`, and rear wheels). Ensure all helper functions receive the `upsampled_wheels` data.

### Parameter Synchronization Checklist
*No new user-facing settings are added in this phase. The upsampling is a fundamental signal conditioning improvement applied universally.*

### Initialization Order Analysis
The new DSP classes in `MathUtils.h` have no external dependencies. They will be instantiated as value members inside `FFBEngine`, ensuring they are initialized automatically when `FFBEngine` is constructed.

### Version Increment Rule
Increment the version number in `VERSION` and `src/Version.h` by the **smallest possible increment** (e.g., `0.7.107` -> `0.7.108`).

---

## Test Plan (TDD-Ready)

### Design Rationale
The tests must prove that the upsamplers correctly convert a 100Hz stepped signal into a continuous 400Hz signal. We need to verify that derivatives calculated across the sub-frames are non-zero and stable, which is the core requirement for fixing the Slope Detection algorithm.

### 1. `test_math_linear_extrapolator`
*   **Description:** Verifies `LinearExtrapolator` correctly ramps values between 100Hz frames.
*   **Inputs:** 
    *   Tick 1 (New Frame): Input 0.0, dt 0.0025
    *   Tick 2, 3, 4 (Old Frame): Input 0.0, dt 0.0025
    *   Tick 5 (New Frame): Input 1.0, dt 0.0025
    *   Tick 6, 7, 8 (Old Frame): Input 1.0, dt 0.0025
*   **Expected Output:** 
    *   Tick 5: 1.0 (Rate is calculated as 100.0 units/sec)
    *   Tick 6: 1.25
    *   Tick 7: 1.50
    *   Tick 8: 1.75
*   **Boundary Condition:** Test that if `is_new_frame` stays false for > 15ms, the output stops extrapolating (clamps) to prevent runaway values.

### 2. `test_math_holt_winters`
*   **Description:** Verifies `HoltWintersFilter` smooths and predicts noisy inputs.
*   **Inputs:** A stepped signal (0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0).
*   **Expected Output:** The output should form a smooth curve transitioning from 0.0 to 1.0, rather than a sharp linear ramp or a sudden step.

### 3. `test_ffb_upsampling_integration`
*   **Description:** Verifies `FFBEngine::calculate_force` utilizes the upsampled data.
*   **Inputs:** Feed `calculate_force` with 4 ticks of telemetry. Tick 1 has `mElapsedTime = 0.01` and `mLateralPatchVel = 1.0`. Ticks 2, 3, 4 have `mElapsedTime = 0.01` and `mLateralPatchVel = 1.0`.
*   **Expected Output:** By inspecting the `FFBSnapshot` (or using `FFBEngineTestAccess`), verify that the internal `raw_front_lat_patch_vel` changes across ticks 2, 3, and 4, proving the upsampler is active in the main pipeline.

**Test Count Specification:** Baseline + 3 new test cases.

---

## Deliverables
- [ ] Update `src/MathUtils.h` with `LinearExtrapolator` and `HoltWintersFilter`.
- [ ] Update `src/FFBEngine.h` with filter instances and timing state.
- [ ] Update `src/FFBEngine.cpp` to implement the `is_new_frame` logic and apply filters to the local `upsampled_wheels` array.
- [ ] Create `tests/test_upsampling.cpp` with the specified TDD tests.
- [ ] Increment version in `VERSION` and `src/Version.h`.
- [ ] Update this plan document with "Implementation Notes" (Unforeseen Issues, Plan Deviations, Challenges, Recommendations) upon completion.