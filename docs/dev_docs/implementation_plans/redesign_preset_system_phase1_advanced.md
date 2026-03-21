# Implementation Plan - Redesign Preset System Phase 1 (AdvancedConfig)

This document outlines the plan for refactoring the advanced physics and hardware-related configuration parameters into a grouped `AdvancedConfig` structure.

## Proposed Changes

### 1. Data Structures
- Define `AdvancedConfig` struct in `src/ffb/FFBConfig.h`.
- Include members:
    - `gyro_gain`
    - `gyro_smoothing`
    - `stationary_damping`
    - `soft_lock_enabled`
    - `soft_lock_stiffness`
    - `soft_lock_damping`
    - `speed_gate_lower`
    - `speed_gate_upper`
    - `rest_api_enabled`
    - `rest_api_port`
    - `road_fallback_scale`
    - `understeer_affects_sop`
- Implement `Equals()` and `Validate()` methods.

### 2. `Preset` Struct (src/core/Config.h)
- Replace loose advanced variables with `AdvancedConfig advanced;`.
- Update `Apply()`, `UpdateFromEngine()`, `Equals()`, and `Validate()`.
- Update fluent setters (e.g., `SetSoftLock`, `SetGyro`, `SetSpeedGate`, etc.).

### 3. `FFBEngine` Class (src/ffb/FFBEngine.h)
- Replace loose advanced variables with `AdvancedConfig m_advanced;`.

### 4. Configuration I/O (src/core/Config.cpp)
- Update `ParsePhysicsLine`, `ParseVibrationLine`, `SyncSystemLine`, and `SyncPhysicsLine` to use the new struct paths.
- Update `WritePresetFields` to use the new struct paths.
- Update `LoadPresets` hardcoded presets.

### 5. Physics Engine & Utilities
- Update `src/ffb/FFBEngine.cpp` to use `m_advanced.<var>`.
- Update `src/physics/SteeringUtils.cpp` to use `m_advanced.<var>`.

### 6. GUI (src/gui/GuiLayer_Common.cpp)
- Update ImGui sliders and checkboxes to point to the new struct paths.

### 7. Tests
- Create `tests/test_refactor_advanced.cpp` for TDD.
- Update existing tests if they access these variables directly (e.g., via `FFBEngineTestAccess`).

## Verification Plan

### Automated Tests
- Run `tests/test_refactor_advanced.cpp` (Consistency, Round-Trip, Validation).
- Run all 590+ existing tests to ensure no regressions.

### Manual Verification
- Verify that the GUI sliders and checkboxes still work and correctly update the engine state.
- Verify that saving and loading presets still works as expected.

## Encountered Issues
- Numerous test files (over 40) directly accessed `m_gyro_gain`, `m_speed_gate_lower`, etc. These all had to be updated to use the new nested `m_advanced` path.
- The use of `sed` for mass-replacement in tests required careful validation to avoid double-nesting (e.g., `m_advanced.advanced.gyro_gain`). A Python script was used to perform more reliable replacements.
- MSVC compatibility: ensure `(std::max)` and `(std::min)` are used to avoid macro collision issues.

## Deviations from the Plan
- A Python script was introduced during the refactor to automate the update of the extensive test suite, which was more efficient and safer than manual find-and-replace or `sed` for this volume of changes.

## Suggestions for the Future
- Continue with remaining categories (SafetyConfig) to complete Phase 1.
- Proceed to Phase 2 (TOML integration) once all grouped structs are in place.
