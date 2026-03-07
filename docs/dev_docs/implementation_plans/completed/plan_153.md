# Implementation Plan - Issue #153: Implement FFB Strength Normalization Stage 2 - Hardware Scaling Redefinition (UI & Config)

## Context
Following the implementation of dynamic normalization for structural forces (Stage 1), the legacy `m_max_torque_ref` setting is now mathematically obsolete and actively confusing for users. Stage 2 replaces `m_max_torque_ref` with a "Physical Target" model consisting of two explicit settings: `m_wheelbase_max_nm` (the physical limit of the user's hardware) and `m_target_rim_nm` (the desired peak force at the steering rim). This completely decouples the user's hardware capabilities from their subjective strength preferences, and allows tactile textures to be rendered in absolute Newton-meters.

## Reference Documents
- `docs/dev_docs/implementation_plans/FFB Strength Normalization Plan Stage 2 - Hardware Scaling Redefinition (UI & Config).md`
- GitHub Issue #153: Implement FFB Strength Normalization Stage 2 - Hardware Scaling Redefinition (UI & Config)

## Codebase Analysis Summary
- **Current Architecture:** `m_max_torque_ref` is used to calculate `ctx.decoupling_scale` (`max_torque_ref / 20.0`). This scale is multiplied into almost every FFB effect. Finally, the total sum is divided by `max_torque_ref`. This causes textures to scale as a fixed percentage of the wheelbase capacity.
- **Impacted Functionalities:**
    - `FFBEngine.h`: Class member variables for FFB scaling.
    - `FFBEngine.cpp`: Physics calculation loop and effect summation.
    - `Config.h`: Preset structure and engine synchronization.
    - `Config.cpp`: Ini file parsing, saving, and migration logic.
    - `GuiLayer_Common.cpp`: User interface for FFB tuning.
    - `tests/`: New unit tests for scaling verification.

## FFB Effect Impact Analysis
| FFB Effect | Category | Impact | Technical Change | User-Facing Change |
| :--- | :--- | :--- | :--- | :--- |
| **Base Steering / SoP** | Structural | **Modified** | Mapped via `target_rim_nm / wheelbase_max_nm`. | Users now explicitly set how heavy they want the wheel to feel (e.g., 10 Nm) regardless of their wheelbase capacity. |
| **All Textures (Road, ABS, etc.)** | Tactile | **Modified** | `decoupling_scale` removed. Mapped via `1.0 / wheelbase_max_nm`. | Textures now output absolute physical torque. A 2 Nm ABS pulse will feel exactly like 2 Nm on *any* wheelbase. |

## Proposed Changes

### 1. FFBEngine Parameter Refactoring
- **FFBEngine.h**:
    - Remove `float m_max_torque_ref;`.
    - Add `float m_wheelbase_max_nm;`.
    - Add `float m_target_rim_nm;`.
- **FFBEngine.cpp**:
    - Remove `ctx.decoupling_scale` from `FFBCalculationContext`.
    - Delete `decoupling_scale` calculation logic.
    - In `calculate_force`, update the `raw_torque_input` calculation to use `m_wheelbase_max_nm` if `m_torque_source == 1`.
    - Remove `* ctx.decoupling_scale` from all effect functions: `calculate_sop_lateral`, `calculate_gyro_damping`, `calculate_abs_pulse`, `calculate_lockup_vibration`, `calculate_wheel_spin`, `calculate_slide_texture`, `calculate_road_texture`, `calculate_soft_lock`, `calculate_suspension_bottoming`.
    - Update **SUMMATION** and **OUTPUT SCALING** sections:
      ```cpp
      double di_structural = norm_structural * ((double)m_target_rim_nm / (double)m_wheelbase_max_nm);
      double di_texture = texture_sum_nm / (double)m_wheelbase_max_nm;
      double norm_force = (di_structural + di_texture) * m_gain;
      ```

### 2. Configuration & Preset Updates
- **Config.h**:
    - Update `Preset` struct: Replace `max_torque_ref` with `wheelbase_max_nm` (default 15.0) and `target_rim_nm` (default 10.0).
    - Update `SetMaxTorque` to `SetHardwareScaling(float wheelbase, float target)`.
    - Update `Apply`, `UpdateFromEngine`, `Validate`, and `Equals` to handle the two new fields.
    - In `Apply`, ensure `m_session_peak_torque` is initialized from `m_target_rim_nm` (or a safe floor) to avoid Stage 1 normalization spikes.
- **Config.cpp**:
    - `ParsePresetLine`: Remove `max_torque_ref` handler, add `wheelbase_max_nm` and `target_rim_nm`.
    - **Migration Logic**: In `ParsePresetLine`, if `max_torque_ref` is encountered, convert it:
        - If `> 40.0`, set `wheelbase = 15.0, target = 10.0` (Safe DD default).
        - Else, set `wheelbase = old_val, target = old_val` (1:1 mapping for calibrated users).
    - `LoadPresets`: Update all hardcoded presets (T300, Simagic, Moza) with realistic Nm values.
    - `WritePresetFields` & `Save`: Update to write new keys.

### 3. User Interface Updates
- **GuiLayer_Common.cpp**:
    - Replace the "Max Torque Ref" slider with:
        - "Wheelbase Max Torque" (1.0 to 50.0 Nm).
        - "Target Rim Torque" (1.0 to 50.0 Nm).
    - Update tooltips to explain the decoupling.
    - Remove/Update the `FormatDecoupled` helper as it used `m_max_torque_ref`.

### 4. Version Increment
- Update `VERSION` and `src/Version.h` to `0.7.68`.

## Test Plan (TDD-Ready)
**New Test File:** `tests/test_ffb_hardware_scaling.cpp`

1. **`test_hardware_scaling_structural`**:
   - Inputs: `norm_structural = 1.0`, `target_rim = 10.0`, `wheelbase_max = 20.0`.
   - Expected `di_structural`: `0.5`.
2. **`test_hardware_scaling_textures`**:
   - Inputs: `texture_sum_nm = 5.0`, `wheelbase_max = 20.0`.
   - Expected `di_texture`: `0.25`.
   - Verify that changing `target_rim` does NOT affect `di_texture`.
3. **`test_config_migration`**:
   - Input: Parse `max_torque_ref=100.0`.
   - Expected: `wheelbase_max=15.0`, `target_rim=10.0`.
   - Input: Parse `max_torque_ref=20.0`.
   - Expected: `wheelbase_max=20.0`, `target_rim=20.0`.

## Deliverables
- [x] Modified `src/FFBEngine.h` & `src/FFBEngine.cpp`
- [x] Modified `src/Config.h` & `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] New `tests/test_ffb_hardware_scaling.cpp`
- [x] Updated `VERSION` & `src/Version.h` (Version.h is auto-generated via CMake from VERSION and src/Version.h.in)
- [x] Implementation Notes (updated in this document upon completion)

## Implementation Notes
### Unforeseen Issues
- **Hidden 1/20 Scaling in Tests**: Many existing tests were failing because they relied on the legacy `m_max_torque_ref / 20.0` (decoupling_scale) math, which provided a 1/5 attenuation at the default 100 Nm. These tests had hardcoded expectations that didn't match the new 1:1 physical torque model.
### Plan Deviations
- **Test Suite Updates**: Had to update approximately 15 existing test files to fix their scaling expectations. Most tests were updated to use a consistent `m_wheelbase_max_nm = 100.0` and `m_target_rim_nm = 100.0` baseline to maintain 1:1 Nm-to-Normalized scaling for easier verification.
- **Version.h**: Noted that `Version.h` is an artifact of CMake's `configure_file`. I updated `VERSION` (the source of truth) and verified that the build system propagates this.
### Challenges
- **Refactoring Private Members in Tests**: The new dynamic normalization variables (`m_session_peak_torque`) were private in `FFBEngine`. I had to use `FFBEngineTestAccess` (a friend class) to manipulate these in unit tests to verify the math without needing complex telemetry.
### Recommendations
- Future stages should look into persisting the session peak torque across game restarts to avoid the short "learning" phase at the start of each session.
