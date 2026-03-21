# Implementation Plan - Preset System Redesign (Phase 1: BrakingConfig)

## Overview
This plan covers the implementation of the `BrakingConfig` logical category in the preset system refactor, following the "Strangler Fig" pattern. It groups all lockup vibration and ABS pulse parameters into a structured data model.

## Target Variables
The following variables are migrated into `BrakingConfig`:
- `lockup_enabled`
- `lockup_gain`
- `lockup_start_pct`
- `lockup_full_pct`
- `lockup_rear_boost`
- `lockup_gamma`
- `lockup_prediction_sens`
- `lockup_bump_reject`
- `brake_load_cap`
- `lockup_freq_scale`
- `abs_pulse_enabled`
- `abs_gain`
- `abs_freq` (maps to `m_abs_freq_hz` in `FFBEngine`)

## Proposed Changes

### 1. Data Model
- Define `BrakingConfig` struct in `src/ffb/FFBConfig.h`.
- Implement `Equals()` and `Validate()` methods with appropriate safety clamping.

### 2. Engine and Preset Integration
- Update `FFBEngine` to hold a `m_braking` member.
- Update `Preset` to hold a `braking` member.
- Refactor `Apply()`, `UpdateFromEngine()`, `Equals()`, and `Validate()` in both classes.
- Update fluent setters in `Preset` to use the new nested structure.

### 3. I/O and GUI
- Update `Config.cpp` parsing (`ParseBrakingLine`, `SyncBrakingLine`) and serialization (`WritePresetFields`, `Save`) logic.
- Update `GuiLayer_Common.cpp` slider and checkbox bindings.

### 4. Verification
- Create `tests/test_refactor_braking.cpp` with safety tests:
    - **Consistency Test**: Verify physics output matches the baseline.
    - **Round-Trip Test**: Verify `Apply` and `UpdateFromEngine` symmetry.
    - **Validation Test**: Verify clamping logic in `BrakingConfig::Validate`.
- Update existing tests to use the new nested paths.

## Implementation Notes

### Encountered Issues
- (To be updated during implementation)

### Deviations from the Plan
- (To be updated during implementation)

### Suggestions for the Future
- (To be updated during implementation)
