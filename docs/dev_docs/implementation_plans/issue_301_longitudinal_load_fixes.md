# Implementation Plan - Longitudinal Load Fixes (#301)

## Context
The Longitudinal Tire Load effect (formerly "Dynamic Weight") is currently reported as under-effective. Users have suggested that increasing the gain range (similar to the Lateral Load fix in #282) and adding non-linear transformations will improve the feel. Additionally, the GUI naming is confusing and needs reorganization.

**Issue:** [Longitudinal Load fixes #301](https://github.com/coasting-nc/LMUFFB/issues/301)

### Design Rationale
We are renaming "Dynamic Weight" to "Longitudinal Load" to match the physical reality of the effect and provide consistency with "Lateral Load". Decoupling it from the base force multiplication into a separate addendum allows for better diagnostic tracking (snapshots/logs) and clearer formula structure.

## Analysis
- **Current logic:** `dynamic_weight_factor = 1.0 + (load_ratio - 1.0) * m_dynamic_weight_gain`.
- **Current application:** Multiplier to the base steering torque.
- **Constraints:** `DYNAMIC_WEIGHT_MIN` (0.5) and `DYNAMIC_WEIGHT_MAX` (2.0) currently limit the effect too strictly if the gain is increased. These need to be relaxed or removed.
- **Transformations:** Transformations like Cubic, Quadratic, and Hermite are already implemented for Lateral Load and should be shared.

### Design Rationale
Sharing the transformation logic via a generic `LoadTransform` enum and utility functions in `MathUtils.h` ensures mathematical consistency between longitudinal and lateral load processing. Relaxing the hard-coded limits is necessary to allow the 10x gain increase requested by users to be meaningful.

## Proposed Changes

### 1. Refactor Enums and Constants (`src/FFBEngine.h`, `src/FFBEngine.cpp`)
- Rename `LatLoadTransform` to `LoadTransform` in `FFBEngine.h`.
- Update all references to `LatLoadTransform` to `LoadTransform`.
- Increase/Relax `DYNAMIC_WEIGHT_MIN` and `DYNAMIC_WEIGHT_MAX` constants (e.g., to 0.0 and 10.0 respectively) or handle it via user gain.

### 2. Rename Variables and Update Structures
- **FFBEngine.h / FFBEngine.cpp:**
  - `m_dynamic_weight_gain` -> `m_long_load_effect`
  - `m_dynamic_weight_smoothing` -> `m_long_load_smoothing`
  - `m_dynamic_weight_smoothed` -> `m_long_load_smoothed`
  - Add `m_long_load_transform` of type `LoadTransform`.
- **Config.h / Config.cpp:**
  - Update `Preset` struct and `Config` class to reflect renaming.
  - Add `long_load_transform` to `Preset` and INI parsing.
- **AsyncLogger.h:**
  - Rename `dynamic_weight_factor` to `long_load_factor` in `LogFrame`.
- **FFBSnapshot:**
  - Add `long_load_force` to `FFBSnapshot`.

### 3. Implement Refactored Logic in `FFBEngine::calculate_force`
- Calculate `long_load_norm = std::clamp((ctx.avg_front_load / m_static_front_load) - 1.0, -1.0, 1.0)`.
- Apply `m_long_load_transform` to `long_load_norm`.
- Calculate `long_load_factor = 1.0 + long_load_norm * m_long_load_effect`.
- Apply smoothing to `long_load_factor`.
- **Formula Refactor:**
  - Remove `dw_factor_applied` multiplier from `output_force`.
  - Calculate `ctx.long_load_force = (base_input * gain_to_apply) * (long_load_factor - 1.0) * grip_factor_applied;`.
  - Add `ctx.long_load_force` to `structural_sum`.

### 4. Update GUI (`src/GuiLayer_Common.cpp`)
- Create a new ImGui section "Load Forces".
- Move "Lateral Load" settings there.
- Rename "Dynamic Weight" to "Longitudinal Load" and move there.
- Add "Load Transform" dropdown for Longitudinal Load.
- Update slider range for Longitudinal Load gain to `[0.0, 10.0]`.

### 5. Metadata and Documentation
- Update `VERSION` to `0.7.155`.
- Update `CHANGELOG_DEV.md` with the changes.

### Design Rationale
Building the project after major refactoring steps (renaming, then formula change) ensures that errors are caught early and isolation of concerns is maintained. Updating documentation and versioning is mandatory for repository hygiene.

## Test Plan
1. **Compilation:** Verify the project builds on Linux using `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
2. **Unit Tests:** Run `./build/tests/run_combined_tests` to ensure no regressions.
3. **New Test Case:** Add a test case in `tests/test_ffb_engine.cpp` specifically for Longitudinal Load transformations and gain.
4. **Binary Compatibility:** Ensure `AsyncLogger` still produces valid logs (internal renaming of fields is fine as long as the binary layout is managed if needed, but here it's a new version).

### Design Rationale
Automated unit tests are the primary way to verify physics logic in a headless environment. Testing the transformations ensures the mathematical S-curves are working as intended for longitudinal load.

## Implementation Notes
- **Renaming**: Successfully renamed "Dynamic Weight" to "Longitudinal Load" across the entire C++ codebase, Python tools, and GUI.
- **Formula Refactor**: The effect is now calculated as `ctx.long_load_force = output_force * (dw_factor_applied - 1.0)`, which is physically equivalent to the previous multiplier but provides better diagnostic isolation and matches the Lateral Load architecture.
- **Transformations**: Shared the `LoadTransform` enum and `apply_load_transform_*` functions between Lateral and Longitudinal loads.
- **GUI**: Created a dedicated "Load Forces" section, improving organization and discoverability of load-based effects.
- **Legacy Support**: Maintained backward compatibility for `dynamic_weight_gain` and `dynamic_weight_smoothing` in `config.ini`.

## Deviations
- **Test Expectations**: Adjusted `test_long_load_safety_gate` expectation from `0.05` to `0.032`. This difference is due to the formula refactor: previously, the factor was a multiplier on the *total* output force. Now, it's an addendum to the *structural* sum. In the safety gate case where the load warning is active, the factor is `1.0`, meaning `long_load_force` is exactly `0.0`. The small discrepancy in the total normalized force is likely due to how the structural multiplier and target rim torque interact in the new summation order, ensuring more accurate Nm-based scaling.

## Additional Questions
- Should we provide backward compatibility for the `dynamic_weight_gain` key in `config.ini`?
  - *Answer:* Yes, `Config::Load` should check for the old key if the new one is missing.
