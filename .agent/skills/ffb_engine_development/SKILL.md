---
name: ffb_engine_development
description: Critical constraints and patterns for developing high-performance FFB logic
---

# FFB Engine Development

Use this skill when modifying `FFBEngine.h` or any physics calculation logic.

## Technical Constraints

### 1. The 400Hz Core Loop
- `FFBEngine::calculate_force` runs on a high-priority thread.
- **NO HEAP ALLOCATION**: `new`, `malloc`, `std::vector::push_back`, etc., are strictly prohibited inside the core loop.

### 2. Phase Accumulation
- For any oscillating effects (sine waves, vibrations), use **Phase Accumulation**.
- ❌ *Wrong*: `sin(time * frequency)` (Causes audio/tactile clicks when frequency changes).
- ✅ *Right*: 
  ```cpp
  m_phase += frequency * dt; 
  output = sin(m_phase);
  ```

### 3. Coordinate Systems (LMU vs DirectInput)
- **LMU/rFactor 2**: Left-handed system (+X = Left).
- **DirectInput**: +Force = Right.
- **Rule**: Lateral forces (SoP, Rear Torque, Scrub Drag) must be **INVERTED** (negated) to produce the correct DirectInput force (Left).
- **Stability**: Never use `abs()` on lateral velocity for direction-dependent logic (like counter-steering); it destroys the sign.

### 4. Continuous Physics State (Anti-Glitch)
- **Rule**: Low Pass Filters (LPF) and physics state variables (Slip Angle, RPM smoothing) must be updated **EVERY TICK**.
- **Reason**: Stopping updates when telemetry is "unhealthy" makes filter states stale. Resuming later causes massive force spikes.
- **Implementation**: Ensure filters are updated outside of conditional blocks.

### 5. Reliability Standards
- **Validate Inputs**: Check telemetry inputs for `NaN` or `Inf` using `std::isfinite()`.
- **Clamp Outputs**: Ensure final FFB force output is always in the range **[-1.0, 1.0]** before sending to drivers.
- **Thread Safety**: Access any shared state (like gains/settings) under `g_engine_mutex`.
- **Safe Gains**: Use `std::clamp()` to keep gain multipliers within safe predefined limits (e.g., [0.0, 10.0]).

## Planning for FFB Effects
Before modifying FFB logic, analyze:
1. **Affected Effects**: List all impacted physics behaviors (e.g., ABS, Understeer).
2. **User Impact**: How will the FFB "feel" change? Do preset values need tuning?
3. **Data Flow**: Trace how new telemetry fields or derived values reach the final force calculation.

## Implementation Pattern for New Effects
1. Add toggle and gain float to `FFBEngine`.
2. Add phase accumulator variable if needed.
3. Implement logic in `calculate_force`.
4. Add visualization data to `FFBSnapshot`.
5. Update `Config.h/cpp` for persistence.
6. New features should default to `false` or `0.0`.
