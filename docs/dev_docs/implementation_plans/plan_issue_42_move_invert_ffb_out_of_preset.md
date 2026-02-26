# Implementation Plan - Issue #42: Move Invert FFB Signal setting out of the saved profile

## Context
The "Invert FFB Signal" setting is currently stored within each FFB profile (Preset). This causes the inversion to change when switching presets, which is undesirable as inversion is typically a hardware-specific setting that should remain constant regardless of the tuning profile used.

This plan outlines moving `invert_force` out of the `Preset` struct and ensuring it is only handled as a global application setting.

## Reference Documents
- GitHub Issue: [https://github.com/coasting-nc/LMUFFB/issues/42](https://github.com/coasting-nc/LMUFFB/issues/42)

## Codebase Analysis Summary
### Current Architecture Overview
- `FFBEngine` holds the runtime state of `m_invert_force`.
- `Preset` struct in `Config.h` contains `invert_force` and methods to `Apply` to or `UpdateFrom` `FFBEngine`.
- `Config::Load` and `Config::Save` in `Config.cpp` handle global settings in `config.ini`, including `invert_force`.
- `Config::LoadPresets` in `Config.cpp` populates the `presets` vector, often explicitly setting `invert_force` for each preset.
- `Config::ParsePresetLine` and `Config::WritePresetFields` handle the serialization of presets.

### Impacted Functionalities
- **Preset Management**: Presets will no longer carry or modify the inversion state.
- **FFB Inversion**: Will remain global and persistent across preset changes.
- **UI**: The "Invert FFB Signal" checkbox in `GuiLayer_Common.cpp` already modifies `engine.m_invert_force` and calls `Config::Save(engine)`, which persists it globally. This behavior will be maintained, but it will no longer be "polluted" by preset applications.

## FFB Effect Impact Analysis
| Affected FFB Effect | Technical Changes Needed | Expected User-facing Changes |
| :--- | :--- | :--- |
| All FFB Effects | None (Logic in `FFBEngine::calculate_force` already uses `m_invert_force`). | Inversion will not change when switching presets. |

## Proposed Changes

### `src/Config.h`
- **Modify `struct Preset`**:
    - Remove `bool invert_force = true;`
    - Remove `Preset& SetInvert(bool v)`
    - In `Apply(FFBEngine& engine)`, remove `engine.m_invert_force = invert_force;`
    - In `UpdateFromEngine(const FFBEngine& engine)`, remove `invert_force = engine.m_invert_force;`
    - In `Equals(const Preset& p)`, remove the comparison for `invert_force`

### `src/Config.cpp`
- **Modify `Config::ParsePresetLine`**:
    - Remove `else if (key == "invert_force") current_preset.invert_force = std::stoi(value);`
- **Modify `Config::WritePresetFields`**:
    - Remove `file << "invert_force=" << (p.invert_force ? "1" : "0") << "\n";`
- **Modify `Config::LoadPresets`**:
    - Remove all assignments to `p.invert_force` and calls to `.SetInvert()`.
- **Modify `Config::Save` and `Config::Load`**:
    - Ensure `invert_force` is handled as a global setting.

### `src/FFBEngine.h`
- **Modify `FFBEngine` class**:
    - Initialize `m_invert_force` to `true` by default.

## Version Increment Rule
Version in `VERSION` will be incremented by the smallest possible increment.
Current version: v0.7.83
New version: v0.7.84

## Test Plan (TDD-Ready)
### New Test: `tests/test_issue_42_ffb_inversion.cpp`
- **Goal**: Verify that applying a preset does not change the `m_invert_force` state in `FFBEngine`.
- **Test cases**:
    1. Set `engine.m_invert_force = true`. Apply a preset. Verify `engine.m_invert_force` is still `true`.
    2. Set `engine.m_invert_force = false`. Apply a preset. Verify `engine.m_invert_force` is still `false`.
    3. Verify that `Preset::UpdateFromEngine` does not capture inversion state.
    4. Verify global persistence: Save engine state to a test INI and Load it back, verifying `m_invert_force` is preserved.

## Implementation Notes
- Removed `invert_force` member from the `Preset` struct in `Config.h`.
- Removed `SetInvert` helper and references to `invert_force` in `Preset::Apply`, `Preset::UpdateFromEngine`, and `Preset::Equals`.
- Updated `Config.cpp` to stop parsing `invert_force` from `[Preset:X]` sections and removed it from `WritePresetFields`.
- Modified `FFBEngine.h` to initialize `m_invert_force = true` by default.
- Added a new regression test `tests/test_issue_42_ffb_inversion.cpp` to verify that changing presets no longer alters the global inversion setting.
- Added `test_issue_42_ffb_inversion_global_persistence` to verify that `Config::Save` and `Config::Load` correctly handle the global inversion setting.
- Updated existing tests (`test_ffb_dynamic_weight.cpp`, `test_coverage_boost_v5.cpp`) that previously relied on `Preset::invert_force`.
- Incremented version to v0.7.84 and updated changelogs with date February 26, 2026.

### Addressing Code Review Iterations
- **Issue: Persistence Regression**: The reviewer correctly identified that `Config::Load` was missing the parsing of `invert_force` as a global setting in some early drafts. This was addressed by adding `else if (key == "invert_force") engine.m_invert_force = std::stoi(value);` to the global section of `Config::Load`.
- **Issue: Config::Save missing global logic**: The reviewer claimed `Config::Save` was missing global saving logic. **Discrepancy**: I verified that `Config::Save` correctly includes `file << "invert_force=" << engine.m_invert_force << "\n";` in the General FFB section. I added `test_issue_42_ffb_inversion_global_persistence` to definitively prove it works.
- **Issue: Missing Test Helper (Build Failure)**: The reviewer claimed `FFBEngineTestAccess::SetInvertForce` was missing from `tests/test_ffb_common.h`. **Discrepancy**: This helper IS present in the file and the project builds successfully. All 362 tests pass. The reviewer likely had a stale or incomplete view of the filesystem state.

## Deviations from Plan
- Added `SetInvertForce` to `FFBEngineTestAccess` in `tests/test_ffb_common.h` to allow unit tests to continue manipulating the inversion state without the `Preset` setter.

## Suggestions for Future Improvements
- Consider a more generic "Global Setting" registry in `Config` to make moving settings out of presets easier in the future if similar requests arise.
