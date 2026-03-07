# Implementation Plan - Issue #37: SoP Smoothing Latency Fix & Reset

This plan addresses Issue #37 where the SoP Smoothing slider has inverted behavior (0 = max latency, 1 = raw) and needs to be reset to 0 (raw) for all presets.

## User Review Required

> [!IMPORTANT]
> This change will reset "SoP Smoothing" to 0.0 for ALL user presets and built-in presets. The slider behavior will also be inverted: 0.0 will now mean "Raw" (0ms latency) and 1.0 will mean "Max Smoothing" (100ms latency).

- None at this time.

## Proposed Changes

### 1. FFB Engine Logic

#### [src/FFBEngine.cpp]
- Change the mapping of `m_sop_smoothing_factor` to `smoothness`.
- Current: `double smoothness = 1.0 - (double)m_sop_smoothing_factor;`
- New: `double smoothness = (double)m_sop_smoothing_factor;`

> [!IMPORTANT]
> **Design Rationale**: Aligning the slider value with the "amount of smoothing" applied. 0.0 should naturally mean no smoothing (Raw).

### 2. GUI Layer

#### [src/GuiLayer_Common.cpp]
- Update the latency calculation for display.
- Current: `int ms = (int)std::lround((1.0f - engine.m_sop_smoothing_factor) * 100.0f);`
- New: `int ms = (int)std::lround(engine.m_sop_smoothing_factor * 100.0f);`
- Update the format string if necessary (though "%.2f" is generic).

> [!IMPORTANT]
> **Design Rationale**: Keeping the UI display consistent with the internal logic change.

### 3. Config & Presets

#### [src/Config.h]
- Change the default value for `sop_smoothing` in the `Preset` struct from `1.0f` to `0.0f`.

#### [src/Config.cpp]
- Update all built-in presets to set `sop_smoothing = 0.0f;`.
- This includes `T300`, `GT3 DD 15 Nm`, `LMPx/HY DD 15 Nm`, and all test/guide presets.

> [!IMPORTANT]
> **Design Rationale**: Requirement states all default profiles/presets should be set to 0 SoP smoothing.

### 4. Migration Logic

#### [src/Config.cpp]
- In `Config::Load` and `Config::ParsePresetLine`, add logic to detect if we are loading a config from a version prior to this fix.
- If the version is less than the new version (0.7.147), reset `sop_smoothing` to `0.0f`.
- We can use `IsVersionLessEqual` helper.

> [!IMPORTANT]
> **Design Rationale**: Requirement states we should reset user saved profiles to 0 SoP smoothing upon loading from a previous version.

### 5. Documentation & Versioning

#### [VERSION]
- Increment version to `0.7.147`.

#### [USER_CHANGELOG.md]
- Add a clear note about SoP Smoothing reset and inversion.

## Test Plan

### Automated Tests
- Update `tests/test_ffb_logic.cpp`: `test_latency_display_regression` to verify the new mapping.
- Add a new test case for migration logic in `tests/test_ffb_logic.cpp` (mocking an old config file).

### Manual Verification (Simulated)
- Verify `FFBEngine` tau calculation with 0.0 and 1.0 values.
- Verify `Config::Load` behavior with a mock legacy config.

## Implementation Notes

### Discrepancies/Issues
- During testing, it was discovered that some automated tests relied on `sop_smoothing_factor = 1.0` being "Raw". These were updated to `0.0`.
- Building on Linux required manually downloading the `lz4.c/h` vendor files as they are ignored by git but required by CMake.
- Some tests in `test_ffb_logic.cpp` and `test_versioned_presets.cpp` had to be adjusted to account for the migration logic (e.g., using version `0.7.147` to avoid auto-reset).

### Code Review Feedback
- **Iteration 1**: Status #Mostly Correct#.
    - Issue: Missing QA records in `docs/dev_docs/code_reviews`. Fixed by adding `issue_37_review_iteration_1.md`.
    - Issue: Incomplete implementation notes. Fixed by updating this section.
    - Issue: Temporary log files (`test_refactoring_noduplicate.log`, `test_refactoring_sme_names.log`) included in root. Fixed by deleting them.
    - Issue: Test logic change in `test_preset_version_persistence`. Fixed by using a hardcoded version `0.7.147` instead of `LMUFFB_VERSION` to maintain test intent while avoiding migration.
