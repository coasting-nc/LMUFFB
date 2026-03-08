# Implementation Plan - Issue #290: Decouple ABS Pulse and Lockup Vibration from Vibration Strength

## Context
Issue #290 reports that the ABS Pulse effect is not working when the "Vibration Strength" slider (`m_vibration_gain`) is set to zero. This occurs because the ABS Pulse and Lockup Vibration effects are currently bundled into the same summation group as road and slide textures, which are then multiplied by the global vibration gain.

## Design Rationale
### Physics and Feel
The "Vibration Strength" slider is intended to manage the "noise" floor of the FFB—secondary surface effects like road texture and tire scrubbing. However, ABS Pulse and Lockup Vibration are "signal" effects that provide critical telemetry about the car's state at the limit of adhesion. By bundling them together, a user who wants to eliminate "chatter" (low vibration strength) inadvertently loses essential tactile cues for braking. Decoupling these ensures that safety-critical and performance-critical vibrations remain governed by their own specific gain sliders.

### Architectural Integrity
The FFB summation pipeline should distinguish between "Surface/Environmental" effects (Road, Slide, Spin, Bottoming) and "Vehicle State" vibrations (ABS, Lockup). This change moves the pipeline towards a more categorized structure, improving predictability for the user.

## Reference Documents
- [GitHub Issue #290](https://github.com/coasting-nc/LMUFFB/issues/290)
- [FFBEngine.cpp Source Analysis](../../../src/FFBEngine.cpp)

## Codebase Analysis Summary
### Current Architecture
The `FFBEngine::calculate_force` function implements a split scaling model:
1. **Structural Group**: Scaled by `m_smoothed_structural_mult` (Dynamic Normalization). Includes base steering, SoP, Yaw Kick, etc.
2. **Texture Group**: Scaled by `m_vibration_gain`. Includes road texture, slide texture, spin vibration, bottoming, ABS pulse, and lockup rumble.
3. **Soft Lock**: Added at the very end in absolute Nm, bypassing most gains except the master gain.

### Impacted Functionalities
- **Summation Logic**: The calculation of `vibration_sum_nm` and `final_texture_nm` in `FFBEngine::calculate_force`.
- **Snapshot/Logging**: No direct change to snapshots as individual components are already captured, but the `total_output` will change.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Changes | Design Rationale |
|---|---|---|---|
| **ABS Pulse** | Move from `vibration_sum_nm` to a new `critical_vibs_nm` group. | Stays active regardless of "Vibration Strength" setting. Uses its own "Pulse Gain". | Ensure braking feedback is always available if enabled. |
| **Lockup Vibration** | Move from `vibration_sum_nm` to a new `critical_vibs_nm` group. | Stays active regardless of "Vibration Strength" setting. Uses its own "Lockup Strength". | Critical indicator of tire flatspotting risk and braking limits. |
| **Road Texture** | No change. | Muted if "Vibration Strength" is 0. | Environmental noise should be controllable. |
| **Slide Texture** | No change. | Muted if "Vibration Strength" is 0. | Tire friction feel is often considered "chatter" by some users. |
| **Spin Vibration** | No change. | Muted if "Vibration Strength" is 0. | Traction loss vibration. |
| **Bottoming Crunch** | No change. | Muted if "Vibration Strength" is 0. | Occasional suspension limit noise. |

## Proposed Changes
### `src/FFBEngine.cpp`
- **File**: `src/FFBEngine.cpp`
- **Logic**: Modify the texture summation block (around line 636).
- **Refinement**:
    ```cpp
    // Group A: Surface/Environmental (Governed by Vibration Strength)
    double surfaces_sum_nm = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch;

    // Group B: Vehicle State Indicators (Independent of Vibration Strength)
    double vehicle_state_vibs_nm = ctx.abs_pulse_force + ctx.lockup_rumble;

    // Final Texture Summation
    double final_texture_nm = (surfaces_sum_nm * (double)m_vibration_gain) + vehicle_state_vibs_nm + ctx.soft_lock_force;
    ```
- **Rationale**: This cleanly separates environmental noise from vehicle state signals while maintaining the absolute Nm scaling model for all texture-like effects (preventing them from being affected by dynamic normalization).

## Version Increment Rule
- Increment `VERSION` from `0.7.149` to `0.7.150`.

## Test Plan (TDD-Ready)
### New Test: `tests/test_issue_290_abs_vibration.cpp`
- **Design Rationale**: This test must prove that even when `m_vibration_gain` is 0.0, the ABS and Lockup signals still reach the final output. It also verifies that surface effects ARE muted.
- **Baseline + 3 new tests**.

#### Test Case 1: ABS Pulse Persistence
- **Inputs**: `m_vibration_gain = 0.0f`, `m_abs_pulse_enabled = true`, `m_abs_gain = 1.0f`.
- **Mock**: Trigger ABS (High brake pedal + high brake pressure rate).
- **Assertions**:
    - `FFBSnapshot::ffb_abs_pulse > 0.0f`
    - `FFBSnapshot::total_output != 0.0f` (Assuming other forces are zero/low)
- **Failure**: Output is 0.0 because of the `m_vibration_gain` multiplier.

#### Test Case 2: Lockup Vibration Persistence
- **Inputs**: `m_vibration_gain = 0.0f`, `m_lockup_enabled = true`, `m_lockup_gain = 1.0f`.
- **Mock**: Trigger Lockup (High wheel slip ratio).
- **Assertions**:
    - `FFBSnapshot::texture_lockup > 0.0f`
    - `FFBSnapshot::total_output != 0.0f`
- **Failure**: Output is 0.0.

#### Test Case 3: Surface Muting (Regression/Sanity)
- **Inputs**: `m_vibration_gain = 0.0f`, `m_road_texture_enabled = true`, `m_road_texture_gain = 1.0f`.
- **Mock**: Trigger road noise (Vertical deflection deltas).
- **Assertions**:
    - `FFBSnapshot::texture_road > 0.0f` (Component is calculated)
    - `FFBSnapshot::total_output == 0.0f` (Assuming other forces are zero)
- **Rationale**: Proves that the slider still works for its intended purpose.

## Deliverables
- [x] Modified `src/FFBEngine.cpp`
- [x] New test `tests/repro_issue_290.cpp`
- [x] Updated `VERSION`
- [x] Updated `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`
- [x] Implementation Notes (Unforeseen Issues, Plan Deviations, Challenges, Recommendations)

## Implementation Notes
### Unforeseen Issues
- The initial reproduction test failed to trigger wheel lockup because `mLocalVel.z` (car speed) was set but wheel rotation was not correctly offset to create slip. This was resolved by explicitly setting `mLongitudinalGroundVel` and `mLongitudinalPatchVel`.
- Running the combined test suite generated several artifact log files (e.g., `test_refactoring_noduplicate.log`) that are not ignored by git. These had to be manually deleted to ensure a clean PR.

### Plan Deviations
- The test file name was changed from `tests/test_issue_290_abs_vibration.cpp` to `tests/repro_issue_290.cpp` for brevity and consistency with other reproduction tests.
- The version increment was adjusted from `0.7.151` to `0.7.150` to strictly follow the "+1" rule after checking the baseline `VERSION` (0.7.149).

### Challenges
- Ensuring that ABS Pulse and Lockup Vibration still scale correctly in absolute Nm required verifying that they were added *after* the `m_vibration_gain` multiplier but *before* the DirectInput scaling.

### Recommendations
- Consider further grouping effects into "Physical Indicators" and "Haptic Textures" in the UI to match this internal decoupling.
