# Implementation Plan - Issue #402: Fix Time-Domain Independence Bug in Road Texture effect

## Context
The Road Texture effect currently uses per-frame deltas of suspension deflection and vertical acceleration to calculate FFB force. This makes the effect's strength dependent on the physics update frequency (e.g., it is 4x weaker at 400Hz than at 100Hz).

## Design Rationale
To achieve Time-Domain Independence (TDI), we must convert per-frame position/acceleration deltas into true time-based derivatives (velocity and jerk) by dividing by `dt`. To maintain the "feel" that was originally tuned at 100Hz, we will apply a normalization factor of 0.01 (the `dt` at 100Hz). This ensures that a given physical event (e.g., the wheel moving at 0.5 m/s) results in the same FFB output regardless of the loop frequency.

## Reference Documents
- GitHub Issue #402: "Fix Time-Domain Independence Bug in Road Texture effect"

## Codebase Analysis Summary
- **Affected File:** `src/ffb/FFBEngine.cpp`
- **Affected Function:** `FFBEngine::calculate_road_texture`
- **Impact:** The internal calculations for `road_noise_val` will be updated to include `ctx.dt`. Constants used for outlier rejection and activity thresholds will be scaled to be time-independent.

## FFB Effect Impact Analysis
| FFB Effect | Technical Changes | Expected User-Facing Changes |
| :--- | :--- | :--- |
| Road Texture | Convert deltas to derivatives by dividing by `dt`. Scale thresholds. | Consistency across different frame rates. At 400Hz (standard production), the effect will be 4x stronger than before, restoring its intended intensity. |

## Proposed Changes
### `src/ffb/FFBEngine.cpp`
- In `FFBEngine::calculate_road_texture`:
    - Calculate `vel_l` and `vel_r` as `(current_deflection - prev_deflection) / ctx.dt`.
    - Update `max_vel` to be `DEFLECTION_DELTA_LIMIT / 0.01` (~1.0 m/s) to maintain the same physical limit.
    - Clamp `vel_l` and `vel_r` using `max_vel`.
    - Update `deflection_active` check to use scaled `DEFLECTION_ACTIVE_THRESHOLD / 0.01`.
    - Calculate `road_noise_val` using `(vel_l + vel_r) * DEFLECTION_NM_SCALE * 0.01`.
    - In the fallback logic, calculate `jerk = (vert_accel - m_prev_vert_accel) / ctx.dt`.
    - Calculate `road_noise_val` using `jerk * ACCEL_ROAD_TEXTURE_SCALE * DEFLECTION_NM_SCALE * 0.01`.

### `VERSION`
- Increment version to 0.7.200.

### `CHANGELOG_DEV.md`
- Add entry for Issue #402 fix.

## Test Plan (TDD-Ready)
### New Test Case: `test_road_texture_time_domain_independence` in `tests/test_issue_402_repro.cpp`
- **Goal:** Verify that the Road Texture effect outputs the same force at 100Hz and 400Hz for the same physical input.
- **Inputs:**
    - Constant suspension velocity: 0.5 m/s.
    - Test 1: `dt = 0.01` (100Hz), move deflection by `0.5 * 0.01`.
    - Test 2: `dt = 0.0025` (400Hz), move deflection by `0.5 * 0.0025`.
- **Expected Output:** `force_100hz` should be nearly equal to `force_400hz`.
- **Assertions:**
    - `ASSERT_GT(force_100hz, 0.1f)` (ensure effect is active).
    - `ASSERT_NEAR(force_100hz, force_400hz, 0.01f)`.

## Deliverables
- [x] Modified `src/ffb/FFBEngine.cpp`.
- [x] New test file `tests/test_issue_402_repro.cpp`.
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`.
- [x] Implementation Notes (final update to this plan).

## Implementation Notes
- **Unforeseen Issues**:
  - The initial regression test failed with `force = 0` because of the "Seeding Gate" and "EMA Stabilization".
  - The "Seeding Gate" (Issue #379) suppresses output on the first frame after a car change/teleport.
  - The `texture_load_factor` uses an EMA with a 100ms time constant, requiring several frames to reach steady state.
- **Plan Deviations**:
  - Updated the regression test to simulate 1 second of physics (100 or 400 frames) to allow all filters and gates to stabilize.
- **Challenges**:
  - Identifying why the force was zero required tracing the `texture_load_factor` back to `m_smoothed_vibration_mult` and its EMA logic.
- **Recommendations**:
  - Future time-domain independence audits should also use the `PumpEngineTime` helper or similar long-duration simulation to avoid transient artifacts.
