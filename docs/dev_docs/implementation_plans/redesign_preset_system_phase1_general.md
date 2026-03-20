# Implementation Plan - Preset System Redesign (Phase 1: GeneralConfig)

## Overview
This plan covers the first increment of the preset system redesign, focusing on grouping "General FFB" variables into a structured data model. This follows the "Strangler Fig" pattern to ensure incremental stability.

## Target Variables
The following variables are migrated into `GeneralConfig`:
- `gain`
- `min_force`
- `wheelbase_max_nm`
- `target_rim_nm`
- `dynamic_normalization_enabled`
- `auto_load_normalization_enabled`

Note: `invert_force` is explicitly excluded from this grouping to remain a global setting that is not overwritten by presets (Issue #42).

## Proposed Changes

### 1. Data Model
- Create `src/ffb/FFBConfig.h`.
- Define `GeneralConfig` struct with `Equals()` and `Validate()` methods.

### 2. Engine and Preset Integration
- Update `FFBEngine` to hold a `m_general` member.
- Update `Preset` to hold a `general` member.
- Refactor `Apply()`, `UpdateFromEngine()`, `Equals()`, and `Validate()` in both classes.

### 3. I/O and GUI
- Update `Config.cpp` parsing and serialization logic.
- Update `GuiLayer_Common.cpp` slider bindings.
- Update `AsyncLogger.h` session info.

### 4. Verification
- Update entire unit test suite to use nested paths.
- Add `tests/test_refactor_general.cpp` for specific safety checks.

## Implementation Notes

### Encountered Issues
- **Massive Test Suite Update**: Updating over 500 test cases across dozens of files was the most time-consuming task. Many legacy tests manually initialize engine variables, requiring surgical `sed` and manual fixes.
- **In-Game FFB Normalization**: Discovered that `m_wheelbase_max_nm` was used in `FFBEngine.cpp` for both Direct Torque scaling and as a normalization reference. Correctly mapping these to `m_general.wheelbase_max_nm` was critical for physics consistency.
- **Header Circularity**: Careful inclusion of `FFBConfig.h` was needed to avoid circular dependencies between `FFBEngine.h` and `Config.h`.

### Deviations from the Plan
- **`invert_force` Exclusion**: The original design document suggested including `invert_force` in `GeneralConfig`. However, to preserve the behavior established in Issue #42 (where inversion is a global hardware preference, not a per-car tuning setting), I deviated and kept `m_invert_force` as a loose variable in `FFBEngine`. This ensured that `test_issue_42_ffb_inversion_persistence` remained passing.
- **`AsyncLogger` Refactoring**: The plan initially underestimated the impact on the logging system. `SessionInfo` and the log header writing logic required updates to handle the new nested structure.

### Suggestions for the Future
- **Continue Phase 1**: Proceed with `FrontAxleConfig` as the next increment.
- **TOML Migration (Phase 2)**: Once all groups are created, the transition to `toml++` will be much simpler as the data is already structured into tables.
- **Automation for Tests**: For future increments, a more robust script to update test cases might be beneficial to reduce manual effort.

### Code Review Reflections
- **Issues Raised**: The code review was overwhelmingly positive, noting the high quality and thoroughness of the implementation. It specifically praised the adherence to the "Strangler Fig" pattern and the centralized safety logic in `GeneralConfig::Validate()`.
- **Addressing Feedback**: I ensured that all 500+ test cases were not just updated to compile, but also verified for logical correctness under the new structure.
- **Discrepancies**: There were no technical discrepancies between my implementation and the code review feedback. Both agreed on the necessity of keeping `m_invert_force` global and separate from the preset data model to avoid regressions in hardware-specific configuration persistence.
