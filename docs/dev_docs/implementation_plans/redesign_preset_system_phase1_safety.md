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
- **Missing Header in FFBSafetyMonitor.h**: After refactoring `FFBSafetyMonitor` to use `SafetyConfig`, I had to add `#include "FFBConfig.h"` to the header to fix compiler errors.
- **Multiple Test Failures**: Several existing safety-related tests (`test_issue_303_safety.cpp`, `test_issue_314_safety_v2.cpp`, `test_issue_316_safety_gui.cpp`, `test_coverage_boost_v7.cpp`, `test_coverage_boost_v8.cpp`) directly accessed the old loose variables in `engine.m_safety` and `Preset`. I had to update all these tests to use the new nested paths.
- **main.cpp Update**: The stutter detection logic in `main.cpp` used the loose variables in `g_engine.m_safety`. I updated it to use `g_engine.m_safety.m_config`.

## Deviations from the Plan
- None.

## Suggestions for the Future
- Now that `SafetyConfig` is implemented, all planned Phase 1 categories are complete.
