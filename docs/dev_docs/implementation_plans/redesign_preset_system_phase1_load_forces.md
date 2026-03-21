# Implementation Plan - Redesign Preset System Phase 1 (LoadForcesConfig)

This document describes the plan and progress for refactoring the Load Forces configuration into a grouped structure.

## Proposed Changes

### 1. Define `LoadForcesConfig` Struct
- Location: `src/ffb/FFBConfig.h`
- Members:
    - `float lat_load_effect`
    - `int lat_load_transform`
    - `float long_load_effect`
    - `float long_load_smoothing`
    - `int long_load_transform`
- Methods:
    - `bool Equals(const LoadForcesConfig& o, float eps = 0.0001f) const`
    - `void Validate()`

### 2. Update `Preset` Struct
- Location: `src/core/Config.h`
- Replace loose variables with `LoadForcesConfig load_forces`.
- Update `Apply()`, `UpdateFromEngine()`, `Equals()`, and `Validate()`.
- Update fluent setters.

### 3. Update `FFBEngine` Class
- Location: `src/ffb/FFBEngine.h`
- Replace loose variables with `LoadForcesConfig m_load_forces`.

### 4. Find and Replace
- `src/ffb/FFBEngine.cpp`
- `src/core/Config.cpp`
- `src/gui/GuiLayer_Common.cpp`
- Other files as identified by `grep`.

## Safety Testing (TDD)
- Create `tests/test_refactor_load_forces.cpp`.
- **Consistency Test:** Verify `final_force` output remains identical.
- **Round-Trip Test:** Verify `Preset <-> Engine` synchronization.
- **Validation Test:** Verify clamping logic.

## Progress Journal
- [x] Create safety tests.
- [x] Define `LoadForcesConfig` struct.
- [x] Update `Preset` and `FFBEngine`.
- [x] Fix compiler errors and find-and-replace.
- [x] Verify all tests pass.

## Encountered Issues
- **Enum to Int conversion in Tests:** Several tests were assigning `LoadTransform` enum values directly to `int` members, which caused compilation errors after refactoring. Resolved by adding `static_cast<int>()` or `(int)` casts in test files.
- **Widespread Test impact:** Load force variables were used in many regression tests (over 15 files). Used `sed` to perform bulk updates across the `tests/` directory.
- **main.cpp usage:** Load force variables were also used in `src/core/main.cpp` for telemetry logging info.

## Deviations from the Plan
- Updated `src/core/main.cpp` which wasn't explicitly mentioned in the "Header Phase" but was necessary for compilation.
- Bulk updated all tests using `sed` to handle the large volume of changes.

## Suggestions for the Future
- Move `LoadTransform` enum to `FFBConfig.h` to avoid casting if possible, or change the struct members to use the enum type once the refactoring is more complete.
