# Implementation Plan: Up-sample FFB Part 1 (100Hz -> 400Hz Telemetry)

## Context
The LMU physics engine outputs telemetry data (like suspension deflection, lateral Gs, and wheel slip) at 100Hz. However, the `lmuFFB` engine processes force feedback at 400Hz to provide high-fidelity output to the wheelbase. Currently, this mismatch causes the telemetry data to "stair-step" (remain constant for 3 frames, then jump on the 4th). This creates high-frequency noise and ruins derivative-based calculations like Road Texture (suspension velocity) and Slope Detection. 

This plan implements **Part 1** of the up-sampling initiative: introducing a zero-latency Holt-Winters Double Exponential Smoothing filter to extrapolate the 100Hz telemetry into a smooth, continuous 400Hz signal before it reaches the FFB physics calculations.

## Design Rationale
*   **Why Holt-Winters?** Standard linear interpolation requires delaying the signal by one frame (10ms latency), which is unacceptable for FFB. Holt-Winters tracks both the *level* (current value) and the *trend* (rate of change), allowing us to accurately *extrapolate* the signal into the future with zero added latency.
*   **Why a Global Setting?** Telemetry up-sampling is a core system fidelity improvement, not a car-specific setup preference. Therefore, it will be implemented as a global toggle in `Config` rather than a per-preset variable.
*   **The `dt` Integrator Bug:** Currently, `FFBEngine` uses the game's `mDeltaTime` (~0.01s) for its 400Hz loop, causing time-based integrators to run 4x faster than intended. When up-sampling is enabled, we will pass the true loop `dt` (0.0025s) to fix this. To prevent breaking backward compatibility for users with up-sampling disabled, legacy mode will retain the 0.01s `dt`.
*   **Road Texture Normalization:** Road texture relies on positional deltas per frame. If the frame rate increases 4x, the delta per frame shrinks by 4x. We will introduce a `dt_scale` normalization factor to ensure the physical amplitude of the road texture remains identical whether up-sampling is on or off.

## Reference Documents
*   [Upsampling Research Report](https://github.com/coasting-nc/LMUFFB/blob/main/docs/dev_docs/reports/upsampling.md)

## Codebase Analysis Summary
*   **`src/main.cpp` (`FFBThread`)**: Currently detects new telemetry frames by checking if `mElapsedTime` has changed. It passes the raw `TelemInfoV01` directly to `FFBEngine::calculate_force`.
*   **`src/FFBEngine.cpp`**: Consumes `data->mDeltaTime` as `ctx.dt`. Calculates derivatives for road texture, bottoming, and slope detection.
*   **`src/Config.h / .cpp`**: Manages global and preset settings.
*   **Impact Zone**: The boundary between `main.cpp` receiving the shared memory and `FFBEngine` processing it. We will intercept the struct here.

## FFB Effect Impact Analysis

| Effect | Technical Change | User Perspective |
| :--- | :--- | :--- |
| **Road Texture** | `delta_l` will become continuous instead of spiking every 4th frame. Added `dt_scale` to maintain amplitude. | Will feel significantly smoother and more realistic, losing the "buzzy/robotic" 100Hz artifacting. |
| **Slope Detection** | `dG_dt` and `dAlpha_dt` will receive smooth 400Hz data instead of stair-stepped data. | Slope detection will become much more stable, reducing false positives and allowing for tighter tuning. |
| **SoP / Yaw Kick** | Base signals (`mLocalAccel.x`, `mLocalRotAccel.y`) will be continuous. | Smoother onset of lateral forces; less high-frequency grain in the wheel during cornering. |
| **Suspension Bottoming** | Method B (Force derivative) will calculate true continuous velocity. | More reliable bottoming detection; fewer missed or falsely triggered thumps. |

## Proposed Changes

### 1. Create `src/TelemetryUpsampler.h` and `src/TelemetryUpsampler.cpp`
*   **`HoltWintersFilter` class**:
    *   State: `m_level`, `m_trend`, `m_alpha`, `m_beta`, `m_initialized`.
    *   `void Update(double value, double dt)`: Updates level and trend.
    *   `double Extrapolate(double dt_since_update) const`: Returns `level + trend * dt_since_update`.
*   **`TelemetryUpsampler` class**:
    *   Contains `HoltWintersFilter` instances for: `mLocalAccel` (x,y,z), `mLocalRot` (x,y,z), `mLocalRotAccel` (x,y,z), `mLocalVel` (x,y,z), `mUnfilteredSteering`, `mUnfilteredThrottle`, `mUnfilteredBrake`, `mSteeringShaftTorque`, and the 12 critical float fields inside `mWheel[4]`.
    *   `void Update(const TelemInfoV01& raw_telem, double telem_dt)`: Feeds new 100Hz data to all filters.
    *   `TelemInfoV01 Extrapolate(const TelemInfoV01& base_telem, double dt_since_update, double sub_dt)`: Returns a copy of the telemetry with extrapolated values injected, and `mDeltaTime` set to `sub_dt`.

### 2. Update `src/Config.h` and `src/Config.cpp`
*   Add global static variable: `static bool m_telemetry_upsampling_enabled;` (Default: `true`).
*   **Parameter Synchronization Checklist**:
    *   [x] Declaration in `Config.h` (Global static, NOT in `Preset`).
    *   [x] Initialization in `Config.cpp`.
    *   [x] Entry in `Config::Save()` (under `[System & Window]`).
    *[x] Entry in `Config::Load()`.
    *   *Note: Because it is a global setting, it does not require entries in `Preset::Apply` or `Preset::UpdateFromEngine`.*

### 3. Update `src/GuiLayer_Common.cpp`
*   Add a checkbox for "Enable Telemetry Upsampling (100Hz -> 400Hz)" under the "Advanced Settings" -> "System Health" or a new "Engine Fidelity" section.

### 4. Update `src/FFBEngine.h` and `src/FFBEngine.cpp`
*   Add `TelemetryUpsampler m_upsampler;` to `FFBEngine` class.
*   In `calculate_road_texture`, apply frame-rate independent scaling:
    ```cpp
    double dt_scale = 0.01 / (ctx.dt + 1e-9);
    delta_l *= dt_scale;
    delta_r *= dt_scale;
    ```

### 5. Update `src/main.cpp` (`FFBThread`)
*   Track `time_since_last_telem`.
*   When `pPlayerTelemetry->mElapsedTime != last_telem_time`:
    *   Calculate `telem_dt` (usually ~0.01).
    *   Call `g_engine.m_upsampler.Update(*pPlayerTelemetry, telem_dt)`.
    *   Reset `time_since_last_telem = 0.0`.
*   Every tick, increment `time_since_last_telem += actual_dt`.
*   If `Config::m_telemetry_upsampling_enabled` is true:
    *   Clamp extrapolation time: `double extrap_time = std::min(time_since_last_telem, 0.020);` (Prevents wild extrapolation if game stutters).
    *   `working_telem = g_engine.m_upsampler.Extrapolate(*pPlayerTelemetry, extrap_time, actual_dt);`
*   Pass `&working_telem` to `calculate_force`.

### 6. Version Increment
*   Increment `VERSION` file and `src/Version.h` to the next patch version (e.g., `0.7.108`).

## Test Plan (TDD-Ready)

### 1. `test_holt_winters.cpp`
*   **Design Rationale**: Proves the core mathematical filter correctly tracks levels and trends without introducing instability.
*   **`test_hw_constant`**: Feed a constant value (e.g., 5.0) for 10 frames. Assert `Extrapolate(0.005)` returns exactly 5.0 and trend is 0.0.
*   **`test_hw_linear_ramp`**: Feed values increasing by 1.0 every 0.01s. Assert `Extrapolate(0.005)` returns exactly the midpoint (e.g., if last was 10.0, returns 10.5).
*   **`test_hw_reset`**: Verify calling `Reset()` clears the initialization state.

### 2. `test_telemetry_upsampler.cpp`
*   **Design Rationale**: Ensures the wrapper class correctly routes all required fields through their respective filters.
*   **`test_upsampler_basic`**: 
    *   Create two `TelemInfoV01` frames with known differences in `mLocalAccel.x` and `mWheel[0].mSuspensionDeflection`.
    *   Call `Update()` with frame 1, then frame 2.
    *   Call `Extrapolate()` with `dt_since_update = 0.0025, 0.0050, 0.0075`.
    *   Assert the returned struct contains correctly interpolated intermediate values.

### 3. `test_ffb_upsampling_integration.cpp`
*   **Design Rationale**: Proves that `FFBEngine` correctly handles the modified `ctx.dt` and that the `dt_scale` fix prevents road texture from exploding.
*   **`test_road_texture_scaling`**:
    *   Initialize engine with Road Texture enabled.
    *   Simulate a 100Hz step (upsampling OFF): `ctx.dt = 0.01`, `delta_l = 0.001`. Record output force.
    *   Simulate a 400Hz step (upsampling ON): `ctx.dt = 0.0025`, `delta_l = 0.00025` (1/4th the delta). Record output force.
    *   Assert that the output forces are nearly identical, proving the `dt_scale` normalization works.

## Deliverables
- [ ] Create `src/TelemetryUpsampler.h` and `src/TelemetryUpsampler.cpp`.
- [ ] Update `src/Config.h`, `src/Config.cpp`, and `src/GuiLayer_Common.cpp`.
- [ ] Update `src/FFBEngine.h` and `src/FFBEngine.cpp` (Road texture scaling).
- [ ] Update `src/main.cpp` (Integration and timing logic).
- [ ] Create `tests/test_holt_winters.cpp`.
- [ ] Create `tests/test_telemetry_upsampler.cpp`.
- [ ] Create `tests/test_ffb_upsampling_integration.cpp`.
- [ ] Update `tests/CMakeLists.txt` to include new test files.
- [ ] Increment version in `VERSION` and `src/Version.h`.
- [ ] Update this plan document with "Implementation Notes" (Unforeseen Issues, Plan Deviations, Challenges, Recommendations) upon completion.