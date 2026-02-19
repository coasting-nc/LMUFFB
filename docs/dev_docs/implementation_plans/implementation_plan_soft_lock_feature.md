# Implementation Plan - Issue #117: Soft Lock Feature

The "Soft Lock" feature provides resistance when the steering wheel reaches the car's maximum steering range. This prevents the user from turning the wheel past the car's intended limits, improving realism and preventing mechanical jolts.

## Proposed Changes

### 1. `src/FFBEngine.h`
- Add state variables for Soft Lock:
    - `bool m_soft_lock_enabled`
    - `float m_soft_lock_stiffness` (Nm per unit of excess steering)
    - `float m_soft_lock_damping` (Nm per rad/s)
- Update `FFBCalculationContext` to include `soft_lock_force`.
- Update `FFBSnapshot` to include `ffb_soft_lock` for logging and telemetry analysis.
- Declare `calculate_soft_lock` helper method.

### 2. `src/FFBEngine.cpp`
- Initialize Soft Lock parameters in constructor (default: disabled, stiffness=20.0, damping=0.5).
- Implement `calculate_soft_lock(const TelemInfoV01* data, FFBCalculationContext& ctx)`:
    - Use `data->mUnfilteredSteering` (normalized -1.0 to 1.0).
    - If `abs(mUnfilteredSteering) > 1.0`:
        - `excess = abs(mUnfilteredSteering) - 1.0`
        - `spring = excess * m_soft_lock_stiffness * sign(mUnfilteredSteering)`
        - `damping = m_steering_velocity_smoothed * m_soft_lock_damping`
        - `ctx.soft_lock_force = -(spring + damping)`
    - Ensure the force is properly scaled by `ctx.decoupling_scale` if necessary, though soft lock should probably be an absolute safety force.
- Call `calculate_soft_lock` inside `calculate_force`.
- Add `ctx.soft_lock_force` to the total FFB summation.
- Populate `FFBSnapshot::ffb_soft_lock`.

### 3. `src/Config.h` & `src/Config.cpp`
- Add Soft Lock settings to `Preset` struct.
- Update `Load`, `Save`, and `ApplyPreset` logic to persist these settings.
- Add validation to ensure stiffness and damping are non-negative.

### 4. `src/GuiLayer_Common.cpp` (Optional but recommended)
- Add a new "Soft Lock" section in the "Advanced" or "Safety" tab of the GUI.
- Include a toggle for enabled state and sliders for Stiffness and Damping.

## Verification Strategy

### 1. Unit Testing
- Created `tests/test_ffb_soft_lock.cpp`.
- Simulated `mUnfilteredSteering` at:
    - `0.5` (Result: 0.0 force)
    - `1.0` (Result: 0.0 force)
    - `1.1` (Result: -1.0 force @ max stiffness)
    - `-1.1` (Result: 1.0 force @ max stiffness)
- Verified damping opposes steering velocity at the limit.

### 2. Static Analysis
- Ensured no division by zero in the force calculation.
- Ensured `mUnfilteredSteering` is checked for `isfinite`.
- Verified `soft_lock_force` is included in the structural summation and thus subject to master gain and output scaling.

### 3. Build & Test
- Successfully built on Linux.
- All 263 tests passed.

## Implementation Notes

- **Math Correction:** The initial implementation used a lower `BASE_NM_SOFT_LOCK` (10.0), which was deemed too weak during code review. It was increased to 50.0 to ensure a strong physical stop at the limit, especially when combined with high-torque direct drive wheels.
- **Thread Safety:** Migrated the global `g_engine_mutex` from `std::mutex` to `std::recursive_mutex`. This was necessary because `Config::Save` and `Config::Load` now include their own locking logic for reliability, but they are often called from the GUI thread which already holds the lock. Recursive locking prevents deadlocks in these scenarios.
- **Damping Direction:** Verified that damping correctly opposes movement in both directions (moving away from or returning to the limit), helping to prevent oscillations at the physical hard stop.
- **Scaling:** Used `ctx.decoupling_scale` (which is `max_torque_ref / 20.0`) to ensure that the soft lock Nm intensity scales correctly with the user's torque reference settings.
