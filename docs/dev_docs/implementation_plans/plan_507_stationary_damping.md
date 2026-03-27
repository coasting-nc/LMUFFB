# Implementation Plan - Issue #507: Change default Stationary Damping setting from 100% to 40%

## Context
The goal is to reduce the default "Stationary Damping" from 100% (1.0) to 40% (0.4) project-wide, including default initialization and built-in presets.

## Design Rationale
Stationary damping provides resistance when the car is stopped or moving slowly to prevent the wheel from being too "loose" or oscillating. A default of 100% can feel too heavy or restrictive for some users, especially those with high-torque direct drive wheels. Reducing the default to 40% provides a better "out-of-the-box" experience that feels more natural while still providing enough damping to maintain stability.

## Codebase Analysis Summary
- **src/ffb/FFBConfig.h**: Defines the `AdvancedConfig` struct where `stationary_damping` is initialized. This is the primary source of the default value.
- **src/core/Config.cpp**: Handles loading and saving of presets. It uses the default values from the config structs if keys are missing in TOML files.
- **assets/builtin_presets/**: Built-in presets in TOML format. These may contain explicit overrides for `stationary_damping`.
- **tests/test_issue_418_stationary_damping.cpp**: Regression tests for stationary damping that currently expect a default of 1.0.

## FFB Effect Impact Analysis
- **Affected Effect**: `stationary_damping_force` in `FFBEngine`.
- **Technical Change**: The default value of `m_advanced.stationary_damping` will change from `1.0f` to `0.4f`.
- **User Perspective**: The wheel will feel lighter when the car is stationary by default. It will still be adjustable via the "Stationary Damping" slider in the "Advanced" tab of the GUI. This change improves initial user experience by avoiding an overly heavy wheel feel at zero speed.

## Proposed Changes

### 1. Update Core Configuration Default
- **File**: `src/ffb/FFBConfig.h`
- **Change**: Change `float stationary_damping = 1.0f;` to `float stationary_damping = 0.4f;` in `struct AdvancedConfig`.
- **Design Rationale**: Centralizing the default in the configuration struct ensures all new engine instances and unconfigured presets use the new project standard.
- **Verification**: Confirmed via `read_file`.

### 2. Audit and Update Built-in Presets
- **Action**: Use `grep -r "stationary_damping" assets/builtin_presets/` to find explicit overrides.
- **Change**: If any preset explicitly sets `stationary_damping = 1.0`, update it to `0.4`.
- **Findings**: No explicit overrides found in `assets/builtin_presets/`. All presets will inherit the new `0.4f` default automatically.

### 3. Update Regression Tests
- **File**: `tests/test_issue_418_stationary_damping.cpp`
- `test_stationary_damping_default`: Updated expected value from `1.0f` to `0.4f`.
- `test_stationary_damping_preset_inheritance`: Implemented new test case to verify that a preset without an explicit `stationary_damping` key correctly inherits the `0.4f` default.
- **Architectural Adjustment**: Refactored `Config::TomlToPreset` to be public to support the inheritance test.

### 4. Build and Execute Tests
- **Action**: Built project using `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_HEADLESS=ON && cmake --build build`.
- **Action**: Ran combined tests using `./build/tests/run_combined_tests`.
- **Result**: All 634 tests passed.

### 5. Versioning and Documentation
- **File**: `VERSION`
- **Action**: Incremented version to `0.7.260`.
- **File**: `CHANGELOG_DEV.md`
- **Action**: Added entry for Issue #507.

## Test Plan
- **Baseline**: Previous tests pass with `1.0` default.
- **Implementation**: Modified core code, tests, and added new verification.
- **Verification**: Verified via 100% test pass rate in CI-equivalent environment.

## Deliverables
- [x] Modified `src/ffb/FFBConfig.h`
- [x] Modified `src/core/Config.h` and `src/core/Config.cpp` (Refactor for testability)
- [x] Updated `tests/test_issue_418_stationary_damping.cpp`
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`
- [x] GitHub issue copy in `docs/dev_docs/github_issues/507_change_default_stationary_damping.md`

## Implementation Notes
### Encountered Issues
- **Test Suite Compilation**: The new inheritance test required `TomlToPreset`, which was private and not in the `LMUFFB` namespace scope in `Config.cpp`.
- **Fix**: Refactored `TomlToPreset` to be a public static member of `Config`, resolving the visibility and namespace issues while improving overall project testability.

### Plan Deviations
- **Refactoring for Testability**: Added a small refactor of the `Config` class to expose serialization helpers, which was not explicitly in the initial plan but became necessary for robust TDD verification of preset inheritance.

### Suggestions for the Future
- **Preset Audit Tools**: Consider adding a CI check that verifies all built-in presets are valid against the current configuration schema to prevent future regressions when defaults change.
