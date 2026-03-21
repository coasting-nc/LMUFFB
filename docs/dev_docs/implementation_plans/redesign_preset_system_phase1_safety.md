# Redesign Preset System - Phase 1 - SafetyConfig

Implementation plan for refactoring Safety configuration parameters into a logical `SafetyConfig` category.

## Goals
- Group loose safety-related variables in `Preset` and `FFBSafetyMonitor` into a `SafetyConfig` struct.
- Maintain full backward compatibility with `config.ini`.
- Ensure no changes to default values or physics logic.

## Parameters to Move
From `Preset` (loose variables) and `FFBSafetyMonitor` (internal members):
- `safety_window_duration`
- `safety_gain_reduction`
- `safety_smoothing_tau`
- `spike_detection_threshold`
- `immediate_spike_threshold`
- `safety_slew_full_scale_time_s`
- `stutter_safety_enabled`
- `stutter_threshold`

## Steps
1. **TDD - Create Safety Regression Tests:**
   - Implement consistency, round-trip, and validation tests in `tests/test_refactor_safety.cpp`.
2. **Define `SafetyConfig` Struct:**
   - Create the struct in `src/ffb/FFBConfig.h` with `Equals()` and `Validate()`.
3. **Refactor `Preset` and `FFBSafetyMonitor`:**
   - Replace loose variables with the new struct.
   - Update `Apply`, `UpdateFromEngine`, `Validate`, and `Equals` methods.
4. **Fix Compiler Errors:**
   - Update `Config.cpp` for INI I/O.
   - Update `GuiLayer_Common.cpp` for UI controls.
   - Update tests to use new struct paths.
5. **Verification:**
   - Run all tests and ensure they pass.

## Encountered Issues
- None yet.

## Deviations from the Plan
- None yet.

## Suggestions for the Future
- Continue with other categories until all are refactored.
