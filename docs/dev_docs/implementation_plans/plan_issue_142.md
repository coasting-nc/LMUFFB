# Implementation Plan - Issue #142: Fix Direct Torque Strength and Passthrough

## Context
Issue #142 identifies that the 400Hz "Direct Torque" source is significantly weaker than the legacy "Shaft Torque" source. This is because the high-frequency signal from Le Mans Ultimate is already normalized to the [-1.0, 1.0] range, whereas the LMUFFB engine expects raw torque in Newton-meters (Nm) for its internal physics pipeline, which later divides by `m_max_torque_ref`. Additionally, users have requested a "TIC mode" (similar to VNM wheelbases) which allows for a full passthrough of the game's FFB without LMUFFB's understeer or dynamic weight modulation.

## Reference Documents
- GitHub Issue #142: https://github.com/coasting-nc/LMUFFB/issues/142
- LMU Shared Memory Documentation (Internal)

## Codebase Analysis Summary
- **Torque Selection:** `FFBEngine::calculate_force` chooses between `data->mSteeringShaftTorque` (Nm) and `genFFBTorque` (Normalized float).
- **Normalization Pipeline:** The engine sums multiple components (Base, SoP, Textures) and then performs `norm_force = total_sum / max_torque_safe`, where `max_torque_safe` is `m_max_torque_ref`.
- **Understeer/Weight Modulation:** `output_force` is currently calculated as `(base_input * m_steering_shaft_gain) * dynamic_weight_factor * ctx.grip_factor`. This modulation is what "Full Passthrough" should be able to bypass.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective (FFB Feel) |
| :--- | :--- | :--- |
| **Direct Torque (400Hz)** | Input `genFFBTorque` is scaled by `m_max_torque_ref`. | Correct, full-strength FFB when using the 400Hz source. |
| **Understeer Drop** | Can be bypassed if `m_torque_passthrough` is enabled. | User can choose to feel the game's native understeer instead of LMUFFB's calculation. |
| **Dynamic Weight** | Can be bypassed if `m_torque_passthrough` is enabled. | Prevents LMUFFB's weight transfer simulation from affecting the base game signal. |
| **Secondary Effects** | No change to SoP, Yaw Kick, or Textures. | These are still added on top of the base signal, providing the "Mix" requested in TIC mode. |

## Proposed Changes

### 1. FFB Engine Core (`src/FFBEngine.h` & `src/FFBEngine.cpp`)
- Add `bool m_torque_passthrough = false;` to `FFBEngine` public members.
- Update `calculate_force` to scale `genFFBTorque`:
  ```cpp
  double raw_torque_input = (m_torque_source == 1) ? (double)genFFBTorque * (double)m_max_torque_ref : data->mSteeringShaftTorque;
  ```
- Implement passthrough logic in `calculate_force`:
  ```cpp
  double dw_factor = m_torque_passthrough ? 1.0 : dynamic_weight_factor;
  double g_factor = m_torque_passthrough ? 1.0 : ctx.grip_factor;
  double output_force = (base_input * (double)m_steering_shaft_gain) * dw_factor * g_factor;
  ```

### 2. Configuration System (`src/Config.h` & `src/Config.cpp`)
- **Parameter Synchronization Checklist:**
  - [ ] Declaration in `FFBEngine.h`: `bool m_torque_passthrough;`
  - [ ] Declaration in `Preset` struct: `bool torque_passthrough = false;`
  - [ ] Entry in `Preset::Apply()`
  - [ ] Entry in `Preset::UpdateFromEngine()`
  - [ ] Entry in `Config::Save()`
  - [ ] Entry in `Config::Load()`
  - [ ] Entry in `Preset::Equals()`

### 3. User Interface (`src/GuiLayer_Common.cpp`)
- Add a checkbox "Pure Passthrough" next to or below the "Torque Source" combo box.
- Tooltip: "Bypasses LMUFFB's internal Understeer and Dynamic Weight modulation for the base steering torque. Recommended when using Direct Torque (400Hz)."

### 4. Versioning
- Increment `VERSION` to `0.7.63`.
- Update `src/Version.h`.
- Update `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.

## Test Plan (TDD-Ready)
### New Tests in `tests/test_ffb_issue_142.cpp`
1. **TestDirectTorqueScaling:**
   - Setup: `m_torque_source = 1`, `genFFBTorque = 1.0f`, `m_max_torque_ref = 50.0f`, `m_gain = 1.0f`.
   - Action: Call `calculate_force`.
   - Assert: `total_output` should be approximately `1.0` (scaled correctly).
2. **TestTorquePassthroughEnabled:**
   - Setup: `m_torque_passthrough = true`, `m_understeer_effect = 1.0f`, `front_grip = 0.5f`.
   - Action: Call `calculate_force`.
   - Assert: `understeer_drop` snapshot field should be `0.0`.
3. **TestTorquePassthroughDisabled:**
   - Setup: `m_torque_passthrough = false`, `m_understeer_effect = 1.0f`, `front_grip = 0.5f`.
   - Action: Call `calculate_force`.
   - Assert: `understeer_drop` snapshot field should be > `0.0`.

## Deliverables
- [x] Modified `src/FFBEngine.h` & `src/FFBEngine.cpp`
- [x] Modified `src/Config.h` & `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] New Test File `tests/test_ffb_issue_142.cpp`
- [x] Updated `VERSION`, `Version.h`, and changelogs.
- [x] Implementation Notes (updated after development).

## Implementation Notes
### Unforeseen Issues
- Existing diagnostic test `test_ffb_diag_updates.cpp` failed because it expected `steer_force` in snapshots to match `genFFBTorque` exactly. Since `steer_force` now correctly reflects the scaled torque used for physics, the test was updated to set `m_max_torque_ref = 1.0` for predictable output.
- `AsyncLogger` and `SessionInfo` needed updates to include the new `torque_passthrough` setting to ensure telemetry logs accurately reflect the engine state.

### Plan Deviations
- Added `ASSERT_LT` macro to `tests/test_ffb_common.h` to support more descriptive assertions in the new unit tests.
- Extended the update to `AsyncLogger` and `main.cpp` to include the new setting in session metadata, which wasn't explicitly in the initial proposed changes but is required for completeness.

### Challenges
- Ensuring the `understeer_drop` snapshot field correctly reflects the *applied* effect when passthrough is enabled (setting it to 0). This required a minor logic adjustment in the snapshot capture block.

### Recommendations
- Users should be encouraged to use "Direct Torque" (400Hz) with "Pure Passthrough" enabled if they prefer the game's original steering geometry and weight, but still want LMUFFB's high-frequency textures.
