# Implementation Plan - Issue #414: Fix bottoming effect falsely triggering during high-speed cornering and weaving

## Context
The "Bottoming Effect" in LMUFFB is designed to provide tactile feedback when the car's chassis scrapes the ground or hits the bump stops. It has two main detection methods: Ride Height (Method A) and Suspension Force Impulse (Method B). Additionally, there is a "Safety Trigger" that monitors raw tire load to catch extreme compressions that might be missed by the primary methods.

**Issue:** https://github.com/coasting-nc/LMUFFB/issues/414
**Title:** Fix: bottoming effect falsely triggering during high-speed cornering and weaving

> [!IMPORTANT]
> **Design Rationale: Context**
> The current 2.5x static load threshold is too low for high-downforce cars (LMH, GTE) at high speeds. Aero load alone can double the car's weight, and lateral transfer during cornering or weaving easily pushes a single tire beyond 2.5x its static weight, causing a false "bottoming" vibration that ruins the FFB feel.

## Analysis
The problem lies in `FFBEngine::calculate_suspension_bottoming` where the safety trigger is calculated:
```cpp
double bottoming_threshold = m_static_front_load * BOTTOMING_LOAD_MULT;
if (max_load > bottoming_threshold) { ... }
```
`BOTTOMING_LOAD_MULT` is currently `2.5`.

Furthermore, users have no way to disable or tune this effect because the controls are missing from the GUI, even though the `FFBEngine` already has `m_bottoming_enabled` and `m_bottoming_gain` members.

### Requirements:
1. Increase `BOTTOMING_LOAD_MULT` to `4.0`.
2. Expose `Bottoming Effect` (bool) and `Bottoming Strength` (float) in the GUI.
3. Ensure these settings are correctly saved to and loaded from the configuration file and presets.
4. Add regression tests to verify the fix and prevent regressions.

> [!IMPORTANT]
> **Design Rationale: Analysis**
> Increasing the threshold to 4.0x provides enough headroom for Aero (approx 2x) + Cornering Load Transfer (approx 1.5x) while still being sensitive enough to catch genuine bottoming impacts (which often see 5x-10x spikes). Exposing the controls empowers users to handle edge cases where even 4.0x might be problematic, or simply to tune the effect to their preference.

## Proposed Changes

### 1. FFB Engine (`src/ffb/FFBEngine.h`)
- Change `BOTTOMING_LOAD_MULT` from `2.5` to `4.0`.

### 2. GUI Layer (`src/gui/GuiLayer_Common.cpp`)
- Add `BoolSetting` for `m_bottoming_enabled`.
- Add `FloatSetting` for `m_bottoming_gain` (only visible if enabled).
- Group `Bottoming Logic` (method) under the enabled check.

### 3. Tooltips (`src/gui/Tooltips.h`)
- Add `BOTTOMING_EFFECT` and `BOTTOMING_STRENGTH` tooltips.
- Add them to the `Tooltips::All` array for the search feature.

### 4. Configuration & Presets (`src/core/Config.h`, `src/core/Config.cpp`)
- **Config.h**:
    - Add `bottoming_enabled` and `bottoming_gain` to `Preset` struct.
    - Update `Preset::SetBottoming` to accept `enabled` and `gain`.
    - Update `Preset::ApplyToEngine` and `Preset::CaptureFromEngine`.
    - Update `operator==` for `Preset`.
- **Config.cpp**:
    - Update `Config::ParsePresetLine` and `Config::ParseMainConfigLine` to handle the new keys.
    - Update `Config::SavePreset` and `Config::SaveMainConfig` to write the new keys.
    - Update all hardcoded default presets (T300, G29, etc.) to include these values.
    - Add validation in `Config::Validate` for `bottoming_gain`.

> [!IMPORTANT]
> **Design Rationale: Proposed Changes**
> The changes follow the existing pattern for effect toggles and gains in LMUFFB. By updating the `Preset` system, we ensure that the fix is backward compatible (old configs will default to enabled=true) and that users can save different bottoming settings for different cars/wheels.

## Test Plan

### Automated Regression Tests
Create `tests/test_bottoming.cpp`:
1. **`test_bottoming_high_aero_rejection`**: Simulate 3.5x static load at high speed. Verify `texture_bottoming` is 0.0.
2. **`test_bottoming_genuine_impact`**: Simulate 4.5x static load. Verify `texture_bottoming` is > 0.0.
3. **`test_bottoming_user_controls`**: Verify that setting `bottoming_enabled = false` mutes the effect even at 4.5x load, and that `bottoming_gain` scales the output linearly.

### Manual Verification (Mocks)
- Since I am on Linux, I will rely on the automated tests which use a mock `TelemInfoV01` and the real `FFBEngine` logic.

> [!IMPORTANT]
> **Design Rationale: Test Plan**
> The test plan directly addresses the reported bug (false triggers at high aero) and ensures no loss of functionality (genuine impacts still detected). It also validates the new UI/Config integration.

## Implementation Notes
- **Encountered Issues**:
    - The first code review pointed out that my initial test code used `FFBEngineTestAccess` methods that didn't exist. I corrected this by checking `test_ffb_common.h` and only using existing methods, or using public engine members where appropriate.
    - Found a duplicate `.SetBottoming` call in `Config.cpp` for one of the guide presets during the second manual check. Fixed it before final verification.
    - Had to adjust the regression test to explicitly set `m_static_front_load` and latch it, because the automatic learning logic was interfering with the instantaneous load injection in the test.
- **Deviations from the Plan**:
    - No major deviations. The plan was followed as written, with the addition of more robust verification in the regression test.
- **Suggestions for the Future**:
    - The `FFBEngineTestAccess` could be expanded to include more setters/getters for private state to avoid having to use `volatile` or public hacks in tests.

## Additional Questions
None at this stage.
