# Implementation Plan - Redesign Preset System Phase 1 (VibrationConfig)

This document outlines the plan for refactoring the vibration-related configuration parameters into a grouped `VibrationConfig` structure.

## Proposed Changes

### 1. Data Structures
- Define `VibrationConfig` struct in `src/ffb/FFBConfig.h`.
- Include members: `vibration_gain`, `texture_load_cap`, `slide_enabled`, `slide_gain`, `slide_freq`, `road_enabled`, `road_gain`, `spin_enabled`, `spin_gain`, `spin_freq_scale`, `scrub_drag_gain`, `bottoming_enabled`, `bottoming_gain`, `bottoming_method`.
- Implement `Equals()` and `Validate()` methods.

### 2. `Preset` Struct (src/core/Config.h)
- Replace loose vibration variables with `VibrationConfig vibration;`.
- Update `Apply()`, `UpdateFromEngine()`, `Equals()`, and `Validate()`.
- Update fluent setters (e.g., `SetSpin`, `SetSlide`, etc.).

### 3. `FFBEngine` Class (src/ffb/FFBEngine.h)
- Replace loose vibration variables with `VibrationConfig m_vibration;`.

### 4. Configuration I/O (src/core/Config.cpp)
- Update `ParseVibrationLine` and `SyncVibrationLine` to use the new struct paths.
- Update `WritePresetFields` to use the new struct paths.
- Update `LoadPresets` hardcoded presets.

### 5. Physics Engine (src/ffb/FFBEngine.cpp)
- Update all physics calculations to use `m_vibration.<variable>`.

### 6. GUI (src/gui/GuiLayer_Common.cpp)
- Update ImGui sliders to point to the new struct paths.

### 7. Tests
- Create `tests/test_refactor_vibration.cpp` for TDD.
- Update existing tests if they access these variables directly (e.g., via `FFBEngineTestAccess`).

## Verification Plan

### Automated Tests
- Run `tests/test_refactor_vibration.cpp` (Consistency, Round-Trip, Validation).
- Run all 590+ existing tests to ensure no regressions.
- Verify that `test_refactor_vibration_consistency` matches the pre-refactor output exactly.

### Manual Verification
- Verify that the GUI sliders still work and correctly update the engine state.
- Verify that saving and loading presets still works as expected.

## Encountered Issues
- Pre-refactor tests: 595/595 passed (2813 assertions).
- Refactored access to vibration variables in production code and test suite.
- Fixed member access errors in `Config.cpp`, `FFBEngine.cpp`, `GuiLayer_Common.cpp`, and numerous test files.
- All 24 `RefactorSafety` tests pass, confirming the structural refactor preserved physics and synchronization logic.

## Deviations from the Plan
(To be updated)

## Suggestions for the Future
(To be updated)
