# Implementation Plan - Issue #294: Rename variables for load and grip to make sure if only front grip is being used

This plan addresses Issue #294 by renaming variables in the `FFBEngine` and related modules to explicitly specify when they refer to the front axle's load and grip.

## Context
The LMUFFB codebase performs many calculations based on tire load and grip. While some variables are already prefixed with `front_` or `rear_`, others are ambiguous (e.g., `avg_load`). This ambiguity can lead to confusion during development and maintenance. Renaming these to `avg_front_load`, `avg_front_grip`, etc., makes the physics pipeline clearer and easier to audit.

## Design Rationale
- **Clarity and Explicit Semantics:** Renaming variables to include `_front` or `_front_axle` provides immediate clarity on the scope of the data.
- **Consistency:** Ensuring that all variables related to the front axle follow a similar naming convention throughout the codebase (FFBEngine, GripLoadEstimation, Logging, and GUI).
- **Auditability:** Makes it easier to verify that the correct data is being used for axle-specific effects like understeer (front) vs. oversteer (rear/comparison).

## Codebase Analysis Summary
- **FFBEngine.h/cpp:** Contains the main physics loop and context structure where most of these renames will take place.
- **GripLoadEstimation.cpp:** Implements the core logic for calculating these values; must be kept in sync with the engine's data structures.
- **AsyncLogger.h:** Defines the `LogFrame` structure; renames here improve the quality of logged telemetry for analysis.
- **GuiLayer_Common.cpp:** Visualizes these values in real-time; needs to be updated to match the new member names.

## FFB Effect Impact Analysis
The changes are primarily cosmetic (renaming) and should not change the FFB "feel" or logic, but they improve the readability of the implementation of the following effects:
- **Understeer Drop:** Uses front grip.
- **Dynamic Normalization:** Uses front axle load.
- **SoP Lateral:** Uses front load for normalization.
- **Grip Estimation Fallback:** Uses front load.

## Proposed Changes

### 1. `src/FFBEngine.h`
- Rename members in `FFBCalculationContext`:
  - `avg_load` -> `avg_front_load`
  - `avg_grip` -> `avg_front_grip`
- Rename members in `FFBEngine`:
  - `s_load` -> `s_front_load`
  - `s_grip` -> `s_front_grip`
  - `m_auto_peak_load` -> `m_auto_peak_front_load`
- Update function signature for `calculate_axle_grip`:
  - Rename function from `calculate_grip` to `calculate_axle_grip`.
  - Rename parameter `avg_load` to `avg_axle_load`.

### 2. `src/FFBEngine.cpp`
- In `calculate_force`:
  - Rename local `raw_load` to `raw_front_load`.
  - Rename local `raw_grip` to `raw_front_grip`.
  - Update all references to renamed members and locals.
- Update `calculate_sop_lateral` and other helper methods to use renamed members.

### 3. `src/GripLoadEstimation.cpp`
- Update `calculate_grip` implementation with renamed parameter and internal references.
- Update `InitializeLoadReference` and `update_static_load_reference` to use renamed `m_auto_peak_front_load`.

### 4. `src/AsyncLogger.h`
- Rename `LogFrame` field:
  - `load_peak_ref` -> `front_load_peak_ref`.

### 5. `src/GuiLayer_Common.cpp`
- Update any direct references to renamed members (e.g., `s_load`, `s_grip`).

### 6. Version Increment
- Increment version in `VERSION` to `0.7.151`.

## Test Plan
- **Build Verification:** Run `cmake --build build` to ensure the project compiles with no symbol errors.
- **Unit Tests:** Run `./build/tests/run_combined_tests` to ensure no functional regressions.
- **Visual Verification:** Check `src/FFBEngine.cpp` to ensure all `avg_load` and `avg_grip` usages have been correctly updated to their `_front` counterparts.

## Deliverables
- Modified `src/FFBEngine.h`
- Modified `src/FFBEngine.cpp`
- Modified `src/GripLoadEstimation.cpp`
- Modified `src/AsyncLogger.h`
- Modified `src/GuiLayer_Common.cpp` (Verified no changes needed)
- Updated `VERSION`
- Updated `CHANGELOG_DEV.md`
- Updated `tools/lmuffb_log_analyzer/loader.py` and its tests.
- This Implementation Plan with final notes.

## Implementation Notes
### Unforeseen Issues
- **Semantic Ambiguity in `calculate_axle_grip`**: Initial plan to rename the `avg_load` parameter to `avg_front_load` was evolved based on feedback. Since the function is shared by both axles, it was renamed to `calculate_axle_grip` and the parameter to `avg_axle_load` to maintain generic axle semantics while being explicit about the data scope.
- **GUI Access Misunderstanding**: Code review suggested `src/GuiLayer_Common.cpp` needed updates, but thorough investigation proved it relies on `FFBSnapshot` fields which already used appropriate naming, rather than the renamed engine members.

### Plan Deviations
- Renamed `calculate_grip` to `calculate_axle_grip`.
- Renamed parameter in `calculate_axle_grip` to `avg_axle_load` instead of `avg_front_load`.
- Updated Python log analyzer to ensure end-to-end telemetry consistency.

### Challenges
- Ensuring all unit tests were updated to match the new member and method names.

### Recommendations
- Continue the "Explicit Axle" naming convention for any new telemetry channels added in the future.
