# Implementation Plan - Issue #511: Gate Gyroscopic Damping

This plan addresses GitHub issue #511 by implementing a "Smart Damper" approach for gyroscopic damping in the `FFBEngine` class. It includes a Lateral G gate, a steering velocity deadzone, and a force cap.

## Context
The existing gyroscopic damping in LMUFFB is a "dumb" damper that resists all steering movement proportional to speed. This can lead to sluggishness in corners and masking of fine road textures. High-end wheelbases (like Simucube) use gated damping to solve this.

### Design Rationale
Implementing these three features will maintain the safety benefits of damping (wobble protection on straights) while eliminating the intrusive "muddy" feel in corners and allowing road textures to shine through.

## Analysis
The implementation requires:
1.  Adding new configuration parameters to the configuration model and serialization system.
2.  Modifying the `FFBEngine::calculate_gyro_damping` method to apply the gating and capping math.
3.  Ensuring existing tests are updated to account for the new behavior.

### Design Rationale
The specific thresholds (0.1G to 0.4G for the gate, 0.5 rad/s for the deadzone, and 2.0 Nm for the cap) are based on user-provided "sweet spots" for race cars like those in Le Mans Ultimate.

## Proposed Changes

### 1. Configuration Model Updates
- **File:** `src/ffb/FFBConfig.h`
- **Action:** Add the following fields to `struct AdvancedConfig` with default values:
  - `float gyro_lat_g_gate_lower = 0.1f;`
  - `float gyro_lat_g_gate_upper = 0.4f;`
  - `float gyro_max_nm = 2.0f;`
  - `float gyro_vel_deadzone = 0.5f;`
- **Action:** Update `AdvancedConfig::Equals()` and `AdvancedConfig::Validate()`.

### 2. Preset API Updates
- **File:** `src/core/Config.h`
- **Action:** Add a fluent setter `Preset& SetGyroGating(float lower, float upper, float max_nm, float deadzone)` to `struct Preset`.

### 3. Serialization and Synchronization
- **File:** `src/core/Config.cpp`
- **Action:** Update `PresetToToml`, `TomlToPreset`, `Config::ParsePhysicsLine`, and `Config::SyncPhysicsLine`.

### 4. Smart Damping Engine Logic
- **File:** `src/ffb/FFBEngine.cpp`
- **Action:** Modify `FFBEngine::calculate_gyro_damping()`:
  - Calculate `lat_g` and apply `smoothstep` gate (0.1G to 0.4G).
  - Apply soft velocity deadzone (0.5 rad/s) using `std::copysign`.
  - Clamp final damping force to `±gyro_max_nm` (2.0 Nm default).

## Test Plan

### 1. Regression Testing
- **File:** `tests/repro_issue_511.cpp`
- **Action:** Implement tests for Lateral G Gate, Velocity Deadzone, and Force Cap using a smooth continuous input model and `PumpEngineTime`.

### 2. Legacy Test Maintenance
- **Files:** `tests/test_ffb_yaw_gyro.cpp`, `tests/test_ffb_common.cpp`, `tests/test_refactor_advanced.cpp`.
- **Action:** Update legacy tests to use neutral gating values (disabling the gate/cap) or adjusting scenario data (e.g., setting LatG to 0.0) to preserve base math validation.

### 3. Build & Run
- Ensure clean build on Linux and all 633+ tests pass.

### Design Rationale
Targeted regression tests ensure the physics implementation exactly matches the requirement, while updating legacy tests preserves the integrity of the overall system validation.

## Implementation Notes

### Encountered Issues
- **100Hz/400Hz Interaction:** Injecting step-changes in steering angle caused massive velocity spikes in the 400Hz engine, making deadzone testing difficult. Fixed by using a continuous steering increment loop.
- **Upsampler Latency:** Lateral G signals need time to propagate through Holt-Winters and LPF filters. Resolved by using `PumpEngineTime(engine, data, 0.5)` in tests.
- **`std::clamp` UB:** Identified a potential Undefined Behavior in validation where `lo > hi` could occur if thresholds were at the limit. Fixed by tightening the lower bound clamp to 9.9f.

### Deviations from the Plan
- None.

### Suggestions for the Future
- Consider making the "Stationary Damping" also use the force cap to provide a consistent feel across all damping modes.
- The `smoothstep` gate could be exposed as a more generic utility if other effects (like road texture) want to fade out during cornering.
