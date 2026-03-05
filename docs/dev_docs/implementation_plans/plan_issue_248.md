# Implementation Plan - Issue #248: Soft lock is just a bump

## Context
The current soft lock implementation provides a "bump" of resistance when hitting the steering rack limit, but the resistance "disappears" or becomes weak if the user tries to push further or once the velocity drops. This is because the damping component is very strong compared to the spring component for realistic steering excesses. A proper soft lock should act as a solid physical stop.

## Design Rationale
- **Physics-based reasoning**: A steering rack limit is a hard physical stop. In FFB terms, this should be represented by the maximum torque the wheelbase can provide, reached very quickly once the limit is exceeded.
- **Reliability & Safety**: The soft lock must be strong enough to stop the wheel but must not cause unstable oscillations. Damping is necessary but should not overshadow the static resistance (spring).
- **Architectural Integrity**: The soft lock force should scale with the hardware's maximum torque to ensure it can always reach the 100% FFB output limit regardless of the wheelbase model.

## Reference Documents
- GitHub Issue #248: Soft lock is just a bump (not really a physical stop), as soon as it's engaged it disappears.
- Telemetry Data Reference: `docs/dev_docs/references/Reference - telemetry_data_reference.md`

## Codebase Analysis Summary
- **FFBEngine::calculate_soft_lock** in `src/SteeringUtils.cpp`: This is where the soft lock force is currently calculated using a linear spring and a damping component.
- **FFBEngine::calculate_force** in `src/FFBEngine.cpp`: Soft lock is summed into the absolute Nm (Texture) group and scaled by `wheelbase_max_nm`.
- **BASE_NM_SOFT_LOCK** in `src/FFBEngine.h`: A constant currently set to 50.0.

### Impact Zone:
- `src/FFBEngine.h`: May need to adjust or remove `BASE_NM_SOFT_LOCK`.
- `src/SteeringUtils.cpp`: Rewrite `calculate_soft_lock` to use a more aggressive scaling based on hardware limits.
- `tests/test_ffb_soft_lock.cpp`: Update tests to reflect new scaling and verify wall-like behavior.
- `tests/test_issue_181_soft_lock_weakness.cpp`: Update regression test.

## FFB Effect Impact Analysis
| Effect | Technical Changes | Expected User-Facing Changes |
| :--- | :--- | :--- |
| Soft Lock | Change formula from `excess * stiffness * 50` to a steeper ramp reaching `wheelbase_max_nm` within a few degrees. | Soft lock will feel much more like a solid stop. Resistance will persist as long as the wheel is beyond the limit, even at zero velocity. |

## Proposed Changes
### 1. src/FFBEngine.h
- Maintain `BASE_NM_SOFT_LOCK` but consider it a legacy or internal constant if the formula changes to use `m_wheelbase_max_nm` directly. Actually, I will redefine it as a reference strength.

### 2. src/SteeringUtils.cpp
- Modify `FFBEngine::calculate_soft_lock`:
  - Calculate `excess_for_max` based on `m_soft_lock_stiffness`.
    - e.g., `excess_for_max = 0.1 / (std::max(1.0f, m_soft_lock_stiffness))`.
    - This means at default stiffness (20), full force is reached at 0.5% excess.
    - At max stiffness (100), full force is reached at 0.1% excess (~0.3 degrees on a typical rack).
  - `spring_nm = std::min(1.0, excess / excess_for_max) * m_wheelbase_max_nm * 1.5;`
    - Over-driving by 1.5x ensures that even if other effects are slightly opposing, we hit the 100% clamp.
  - Scale damping relative to `m_wheelbase_max_nm` and `m_soft_lock_damping`.
    - `damping_nm = m_steering_velocity_smoothed * m_soft_lock_damping * (double)m_wheelbase_max_nm * 0.5;`
    - Clamp damping to avoid excessive counter-force on return: `std::clamp(damping_nm, -max, max)`.

### 3. Version Increment
- Increment version in `VERSION` and `src/Version.h.in` (if needed, but usually `VERSION` is enough as CMake handles `Version.h`) to `0.7.125`.

## Test Plan (TDD-Ready)
### Design Rationale:
- Tests must prove that for very small steering excess (e.g., 0.2%), the soft lock force is high enough to be significant on the configured wheelbase.
- Tests must prove that the force does NOT disappear when steering velocity is zero.

### New/Updated Tests:
- **test_soft_lock_wall_persistence**: Verify that at 0.5% excess and zero velocity, the force is at least 100% of `wheelbase_max_nm` (clamped to -1.0 output).
- **test_soft_lock_stiffness_scaling**: Verify that increasing stiffness reduces the excess needed to reach full force.
- **Update existing tests**: Align `test_ffb_soft_lock.cpp` with new scaling.

## Deliverables
- [x] Code changes in `src/SteeringUtils.cpp`.
- [x] Updated unit tests in `tests/test_ffb_soft_lock.cpp`, `tests/test_issue_184_repro.cpp`, and `tests/test_issue_181_soft_lock_weakness.cpp`.
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`.
- [x] Implementation Notes in this plan.

## Implementation Notes
### Unforeseen Issues
- The initial formula in the design rationale was slightly modified during implementation to ensure even higher saturation at very small excesses (using 2.0x wheelbase max torque as the target for the spring component).
- Multiple existing tests (`test_issue_184_repro.cpp`, `test_issue_206_vibration_scaling.cpp`) needed adjustments because the new soft lock is significantly stronger than the legacy version, which tripped up some precise assertions.

### Plan Deviations
- Added `SetSteeringVelocitySmoothed` to `FFBEngineTestAccess` to allow explicit testing of the zero-velocity persistence requirement.
- Cleaned up root directory pollution (test logs) that were accidentally generated during the build/test loop.

### Challenges Encountered
- Balancing damping and spring force: At high stiffness, the spring ramp is extremely steep. Without proper clamping of damping, the return-to-center feel could have been too abrupt or oscillated. The 50% max torque clamp on damping provides a good safety margin.

### Recommendations for Future Plans
- Always check transitive dependencies or explicit includes for `std::clamp` (`<algorithm>`) even if it builds, to ensure standard compliance across compilers.
- Ensure that "force NM" targets in physics calculations always consider the hardware scaling group they belong to (Structural vs Texture).
