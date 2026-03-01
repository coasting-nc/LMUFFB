# Implementation Plan - Issue #207: Disable by default the Session-Learned Dynamic Normalization

## Context
Issue #207 requests that dynamic normalization be disabled by default to ensure a consistent and predictable FFB feel. LMUFFB implements normalization in two stages:
1. **Structural Normalization** (Stage 1, Issue #152): Scales Steering, SoP, and other physical forces based on learned session torque peaks.
2. **Tactile Normalization** (Stage 3, Issue #154): Scales Road, Slide, and Lockup haptics based on learned session tire load peaks.

To provide maximum flexibility and address the inconsistency concerns, both systems will be disabled by default and controllable via separate UI toggles.

Additionally, this update resolves a critical "cross-pollination" issue where peaks learned for one vehicle (e.g., a high-torque Hypercar) could persist when switching to a different vehicle (e.g., a GT3), leading to incorrectly scaled FFB until the new peak was eventually learned. This is now handled by an explicit reset trigger on car change.

## Reference Documents
- GitHub Issue #207: [Disable by default the Session-Learned Dynamic Normalization](https://github.com/coasting-nc/LMUFFB/issues/207)
- GitHub Issue #152: Stage 1 Structural Normalization.
- GitHub Issue #154: Stage 3 Tactile Normalization.

## Codebase Analysis Summary
- **FFBEngine.h**:
  - `m_dynamic_normalization_enabled`: Controls Stage 1 (Structural).
  - `m_auto_load_normalization_enabled`: Controls Stage 3 (Tactile).
- **FFBEngine.cpp**:
  - `calculate_force()`: Peak followers and scaling selection.
  - `ResetNormalization()`: New method to restore baseline peaks when disabling toggles or changing cars.
- **GripLoadEstimation.cpp**:
  - `update_static_load_reference()`: Guarded by `m_auto_load_normalization_enabled`.
  - `InitializeLoadReference()`: Reset normalization on car change.
- **Config.h / Config.cpp**:
  - Support for persisting both flags in `Preset` and global settings.
- **GuiLayer_Common.cpp**:
  - Independent checkboxes for both features.

## FFB Effect Impact Analysis
| Effect | Technical Changes | Expected User-Facing Change |
| :--- | :--- | :--- |
| **Structural Forces** | Disabled by default. `m_session_peak_torque` held at `m_target_rim_nm`. | FFB strength remains absolute and manually adjustable. No "softening" after spikes. |
| **Tactile Haptics** | Disabled by default. `m_auto_peak_load` held at class-default seed. | Haptic intensity (Road, ABS, etc.) remains consistent based on car class and manual sliders. |

## Proposed Changes

### 1. FFBEngine.h
- Initialize `m_dynamic_normalization_enabled` and `m_auto_load_normalization_enabled` to `false`.
- Declare `ResetNormalization()`.

### 2. FFBEngine.cpp
- Implement `ResetNormalization()`: restores peaks to `m_target_rim_nm` and class-default seed loads.
- Guard peak followers and multiplier updates with their respective enablement flags.
- Update `InitializeLoadReference` (in `GripLoadEstimation.cpp`) to call `ResetNormalization` on car change. This ensures that a new session with a different vehicle always starts from a clean baseline, preventing "learned" peaks from one car from polluting another.

### 3. Config.h / Config.cpp
- Add both flags to `Preset` struct.
- Update `Apply()`, `UpdateFromEngine()`, `Equals()`.
- Update parsing and saving logic.

### 4. GuiLayer_Common.cpp / Tooltips.h
- Add "Enable Dynamic Normalization (Session Peak)" in General FFB.
- Add "Enable Dynamic Load Normalization" in Tactile Textures.
- Logic: When changed to `false`, call `engine.ResetNormalization()`.

## Parameter Synchronization Checklist
- [x] Declaration in `FFBEngine.h`
- [x] Declaration in `Preset` struct (`Config.h`)
- [x] Entry in `Preset::Apply()`
- [x] Entry in `Preset::UpdateFromEngine()`
- [x] Entry in `Config::Save()`
- [x] Entry in `Config::Load()`

## Test Plan
- **Toggle State Verification**: Verify default is `false`.
- **Reset Logic Verification**: Verify that enabling, driving (to change peak), then disabling returns the engine to baseline values, not the learned peak.
- **Shadow Regression**: For tests that verify normalization (e.g. `test_peak_hold_adaptation`), add companion tests (e.g. `test_peak_hold_adaptation_disabled`) that verify peaks remain static when the feature is off.
- **Full Suite Verification**: Run all tests to ensure `target_rim_nm` and class seeds are used correctly when normalization is off.

## Deliverables
- [x] Modified `src/FFBEngine.h`, `src/FFBEngine.cpp`
- [x] Modified `src/Config.h`, `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `src/Tooltips.h`
- [x] Modified `VERSION`, `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`
- [x] New/Updated tests.
- [x] Implementation Notes (to be added at the end).

## Implementation Notes

### Unforeseen Issues
- Initial tests failed because `InitializeEngine` was forcing a 20.0 Nm peak for legacy support. Added `ResetNormalization` and updated `InitializeEngine` to ensure tests start from a predictable default state that matches the new toggles.
- Redundant string parsing in `calculate_force` was identified during code review. Fixed by moving `m_auto_peak_load` reset to `InitializeLoadReference` which runs only when the car changes.

### Plan Deviations
- **Two Toggles**: The original request only mentioned "Dynamic Normalization", but the app uses it in two places: Structural (Stage 1) and Tactile (Stage 3). To be thorough and ensure full consistency as requested by the user's second prompt, I implemented independent toggles for both.
- **Mid-Session Reset**: Added a dedicated `ResetNormalization` method to ensure that toggling the features OFF mid-session (or switching cars) immediately restores the manual baseline.

### Challenges Encountered
- **Multiplier Smoothing**: The structural multiplier uses a 250ms EMA. Tests needed to run for ~1 second (400 frames) to reliably settle on the new scaling value after a reset or toggle.

### Recommendations for Future Plans
- When a feature has multiple "stages" or affects different signal paths (e.g. physics vs textures), explicitly identify all affected toggles in the architectural phase.
