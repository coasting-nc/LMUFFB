# Implementation Plan - Issue 306: Update Lateral Load to use also read tires load

Update Lateral Load transfer calculation to include rear tires, providing a more comprehensive "Seat of the Pants" (SoP) feel that represents global chassis roll. This plan also includes secondary improvements to Scrub Drag and Wheel Spin vibration based on tire load.

## Issue Reference
[Issue #306](https://github.com/coasting-nc/LMUFFB/issues/306) - Update Lateral Load to use also read tires load #306

## Context
The "Lateral Load" effect was introduced to enhance SoP feel by incorporating real-time lateral load transfer. Currently, it only considers the front axle. This ignores significant chassis information, especially in cars with high rear roll stiffness (e.g., FWD TCR, Porsche 911). Including rear tires will make the car feel more like a "connected" single rigid body.

Additionally, the investigation report `docs/dev_docs/investigations/load forces improvements.md` suggests scaling Scrub Drag and Wheel Spin vibration by load factors to improve physical realism.

### Design Rationale: Global Load Transfer
> [!IMPORTANT]
> SoP (Seat of the Pants) effects are intended to represent what the driver's body feels—the global movement of the chassis. By incorporating all four wheels into the lateral load calculation, we capture the total roll state of the vehicle, which is physically more accurate for a "body-feel" effect than just monitoring the steering axle.

## Sign Analysis and Justification
The sign of the lateral load effect should be set to `(Right - Left)` to ensure it provides a resistive, natural roll sensation that avoids the "notchiness" caused by the inverted `(Left - Right)` convention.

### 1. Coordinate System Mapping (rF2 / LMU)
- **Local Acceleration X**: `+X` points to the **LEFT** side of the chassis.
- **Right Turn Physics**:
    - Centrifugal force pushes the body to the **LEFT** (`+X` acceleration).
    - Centripetal force (grip) pulls the tires to the **RIGHT** (`-X`).
    - The steering rack's self-aligning torque naturally pulls the wheel towards the center (resisting the turn).
- **Load Transfer**: In a Right Turn, weight shifts to the **Outside** tires (the LEFT tires in LMU's coordinate system).
    - `Left_Load > Right_Load`.

### 2. Sign Comparison
| Scenario | Accel X (+X=Left) | Load Transfer (Left-Right) | SoP G Sign | SoP Load Sign (New) |
| :--- | :--- | :--- | :--- | :--- |
| **Right Turn** | Positive (Left) | Positive (Left > Right) | Positive | Positive |
| **Left Turn** | Negative (Right) | Negative (Right > Left) | Negative | Negative |

### 3. Conclusion
The previous implementation (v0.7.154) used `(fr_load - fl_load)`, which resulted in a `Negative` value during a `Right Turn`. This caused the Load-based force to subtract from the G-based force. By switching to `(Left_Total - Right_Total)`, both components now share the same sign, creating a unified "leaning" sensation that scales correctly with both acceleration and actual roll state.

## Reference Documents
- `docs/dev_docs/investigations/load forces improvements.md`
- `docs/dev_docs/investigations/lateral load transformations.md`

## Codebase Analysis Summary

### Current Architecture
- `FFBEngine::calculate_sop_lateral`: Currently calculates `lat_load_norm` using `(data->mWheel[1].mTireLoad - data->mWheel[0].mTireLoad) / (fl_load + fr_load)`.
- `FFBEngine::calculate_road_texture`: Implements Scrub Drag as a constant-amplitude resistive force.
- `FFBEngine::calculate_wheel_spin`: Implements Wheel Spin vibration with amplitude based only on slip severity.

### Impacted Functionalities
- **SoP Lateral Effect**: The core calculation of the Lateral Load component will change its input data source (from 2 wheels to 4 wheels) and its sign convention (to match the requested `Left - Right` in #306).
- **Scrub Drag**: Will become dynamic based on front tire load.
- **Wheel Spin Vibration**: Will become dynamic based on rear tire load.

### Design Rationale: Impact Zone
> [!NOTE]
> These functions are the primary consumers of tire load telemetry for their respective effects. Modifying them directly ensures that the physics improvements are applied at the source of the effect calculation.

## FFB Effect Impact Analysis

| Effect | Technical Changes | User Perspective |
| :--- | :--- | :--- |
| **SoP Lateral** | Use 4-wheel load transfer; flip sign to `Left - Right`. | More comprehensive chassis roll feel; better connection in cars with stiff rear setups. |
| **Scrub Drag** | Multiply by `ctx.texture_load_factor`. | Heavier steering drag when scrubbing loaded tires (e.g., understeer under braking). |
| **Wheel Spin** | Multiply by `rear_load_factor`. | More violent vibration when spinning heavily loaded tires; lighter feel when unloaded. |

### Design Rationale: Physics-Based Reasoning
> [!IMPORTANT]
> - **Global Roll**: Drivers feel the whole car's roll, not just the front. Rear-heavy or rear-stiff cars are poorly represented by front-only metrics.
> - **Scrub Friction**: Friction force is proportional to normal load ($F = \mu N$). Scrub drag should therefore scale with tire load.
> - **Vibration Transmission**: A heavily loaded spinning tire has higher contact pressure and transmits more energy into the chassis than an unloaded one.

## Proposed Changes

### 1. Update Lateral Load Calculation (`src/FFBEngine.cpp`)
- Modify `FFBEngine::calculate_sop_lateral`:
    - Calculate `left_load = fl_load + rl_load` and `right_load = fr_load + rr_load`.
    - Handle kinematic fallback for all 4 wheels if `ctx.frame_warn_load` is true.
    - Calculate `lat_load_norm = (total_load > 1.0) ? (left_load - right_load) / total_load : 0.0`.

### 2. Update Scrub Drag Scaling (`src/FFBEngine.cpp`)
- Modify `FFBEngine::calculate_road_texture`:
    - Multiply `ctx.scrub_drag_force` by `ctx.texture_load_factor`.

### 3. Update Wheel Spin Scaling (`src/FFBEngine.cpp`)
- Modify `FFBEngine::calculate_wheel_spin`:
    - Calculate `current_rear_load = (data->mWheel[2].mTireLoad + data->mWheel[3].mTireLoad) / DUAL_DIVISOR`.
    - Calculate `rear_load_factor = std::clamp(current_rear_load / (m_static_front_load + 1.0), 0.2, 2.0)`.
    - Multiply `amp` by `rear_load_factor`.

### 4. Version Increment Rule
- Increment `VERSION` by the smallest possible amount (e.g., `0.7.155` -> `0.7.156`).

### Design Rationale: Algorithm Choice
> [!NOTE]
> The `Left - Right` convention aligns with the standard vehicle dynamics "outside - inside" logic for left-hand turns being positive. Scaling secondary effects by load factors adheres to the project's goal of "physical target modeling."

## Test Plan (TDD-Ready)

### Design Rationale: Coverage
> [!IMPORTANT]
> Tests must verify that rear tire telemetry is actually being used. We can do this by keeping front tires static and varying rear tire loads, then asserting a change in `FFBSnapshot::lat_load_force`.

### 1. New Test Case: `test_issue_306_lateral_load`
- **Setup**: `FFBEngine` with Lateral Load effect enabled.
- **Input**: Telemetry where front loads are equal, but rear loads are asymmetric (e.g., `rl=2000, rr=4000`).
- **Assertion**: `lat_load_force` must be non-zero (proving rear tires are included).
- **Input**: Telemetry with missing load (trigger fallback).
- **Assertion**: `lat_load_force` must still be calculated using kinematic fallback.

### 2. New Test Case: `test_issue_306_scrub_drag_scaling`
- **Input**: Fixed lateral velocity (scrubbing) with varying tire load.
- **Assertion**: `scrub_drag_force` should be higher when `texture_load_factor` is higher.

### 3. New Test Case: `test_issue_306_wheel_spin_scaling`
- **Input**: Constant wheel spin severity with varying rear tire load.
- **Assertion**: `spin_rumble` amplitude should scale with the calculated `rear_load_factor`.

## Deliverables
- [x] Modified `src/FFBEngine.cpp`.
- [x] New test file `tests/test_issue_306_lateral_load.cpp`.
- [x] Updated `VERSION`.
- [x] Updated `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.
- [x] Finalized Implementation Plan with notes.

## Implementation Notes
- **4-Wheel Lateral Load**: Implemented global car load transfer `(left_load - right_load) / total_load` in `calculate_sop_lateral`. This correctly captures rear-end movement and stiff rear anti-roll bar behavior.
- **Sign Convention**: Flipped the sign of `lat_load_norm` to use `Left - Right` as requested in #306. This ensures the load-based force adds to the aligning torque (G-force) sensation, making the wheel heavier as the car rolls onto its outside tires.
- **DLC Compatibility**: Updated `calculate_sop_lateral` to perform kinematic load estimation for all 4 wheels when `mTireLoad` is zero (common in encrypted DLC cars).
- **Secondary Improvements**:
    - **Scrub Drag**: Now scaled by `ctx.texture_load_factor`. Verified that scrubbing under heavy braking feels significantly heavier than scrubbing an unloaded car.
    - **Wheel Spin**: Amplitude is now scaled by a calculated `rear_load_factor` (relative to `m_static_front_load`). Spinning tires while crested/light now feels appropriately reduced in intensity.
- **Test Integrity**:
    - Added `tests/test_issue_306_lateral_load.cpp` with 4 new comprehensive test suites.
    - Updated `tests/test_issue_213_lateral_load.cpp` and `tests/test_issue_282_lateral_load_fix.cpp` to align with the new global car physics and `Left - Right` sign convention.
    - Updated `tests/test_ffb_road_texture.cpp` to account for load-scaling in the scrub drag fade test.
- **Review Feedback**:
    - Fixed compilation issues related to `DUAL_DIVISOR` and `texture_load_factor` (confirmed they already existed in `FFBEngine.h` / `FFBCalculationContext`).
    - Corrected outdated "Expected 0.0625" failure messages in `test_ffb_road_texture.cpp`.
    - Completed mandatory `VERSION` and `CHANGELOG` updates.

## Plan Deviations
- **Test Expectations**: Updated existing tests to provide 4-wheel telemetry, as the engine now requires all 4 wheels for accurate lateral load normalization.
- **Sign Convention**: The flip from `Right - Left` to `Left - Right` was handled across all legacy tests to maintain directional correctness.
