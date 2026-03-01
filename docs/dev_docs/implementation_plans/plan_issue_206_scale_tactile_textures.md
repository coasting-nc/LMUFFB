# Implementation Plan - Issue #206: Scale the strength of Absolute Tactile Textures

## Context
Issue #206 requests a separate slider to scale the strength of "Absolute Tactile Textures". Currently, tactile effects (Road Details, Slide Rumble, Lockup Vibration, etc.) are calculated in absolute Nm and mapped directly to the wheelbase's maximum torque. On some hardware (especially lower-end wheels), these absolute values can be disproportionately strong or weak. A dedicated "Tactile Strength" slider will allow users to scale these effects globally while maintaining their internal absolute Nm logic.

## Reference Documents
- GitHub Issue #206: [Scale the strength of Absolute Tactile Textures](https://github.com/coasting-nc/LMUFFB/issues/206)
- GitHub Issue #153: Stage 2 Hardware Strength Scaling.
- GitHub Issue #154: Stage 3 Tactile Haptics Normalization.
- GitHub Issue #181: Soft Lock Absolute Nm consistency.
- GitHub Issue #207: Disable Dynamic Normalization by default.

## Codebase Analysis Summary
- **FFBEngine.h/cpp**:
  - `calculate_force()`: Sums structural forces and tactile textures. Currently, tactile textures are summed into `texture_sum_nm` and converted to DirectInput percentage using `wheelbase_max_safe`.
  - Soft Lock is currently included in the `texture_sum_nm` (Absolute Nm) group.
- **Config.h/cpp**:
  - `Preset` struct: Holds all configurable parameters.
  - `Config::Load/Save`: Persists settings to `config.ini`.
- **GuiLayer_Common.cpp**:
  - Implements the ImGui-based user interface.
- **Tooltips.h**:
  - Centralized storage for UI tooltips.

## FFB Effect Impact Analysis
| Effect | Technical Changes | Expected User-Facing Change |
| :--- | :--- | :--- |
| **Tactile Textures** | Multiplied by `m_tactile_gain` before final scaling. Includes: Road Noise, Slide Noise, Spin Rumble, Bottoming Crunch, ABS Pulse, Lockup Rumble. | User can globally increase/decrease the intensity of all haptic "vibrations" using a single slider. |
| **Soft Lock** | **EXCLUDED** from `m_tactile_gain` scaling. | Remains at a consistent absolute Nm level for safety and physical realism. |
| **Structural Forces** | **No Change**. | Steering weight and SoP feel remain unaffected by the tactile gain. |

## Proposed Changes

### 1. FFBEngine.h
- Add `float m_tactile_gain = 1.0f;` to the `FFBEngine` class.

### 2. FFBEngine.cpp
- Update `calculate_force()`:
  - Calculate `texture_sum_nm` (tactile textures).
  - Apply `m_tactile_gain` to `texture_sum_nm`.
  - Keep `soft_lock_force` separate or add it back after `m_tactile_gain` is applied.
  - Current summation logic in `calculate_force()`:
    ```cpp
    double texture_sum_nm = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch + ctx.abs_pulse_force + ctx.lockup_rumble + ctx.soft_lock_force;
    ```
  - Proposed summation logic:
    ```cpp
    double tactile_sum_nm = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch + ctx.abs_pulse_force + ctx.lockup_rumble;
    double final_texture_nm = (tactile_sum_nm * m_tactile_gain) + ctx.soft_lock_force;
    ```

### 3. Config.h
- Add `float tactile_gain = 1.0f;` to the `Preset` struct.
- Add `Preset& SetTactileGain(float v)` fluent setter.
- Update `Preset::Apply()` to copy `tactile_gain` to `engine.m_tactile_gain` (with clamping `[0.0, 2.0]`).
- Update `Preset::UpdateFromEngine()` to copy `engine.m_tactile_gain` back to `tactile_gain`.
- Update `Preset::Equals()` to include `tactile_gain`.

### 4. Config.cpp
- Update `ParsePresetLine()` to handle `tactile_gain`.
- Update `WritePresetFields()` to include `tactile_gain`.
- Update `Config::Save()` to write `engine.m_tactile_gain` to `config.ini`.
- Update `Config::Load()` to read `engine.m_tactile_gain` from `config.ini`.

### 5. GuiLayer_Common.cpp
- In `DrawTuningWindow()`, inside the "Tactile Textures" section:
  - Add `FloatSetting("Tactile Strength", &engine.m_tactile_gain, 0.0f, 2.0f, FormatPct(engine.m_tactile_gain), Tooltips::TACTILE_GAIN);`

### 6. Tooltips.h
- Add `inline constexpr const char* TACTILE_GAIN = "Global multiplier for all haptic textures (Road, Slide, Lockup, etc.). Allows scaling absolute Nm effects to better match your hardware.";`

### 7. Versioning
- Increment version to `0.7.110`.

## Parameter Synchronization Checklist
- [x] Declaration in `FFBEngine.h`: `float m_tactile_gain`
- [x] Declaration in `Preset` struct: `float tactile_gain`
- [x] Entry in `Preset::Apply()`
- [x] Entry in `Preset::UpdateFromEngine()`
- [x] Entry in `Config::Save()`
- [x] Entry in `Config::Load()`
- [x] Validation logic: `std::clamp(v, 0.0f, 2.0f)`

## Test Plan
- **Baseline Test Verification**: Ensure all existing tests pass.
- **New Unit Test**: `tests/test_issue_206_tactile_scaling.cpp`
  - **Test case 1**: Verify `m_tactile_gain = 0.5` reduces Road/Slide/Lockup by half.
  - **Test case 2**: Verify `m_tactile_gain = 2.0` doubles Road/Slide/Lockup.
  - **Test case 3**: Verify `m_tactile_gain` does NOT affect `soft_lock_force`.
  - **Test case 4**: Verify `m_tactile_gain` does NOT affect structural steering torque.
- **Persistence Test**: Verify `tactile_gain` is correctly saved to and loaded from `config.ini`.

## Deliverables
- [x] Modified `src/FFBEngine.h`, `src/FFBEngine.cpp`
- [x] Modified `src/Config.h`, `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `src/Tooltips.h`
- [ ] Modified `VERSION`, `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`
- [x] New test file `tests/test_issue_206_tactile_scaling.cpp`.
- [x] Implementation Notes (to be added at the end).

## Implementation Notes

### Unforeseen Issues
- **Test Case Clipping**: Initial unit test failed because the `calculate_force` output was clipping at 1.0 (or -1.0) due to high steering excess and low wheelbase max torque. Increased `m_wheelbase_max_nm` to 1000.0f in the test to ensure linear scaling could be verified without saturation.
- **Soft Lock Trigger**: Adjusted steering excess to a more reasonable 1.01f (1% excess) to ensure Soft Lock was active but not dominating the entire signal range in a way that masked texture scaling.

### Plan Deviations
- **None**: The implementation followed the proposed architectural changes exactly.

### Challenges
- **Snapshot Verification**: Realized that `FFBSnapshot` stores the *raw* texture values before the global tactile gain is applied. To verify scaling, the test had to compare the final `total_output` after zeroing out structural forces. This was a cleaner approach than adding more members to the snapshot.

### Recommendations for Future Plans
- When testing scaling of summed signals, always ensure the "base" (structural) components are zeroed or held constant, and verify that the hardware limit (clipping) is not reached during the test.
