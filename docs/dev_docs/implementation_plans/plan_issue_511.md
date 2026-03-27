# Implementation Plan - Gate Gyroscopic Damping (Issue #511)

This plan implements three mitigations for Gyroscopic Damping to transform it into a "Smart Damper" as requested in GitHub issue #511.

## Context
Gyroscopic Damping is used to prevent hands-off oscillations (speed wobbles) on straights. However, it can feel sluggish during cornering, mute fine road details, and become too strong during rapid steering maneuvers.

## Design Rationale
The "Smart Damper" approach improves the trade-off between stability and feel:
1.  **Lateral G-Force Gate**: Disables damping during cornering. When the car is cornering, the tires' self-aligning torque provides natural stability, making artificial damping unnecessary and counter-productive (it fights the driver). We use `smoothstep` to ensure a gradual transition between 0.1G and 0.4G.
2.  **Velocity Deadzone**: Road textures cause tiny micro-movements. By ignoring velocities below a certain threshold (0.5 rad/s) and smoothly subtracting the deadzone from higher velocities, we allow these high-frequency details to pass through to the driver's hands without being resisted by the damping logic, avoiding a harsh "step" in resistance.
3.  **Force Capping**: Ensures the damper never overpowers the driver during emergency evasive maneuvers. A hard cap of 2.0 Nm is sufficient to catch a wobble without becoming a physical obstacle.

## Reference Documents
- Verbatim Issue Copy: `docs/dev_docs/github_issues/511_gate_gyroscopic_damping.md`

## Codebase Analysis Summary
- **FFBEngine.h**: Contains `FFBEngine` class definition and internal constants.
- **FFBEngine.cpp**: Implements `calculate_gyro_damping` where the mitigations will be applied.
- **FFBCalculationContext**: Defined in `GripLoadEstimation.h`, it holds the intermediate `gyro_force` value.
- **MathUtils.h**: Provides `smoothstep` and `GRAVITY_MS2` (via `FFBEngine` which uses it).

### Impacted Functionalities
- **Gyroscopic Damping**: The primary target of these changes.
- **Stationary Damping**: Shares the smoothed steering velocity but remains unaffected by the G-gate to preserve pit-box stability.

**Design Rationale**: These specific modules were identified because they directly handle the gyroscopic damping effect calculation and the telemetry inputs required for gating.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
| :--- | :--- | :--- |
| Gyro Damping | Gated by Lateral G, deadzoned by velocity, and capped at 2.0 Nm. | Feels "invisible" during cornering; road details are sharper on straights; steering feels less "heavy" during fast movements. |

**Design Rationale**: The physics-based reasoning is to align the artificial damping with the car's dynamic state. High lateral load provides its own damping via tire physics, so software damping should step aside.

## Proposed Changes

### 1. Add Constants to `src/ffb/FFBEngine.h`
Add the following constants to the private section of `FFBEngine` class:
```cpp
static constexpr double GYRO_G_GATE_MIN = 0.1;
static constexpr double GYRO_G_GATE_MAX = 0.4;
static constexpr double GYRO_MAX_NM = 2.0;
static constexpr double GYRO_VEL_DEADZONE = 0.5;
```

### 2. Update Gyro Damping Logic in `src/ffb/FFBEngine.cpp`
Modify `FFBEngine::calculate_gyro_damping`:
1. **Lateral G Gate**:
   - `double lat_g = std::abs(data->mLocalAccel.x / (double)GRAVITY_MS2);`
   - `double straight_line_gate = 1.0 - LMUFFB::Utils::smoothstep(GYRO_G_GATE_MIN, GYRO_G_GATE_MAX, lat_g);`
   - Apply to `driving_gyro`: `double driving_gyro = m_advanced.gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE) * straight_line_gate;`
2. **Velocity Deadzone**:
   - Apply to a local copy of `m_steering_velocity_smoothed`:
     ```cpp
     double steer_vel = m_steering_velocity_smoothed;
     if (std::abs(steer_vel) < GYRO_VEL_DEADZONE) {
         steer_vel = 0.0;
     } else {
         steer_vel -= std::copysign(GYRO_VEL_DEADZONE, steer_vel);
     }
     ```
3. **Calculation & Capping**:
   - `ctx.gyro_force = -1.0 * steer_vel * driving_gyro;`
   - `ctx.gyro_force = std::clamp(ctx.gyro_force, -GYRO_MAX_NM, GYRO_MAX_NM);`

**Design Rationale**: Applying the gate to the gain and the deadzone to the velocity ensures a smooth, predictable response. Clamping the final force protects the driver from excessive resistance.

## Parameter Synchronization Checklist
No new user-facing parameters are added. Existing `gyro_gain` and `gyro_smoothing` still function as intended but are now modulated by the new logic.

## Initialization Order Analysis
Changes are local to `FFBEngine` and don't affect global initialization.

## Version Increment Rule
Version will be incremented from 0.7.271 to 0.7.272 in `VERSION`.

## Test Plan (TDD-Ready)

### Unit Tests in `tests/test_ffb_yaw_gyro.cpp`
Add a new test case `test_smart_gyro_damping`:
1. **test_gyro_g_gate**: Verify damping force fades as Lateral G increases (0.1G to 0.4G).
2. **test_gyro_velocity_deadzone**: Verify small movements are ignored (< 0.5 rad/s).
3. **test_gyro_force_capping**: Verify 2.0 Nm limit.

**Design Rationale**: These tests provide sufficient coverage for the three new logical components of the "Smart Damper" and prove its safety and fidelity improvements.

## Deliverables
- [x] Modified `src/ffb/FFBEngine.h`
- [x] Modified `src/ffb/FFBEngine.cpp`
- [x] Updated `tests/test_ffb_yaw_gyro.cpp`
- [x] Updated `VERSION`
- [x] Updated `CHANGELOG_DEV.md`
- [x] Code Review iteration record(s)
- [x] Updated Implementation Notes (final step)

## Implementation Notes
### Encountered Issues
- Redefinition errors in Unity Build when using same helper function name (`MeasureGyroForce`) in multiple test files. Fixed by using unique names or anonymous namespaces.
- Existing tests (`test_gyro_damping`, `test_refactor_units`) failed due to the new velocity deadzone and force capping. Fixed by adjusting test inputs and expectations to be compatible with the Smart Damper logic.
- `test_refactor_advanced_consistency` expected a specific total FFB output value that included full gyroscopic damping. Since the test telemetry had a lateral acceleration of 4.0 m/s² (~0.41G), the new logic correctly zeroed out the damping component, leading to a shift in the expected total force. Updated the test expectation to reflect the more accurate physics.
- Initial implementation of `test_smart_gyro_damping` failed because it didn't allow enough time for upsamplers to settle. Fixed by using a pumped time helper.
- **Review Regression**: An earlier iteration accidentally deleted unrelated "Yaw Kick" tests while refactoring. These were fully restored in the final version to maintain feature parity.

### Deviations from the Plan
- Updated G-gate thresholds from initially planned 0.2-0.8 to the requested "sweet spot" of 0.1-0.4.
- Added a separate repro test file `tests/repro_issue_511.cpp` as requested.
- Restored original regression tests in `tests/test_ffb_yaw_gyro.cpp` instead of a destructive refactor, appending the new tests instead.
- Refined the implementation notes to explicitly mention the "Greenlight" status from iteration 2 of the code review.

### Suggestions for the Future
- Consider making the G-gate thresholds and velocity deadzone user-configurable in the "Advanced" section if high-end users request more fine-grained control.
- Monitor telemetry from high-performance cars (GT3/LMP) to verify if the 0.4G threshold is too aggressive for high-downforce cornering stability.
