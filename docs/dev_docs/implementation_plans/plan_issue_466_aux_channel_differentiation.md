# Implementation Plan - Issue #466: Differentiate among the auxiliary channels: apply different setting for the zero latency reconstruction filter

## Context
Following the migration of all auxiliary telemetry channels to the `HoltWintersFilter` (Issue #461), this patch introduces signal-specific tuning. Not all telemetry signals share the same noise profile. Treating smooth driver inputs the same as violent chassis impacts leads to either sluggish responsiveness or dangerous mathematical spikes. This plan categorizes the 32 auxiliary channels (covering 22 distinct logical telemetry types) into three distinct DSP groups (Driver Inputs, High-Frequency Texture, and Chassis Kinematics) and applies hardcoded `alpha`, `beta`, and `ZeroLatency` rules tailored to their physical characteristics.

### Design Rationale
In Digital Signal Processing (DSP), extrapolating a noisy signal is dangerous. The `HoltWintersFilter` in "Zero Latency" mode uses the signal's trend (derivative) to predict the future. 
1. If we extrapolate a 50G kerb strike (Chassis Kinematics), the trend points to infinity, causing a massive artificial FFB jolt. These must be heavily smoothed and interpolated.
2. If we delay steering angle (Driver Inputs), derivative-based effects like Gyro Damping feel like thick mud. These must always be zero-latency.
3. High-frequency vibrations (Texture) are subjective. These should remain tied to the user's UI preference.
4. Calling `SetZeroLatency()` on 22 filters every tick inside `calculate_force` (400Hz) wastes CPU cycles. State changes should only occur when the configuration actually changes.

## Reference Documents
*   previous implementation plan for applying HoltWintersFilter to all auxiliary telemetry channels: `docs/dev_docs/implementation_plans/issue_461_auxiliary_reconstruction_plan.md`
*   `docs\dev_docs\reports\latency and load sensitivity for tire grip.md` (Context on grip and load noise)
*   Issue #459 / #461 Discussions (Latency vs. Smoothness Trade-off)

---

## Codebase Analysis Summary
*   **`src/ffb/FFBEngine.h`**: Contains the 32 `HoltWintersFilter` instances for auxiliary channels. Needs a state-tracker variable (`m_last_aux_recon_mode`) and a new private method (`ApplyAuxReconstructionMode()`) to handle state changes cleanly.
*   **`src/ffb/FFBEngine.cpp`**: 
    *   *Constructor:* Needs to initialize the `alpha` and `beta` parameters for all 32 filters using `Configure()`.
    *   *`calculate_force`:* The 400Hz hot path. Needs to be stripped of unconditional `SetZeroLatency()` calls and replaced with a lightweight state-change detector.

### Design Rationale
By encapsulating the mode-switching logic into `ApplyAuxReconstructionMode()` and triggering it via a simple integer comparison (`if (m_advanced.aux_telemetry_reconstruction != m_last_aux_recon_mode)`), we completely remove configuration overhead from the 400Hz physics loop while ensuring the engine remains self-contained and doesn't require complex callback plumbing from the GUI or Config layers.

---

## FFB Effect Impact Analysis

| Affected FFB Effect | Technical Change | User-Facing Change (Feel) |
| :--- | :--- | :--- |
| **Gyro Damping** | `m_upsample_steering` forced to Zero Latency (Alpha 0.95, Beta 0.40). | **Highly Responsive.** Eliminates the "thick mud" feeling during rapid counter-steering. The wheel will feel instantly connected to the driver's hands. |
| **Road / Slide Texture** | `m_upsample_vert_deflection`, `m_upsample_patch_vel` tied to UI toggle (Alpha 0.80, Beta 0.20). | **User Choice.** Direct Drive users get raw, unfiltered road detail. Belt users get smooth, rattle-free feedback. |
| **Suspension Bottoming** | `m_upsample_susp_force` forced to Smooth/Interpolated (Alpha 0.50, Beta 0.00). | **Safe & Realistic.** Prevents false-positive "crunch" effects caused by 1-frame telemetry spikes when hitting aggressive kerbs. |
| **Seat of Pants (SoP)** | `m_upsample_local_accel` forced to Smooth/Interpolated (Alpha 0.50, Beta 0.00). | **Smooth Weight Transfer.** Eliminates harsh, robotic jolts during collisions or polygon-edge strikes, providing a fluid sense of chassis roll. |

### Design Rationale
This targeted approach ensures that the FFB feels raw and detailed where it matters (steering, tires) and smooth and safe where it is required (chassis impacts), maximizing both fidelity and hardware safety.

---

## Proposed Changes

### 1. `src/ffb/FFBEngine.h`
*   **Add State Tracker:** Add `int m_last_aux_recon_mode = -1;` to the private members.
*   **Add Method:** Add `void ApplyAuxReconstructionMode();` to the private methods.

### 2. `src/ffb/FFBEngine.cpp`
*   **Constructor Updates:** Add `Configure(alpha, beta)` calls for all 32 filters immediately after they are constructed.
    *   *Group 1 (Driver Inputs):* `m_upsample_steering`, `m_upsample_throttle`, `m_upsample_brake`, `m_upsample_brake_pressure[0-3]`. **Tuning:** `Configure(0.95, 0.40)`.
    *   *Group 2 (Texture/Tire):* `m_upsample_vert_deflection[0-3]`, `m_upsample_lat_patch_vel[0-3]`, `m_upsample_long_patch_vel[0-3]`, `m_upsample_rotation[0-3]`. **Tuning:** `Configure(0.80, 0.20)`.
    *   *Group 3 (Chassis/Impacts):* `m_upsample_local_accel_x/y/z`, `m_upsample_local_rot_accel_y`, `m_upsample_local_rot_y`, `m_upsample_susp_force[0-3]`. **Tuning:** `Configure(0.50, 0.00)`.
*   **Implement `ApplyAuxReconstructionMode()`:**
    ```cpp
    void FFBEngine::ApplyAuxReconstructionMode() {
        bool user_wants_raw = (m_advanced.aux_telemetry_reconstruction == 0);
        
        // Group 1: ALWAYS Zero Latency
        m_upsample_steering.SetZeroLatency(true);
        // ... (apply to rest of Group 1)

        // Group 2: TIED TO UI TOGGLE
        for(int i=0; i<4; i++) m_upsample_vert_deflection[i].SetZeroLatency(user_wants_raw);
        // ... (apply to rest of Group 2)

        // Group 3: ALWAYS Smooth (Interpolated)
        m_upsample_local_accel_x.SetZeroLatency(false);
        // ... (apply to rest of Group 3)

        m_last_aux_recon_mode = m_advanced.aux_telemetry_reconstruction;
    }
    ```
*   **Hot-Path Cleanup (`calculate_force`):**
    *   Remove the block of 22 `SetZeroLatency()` calls.
    *   Replace with:
        ```cpp
        if (m_advanced.aux_telemetry_reconstruction != m_last_aux_recon_mode) {
            ApplyAuxReconstructionMode();
        }
        ```

### 3. Versioning
*   Increment `VERSION` to `0.7.221`.
*   Add an entry to `CHANGELOG_DEV.md`.

### Design Rationale
The groupings are derived from the physical nature of the signals. Driver inputs are clean and latency-critical. Texture signals represent "feel" and should remain under user control. Chassis kinematics are inherently noisy and can contain frame-singularities that must be interpolated to prevent hardware-damaging FFB jolts.

---

## Test Plan (TDD-Ready)

### Design Rationale
Because `HoltWintersFilter` encapsulates its state, we will use `FFBEngineTestAccess` (or verify behavioral outputs) to ensure the groups are correctly configured and that the UI toggle only affects Group 2.

**Test 1: `test_aux_channel_group_initialization`**
*   **Description:** Verifies that the `FFBEngine` constructor correctly applies the hardcoded `alpha` and `beta` values to representative channels from each of the 3 groups.
*   **Expected Output:** 
    *   `m_upsample_steering` (Group 1) has alpha=0.95, beta=0.40.
    *   `m_upsample_vert_deflection[0]` (Group 2) has alpha=0.80, beta=0.20.
    *   `m_upsample_local_accel_x` (Group 3) has alpha=0.50, beta=0.00.
*   **Assertion:** Will fail until `Configure()` is called in the constructor.

**Test 2: `test_aux_channel_mode_switching`**
*   **Description:** Verifies that changing `m_advanced.aux_telemetry_reconstruction` and calling `calculate_force` correctly updates the `ZeroLatency` state of the filters.
*   **Expected Output:**
    *   When config is `0` (Zero Latency): Group 1 is `true`, Group 2 is `true`, Group 3 is `false`.
    *   When config is `1` (Smooth): Group 1 is `true`, Group 2 is `false`, Group 3 is `false`.
*   **Assertion:** Will fail until `ApplyAuxReconstructionMode()` is implemented and hooked into the hot path.

**Test 3: `test_hot_path_efficiency` (Optional/Mock)**
*   **Description:** Verifies that `ApplyAuxReconstructionMode()` is only called once when the configuration changes, not every frame.

---

## Deliverables
- [x] **Code Changes:** `FFBEngine.h`, `FFBEngine.cpp`, `MathUtils.h`.
- [x] **Tests:** New test cases in `tests/test_reconstruction.cpp` covering initialization and mode switching. Updated regression tests.
- [x] **Documentation:** Update `CHANGELOG_DEV.md` and `VERSION` to `0.7.221`.

## Implementation Notes

### File Deletion Incident
During the development of this issue, a significant portion of the codebase and project documentation (including this implementation plan) was unexpectedly deleted. This occurred during an environment cleanup and synchronization phase where local working copies were misaligned with the intended repository state. To recover:
1.  Core source files (`FFBEngine.cpp`, `FFBEngine.h`, `MathUtils.h`) were restored manually from the trace history and re-verified.
2.  The implementation plan was reconstructed to ensure project continuity and traceability.
3.  All regression tests were re-run (617/617 passing) to guarantee that no regressions were introduced during the restoration.

### Encountered Issues
- **Beta Clamp**: The `HoltWintersFilter::Configure` method had a hardcoded minimum clamp for `beta` of 0.01. This prevented setting `beta = 0.0` for Group 3 (Chassis/Impacts) as requested. I relaxed this clamp to `0.0`.
- **Regression Test Baselines**: Changing the Alpha/Beta and Latency modes for auxiliary channels (suspension force, patch velocities, accelerations) naturally changed the final FFB output for the same telemetry. This required re-baselining `EXPECTED_VALUE` in several consistency tests.
- **Warning Logic Timing**: `test_mn_missing_susp_force_warning` failed because `m_upsample_susp_force` is now Group 3 (Smooth/Interpolated, Alpha 0.5). A sudden drop to 0 in raw telemetry now results in an exponential decay in the engine, taking ~9 frames to drop below the 10N threshold. I increased the test's frame count to 100 to ensure the warning reliably triggers.
- **Initialization/Seeding**: `test_mn_missing_susp_force_warning` also required direct `Preset::ApplyDefaultsToEngine` and `ApplyAuxReconstructionMode` calls to avoid the filter-seeding logic in `InitializeEngine` which was interfering with the frame counter.

### Deviations from the Plan
- **Relaxed Beta Clamp**: Added a small change to `src/utils/MathUtils.h` to allow `beta = 0.0`.
- **Test Setups**: Modified `tests/test_ffb_magic_numbers.cpp` to use a manual engine setup for the warning counter test instead of the standard `InitializeEngine` helper.

### Suggestions for the Future
- Monitor user feedback on the "Smooth" feel of Group 3. While mathematically safer, some users might prefer the "kick" of raw acceleration spikes.
- Consider if other channels (like Steering Shaft Torque) should also be categorized into these groups or maintain their own specific settings.
