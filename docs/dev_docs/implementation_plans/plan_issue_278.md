# Implementation Plan - Issue #278: Derived Acceleration for FFB Effects

**GitHub Issue:** [#278](https://github.com/coasting-nc/LMUFFB/issues/278) Investigate if there are FFB effects other than Yaw Kick that use acceleration values which should recalculated from velocity to avoid spikes

**Investigation Reference:** [docs/dev_docs/investigations/ffb_effects_acceleration_analysis.md](../../investigations/ffb_effects_acceleration_analysis.md)

## 1. Context
The LMU/rF2 telemetry provided at 100Hz contains noisy acceleration channels (`mLocalAccel`). These channels often exhibit high-frequency spikes due to stiff suspension physics and aliasing. When used in FFB effects, especially those involving derivatives (like "Jerk" in Road Texture), these spikes manifest as harsh, non-physical "metallic clacks" or inconsistent vibrations.

> **Design Rationale:**
> Following the successful implementation of derived yaw acceleration for Yaw Kick (Issue #241), extending this approach to vertical and longitudinal acceleration will improve the overall refinement and reliability of the FFB engine, especially for cars with encrypted telemetry where fallbacks are active.

## 2. Analysis
The investigation identified two primary areas of concern:
1.  **Vertical Acceleration (`mLocalAccel.y`)**: Used as a fallback for Road Texture. Relying on its derivative (Jerk) amplifies noise significantly.
2.  **Longitudinal Acceleration (`mLocalAccel.z`)**: Used in predictive lockup detection. Noise can cause inconsistent ABS/Lockup pulses.

Lateral Acceleration (`mLocalAccel.x`) is considered low risk due to existing EMA smoothing in SoP and Kinematic Load calculations.

> **Design Rationale:**
> Velocity-based derivation is preferred because velocity is an integrated quantity and naturally smoother. By deriving acceleration from velocity at the source (100Hz) and then upsampling the result to 400Hz, we can provide a clean, continuous signal for all downstream FFB effects.

## 3. Proposed Changes

### FFBEngine.h
- Add `m_prev_local_vel` (`TelemVect3`) to track velocity at 100Hz ticks.
- Add `m_local_vel_seeded` (`bool`) to initialize the velocity state.

### FFBEngine.cpp
- Update `calculate_force` to derive acceleration from velocity:
    - In the `is_new_frame` block (100Hz tick):
        - Calculate `derived_accel_y = (data->mLocalVel.y - m_prev_local_vel.y) / data->mDeltaTime` (with safety check for `dt`).
        - Calculate `derived_accel_z = (data->mLocalVel.z - m_prev_local_vel.z) / data->mDeltaTime`.
        - Update `m_prev_local_vel = data->mLocalVel`.
    - Pass these derived 100Hz accelerations into the existing `m_upsample_local_accel_y` and `m_upsample_local_accel_z` extrapolators.
- Update `m_prev_vert_accel` (used in Road Texture) at the end of `calculate_force`:
    - Use `upsampled_data->mLocalAccel.y` instead of raw `data->mLocalAccel.y`.
- Update "Reset filters" logic to include the new velocity state variables.

### Versioning
- Increment `VERSION` to `0.7.145`.
- Update `CHANGELOG_DEV.md`.

> **Design Rationale:**
> Reusing existing extrapolators minimizes architectural changes while providing the desired 400Hz continuity. Overwriting `m_working_info` ensures all existing effects (Road Texture, Lockup, Kinematic Load) benefit immediately without modification to their internal logic.

## 4. Test Plan

### Automated Unit Tests
1.  **test_issue_278_road_texture_spike_rejection**:
    *   Mock a scenario where `mLocalAccel.y` has massive 1-frame spikes (e.g. 50G) but `mLocalVel.y` remains smooth.
    *   Verify that `texture_road` does not produce massive spikes.
2.  **test_issue_278_lockup_continuity**:
    *   Verify that `car_dec_ang` used in lockup detection is smooth even when raw longitudinal acceleration is noisy.
3.  **test_issue_278_400hz_jerk**:
    *   Verify that `delta_accel` in Road Texture is non-zero and consistent across 400Hz frames when there is a steady change in derived acceleration.

### Manual Verification (Linux Mock)
- Use `run_combined_tests` to ensure no regressions in FFB force output for existing car presets.

> **Design Rationale:**
> Tests focus on the "Rectification" problem. By proving that noise in the raw acceleration channel is ignored in favor of the clean velocity-derived signal, we validate the core fix.

## 5. Implementation Steps

1.  **Modify `src/FFBEngine.h`**: Add `m_prev_local_vel` and `m_local_vel_seeded` to the `FFBEngine` class.
2.  **Verify `src/FFBEngine.h`**: Read the file to ensure the new members were added correctly.
3.  **Modify `src/FFBEngine.cpp` (Seeding and Reset)**: Update `calculate_force` and the transition logic to handle `m_prev_local_vel` and `m_local_vel_seeded`.
4.  **Modify `src/FFBEngine.cpp` (Acceleration Derivation)**: Update `calculate_force` to derive Y and Z acceleration from velocity at 100Hz.
5.  **Modify `src/FFBEngine.cpp` (Upsampling and Jerk Update)**: Update the extrapolators to use derived values and update `m_prev_vert_accel` with the upsampled value.
6.  **Verify `src/FFBEngine.cpp`**: Read the file to ensure all logic changes are correct.
7.  **Create `tests/test_issue_278_derived_acceleration.cpp`**: Implement the unit tests described in the Test Plan.
8.  **Verify `tests/test_issue_278_derived_acceleration.cpp`**: Read the file to ensure the tests are correctly implemented.
9.  **Build the project**: Run `cmake --build build` to compile the changes.
10. **Run Tests**: Execute `./build/tests/run_combined_tests` and ensure all tests pass.
11. **Autonomous Loop & Code Review**:
    - Commit changes.
    - Request a code review.
    - Save the review as `docs/dev_docs/code_reviews/issue_278_review_iteration_X.md`.
    - If issues are found, fix them and repeat the loop.
12. **Update `VERSION` and `CHANGELOG_DEV.md`**: Increment the version and record the changes.
13. **Complete Pre-Commit Steps**: Ensure proper testing, verification, review, and reflection are done.
14. **Submit**: Finalize the task with a descriptive commit message.

## 6. Implementation Notes

### Challenges
- **Test Fragility**: Several existing regression tests (e.g., `test_chassis_inertia_smoothing_convergence`, `test_kinematic_load_braking`) were failing initially. These tests manually set `mLocalAccel` without providing matching `mLocalVel` changes. Because the engine now derives acceleration from velocity, these tests produced 0 acceleration. They were updated to provide consistent velocity deltas and increment `mElapsedTime` to trigger the 100Hz frame logic.
- **Seeding Sensitivity**: Ensuring the derivative doesn't spike on the very first frame required careful placement of the `m_local_vel_seeded` logic to capture the initial velocity before any subtraction occurs.

### Deviations from Plan
- **State Management**: Instead of using static variables for 100Hz derived values, I added `m_derived_accel_y_100hz` and `m_derived_accel_z_100hz` as class members. This allows them to be properly reset in the transition logic, preventing "carry-over" acceleration when moving between different car types or from garage to track.

### Difference with the Yaw Acceleration Fix
The implementation for vertical (Y) and longitudinal (Z) acceleration differs from the existing Yaw acceleration derivation (Issue #241) in three key ways:
1.  **Derivation Point**: Yaw acceleration is derived from *upsampled* yaw rate at 400Hz within the effect logic. Vertical and longitudinal accelerations are derived from *raw* 100Hz velocity at the telemetry ingestion point.
2.  **Upsampling Architecture**: For Y and Z, we derive 100Hz acceleration and then pass it through a `LinearExtrapolator` to reach 400Hz. This preserves signal continuity and phase for high-frequency effects like Road Texture fallback. Yaw acceleration uses a simpler delta at 400Hz followed by heavy EMA smoothing.
3.  **Pipeline Position**: By deriving Y and Z acceleration at the source and overwriting `m_working_info`, the clean signals are automatically propagated to all downstream effects (Road Texture, Predictive Lockup, and Kinematic Load) without modifying their internal logic. The Yaw fix was local to the Yaw Kick effect.

### Code Review Iteration 1
- **Issue:** Junk log files included in commit.
- **Resolution:** Deleted `test_refactoring_noduplicate.log` and `test_refactoring_sme_names.log`.
- **Issue:** Missing `VERSION` and `CHANGELOG_DEV.md` updates.
- **Resolution:** Planned for next steps in the implementation.
- **Issue:** Review record missing.
- **Resolution:** Saved as `docs/dev_docs/code_reviews/issue_278_review_iteration_1.md`.

### Code Review Iteration 2
- **Result:** Greenlight (#Correct#).
- **Status:** All identified issues resolved. Documentation and versioning complete.

## 7. Additional Questions
- None at this stage.
