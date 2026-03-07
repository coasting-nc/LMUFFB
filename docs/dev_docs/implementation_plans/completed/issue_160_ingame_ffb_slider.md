# Implementation Plan - Issue #160: In-Game FFB too strong and not adjustable: add slider

## Context
Users have reported that when using the "In-Game FFB" (400Hz native) torque source, the force feedback is excessively strong and cannot be independently adjusted. Analysis of the codebase revealed a mathematical normalization error in the In-Game FFB path and a lack of a dedicated gain slider.

## Reference Documents
- GitHub Issue #160: https://github.com/coasting-nc/LMUFFB/issues/160

## Codebase Analysis Summary
- **FFBEngine::calculate_force**: Currently scales `genFFBTorque` by `m_wheelbase_max_nm` but fails to normalize it before applying `m_target_rim_nm` scaling. It also uses `m_steering_shaft_gain` which is shared with the legacy source.
- **Config & Preset**: Lack the `ingame_ffb_gain` parameter.
- **GuiLayer_Common**: Needs a new slider for In-Game FFB Gain.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| In-Game FFB (Structural) | Fix normalization math in `calculate_force`; Switch to dedicated gain. | Strength will be corrected to match the physical target (will feel weaker but correct). Dedicated slider added. |
| Legacy Shaft Torque | None | No change. |

## Proposed Changes

### 1. FFBEngine.h
- Add `float m_ingame_ffb_gain;` to the `FFBEngine` class.

### 2. Config.h (Preset Struct)
- Add `float ingame_ffb_gain = 1.0f;` to `Preset`.
- Add `Preset& SetInGameGain(float v)` fluent setter.
- Update `Preset::Apply` to set `engine.m_ingame_ffb_gain`.
- Update `Preset::UpdateFromEngine` to capture `engine.m_ingame_ffb_gain`.
- Update `Preset::Equals` to compare `ingame_ffb_gain`.

### 3. Config.cpp
- Update `Config::ParsePresetLine` to recognize `ingame_ffb_gain`.
- Update `Config::WritePresetFields` to save `ingame_ffb_gain`.
- Update `Config::Save` and `Config::Load` to handle the global `ingame_ffb_gain` setting.
- Update `Preset::Validate` to clamp the new value.

### 4. FFBEngine.cpp
- Update `calculate_force` for `m_torque_source == 1`:
  - Change `target_structural_mult` to `1.0 / (m_wheelbase_max_nm + 1e-9)`.
  - Use `m_ingame_ffb_gain` for `output_force` calculation when In-Game FFB is active.

### 5. GuiLayer_Common.cpp
- Add `FloatSetting("In-Game FFB Gain", &engine.m_ingame_ffb_gain, 0.0f, 2.0f, FormatPct(engine.m_ingame_ffb_gain), "Scales the native 400Hz In-Game FFB signal.");` in the "General FFB" section.
- Position it near the "Use In-Game FFB" toggle.

## Parameter Synchronization Checklist
- [ ] Declaration in `FFBEngine.h`: `float m_ingame_ffb_gain;`
- [ ] Declaration in `Preset` struct (`Config.h`): `float ingame_ffb_gain = 1.0f;`
- [ ] Entry in `Preset::Apply()`: `engine.m_ingame_ffb_gain = (std::max)(0.0f, ingame_ffb_gain);`
- [ ] Entry in `Preset::UpdateFromEngine()`: `ingame_ffb_gain = engine.m_ingame_ffb_gain;`
- [ ] Entry in `Config::Save()`: `file << "ingame_ffb_gain=" << engine.m_ingame_ffb_gain << "\n";`
- [ ] Entry in `Config::Load()`: `else if (key == "ingame_ffb_gain") engine.m_ingame_ffb_gain = std::stof(value);`
- [ ] Validation in `Preset::Validate()`: `ingame_ffb_gain = (std::max)(0.0f, ingame_ffb_gain);`

## Test Plan
- **New Test Case**: `tests/test_ffb_ingame_scaling.cpp`
  - Description: Verify that In-Game FFB output is correctly scaled by `m_target_rim_nm` and `m_ingame_ffb_gain`.
  - Expected Behavior: With `genFFBTorque = 1.0`, `target_rim = 10`, `wheelbase = 20`, `gain = 1.0`, the output should be `0.5` (10/20).
- **Manual Verification**: Build on Linux and ensure no compilation errors.
- **Regression Testing**: Run `./build/tests/run_combined_tests` to ensure no regressions.

## Deliverables
- [ ] Modified `src/FFBEngine.h`, `src/FFBEngine.cpp`
- [ ] Modified `src/Config.h`, `src/Config.cpp`
- [ ] Modified `src/GuiLayer_Common.cpp`
- [ ] New/Updated unit tests in `tests/`
- [ ] Updated `VERSION` (to v0.7.71)
- [ ] Updated `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`
- [x] Implementation Notes (to be updated upon completion)
- [x] `review_iteration_1.md` (and subsequent reviews until Greenlight)

## Implementation Notes

### Unforeseen Issues
- **EMA Convergence in Tests**: The initial test case failed because the `m_smoothed_structural_mult` takes time to converge via its 250ms EMA filter. This was resolved by using `FFBEngineTestAccess` to manually set the multiplier to the target value for the test duration.
- **Default Inversion**: The default `m_invert_force = true` caused negative output in tests, which was confusing initially. The test was updated to explicitly disable force inversion.

### Plan Deviations
- None. The implementation followed the proposed plan exactly.

### Challenges
- Identifying the precise normalization error required a deep trace of the Nm-to-DirectInput conversion pipeline. The "1.0" multiplier for In-Game FFB in the normalization stage was the root cause of the "too strong" feedback, as it bypassed the necessary division by wheelbase maximum.

### Recommendations
- Consider unifying the "Gain" terminology across the UI in the future, as "Steering Shaft Gain" and "In-Game FFB Gain" perform the same role for different sources.
