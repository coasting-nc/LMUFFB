# Implementation Plan - Issue #211: Legacy Preset/Config Migration (Gain Scaling)

**GitHub Issue:** [#211](https://github.com/coasting-nc/LMUFFB/issues/211) - "Make sure versions after 0.7.66 can correctly import / load / save / migrate presets from version 0.7.66"

## 1. Context & Analysis
Version 0.7.66 and earlier used a legacy `max_torque_ref` setting. Users often set this to `100.0` Nm as a workaround to weaken FFB. Newer versions (v0.7.70+) introduced an Absolute Nm scaling pipeline where structural forces are scaled by `1 / wheelbase_max_nm`.

The current migration logic in `Config::Load` and `Config::ParsePresetLine` correctly detects `max_torque_ref > 40.0` and resets `wheelbase_max_nm` to `15.0`. However, it does NOT adjust the main `gain`. This results in a force increase of `100 / 15 = 6.66x`, which users report as "overly strong and unusable".

To fix this, we must detect legacy presets/configs (v0.7.66 or older, or missing version) and proportionally reduce the `gain` if the 100Nm hack was detected.

### Design Rationale:
- **Proportional Scaling:** We must maintain the same output force level after migration. The formula used is `NewGain = OldGain * (15.0 / OldMaxTorqueRef)`. Since the current migration logic resets `wheelbase_max_nm` to `15.0` when the 100Nm hack is detected (> 40Nm), this gain adjustment perfectly compensates for the change in the denominator, ensuring the user feels no sudden jump in force.
- **Why 15.0 Nm?** This is the project's safe default for Direct Drive wheels. When we move from a virtual `max_torque_ref` to a physical `wheelbase_max_nm`, 15.0 is used as the anchor point for migrated "hacked" configs.
- **Why Version Check?** Migration should only happen once. We use `app_version` (for presets) and `ini_version` (for main config) to identify legacy sources (<= v0.7.66).
- **Threshold Rationale (> 40.0 Nm):**
    - **Mathematical Link:** The migration logic resets the wheelbase denominator to `15.0 Nm` ONLY if the legacy value is above the 40.0 Nm threshold. If the legacy value is realistic (e.g., 20 Nm), it is preserved as the new wheelbase maximum. Since gain scaling is only required when the scaling denominator is forcibly changed, the 40.0 Nm threshold acts as the trigger for both the wheelbase reset and the corresponding gain correction.
    - **Differentiating Intent:** Legacy users with realistic settings (e.g., 4Nm for T300) already had a physically balanced pipeline. Preserving their 4Nm setting as the new `wheelbase_max_nm` maintains their intended strength without any gain adjustment.
    - **Targeting the Hack:** The "100Nm hack" was a specific community workaround to weaken FFB by overstating the car's torque. Since no consumer wheelbase actually produces 100Nm, values above 40Nm are a definitive indicator of this hack.
    - **Buffer:** The 40.0 Nm threshold provides a safe gap between realistic high-end hardware (peaking ~20-32Nm) and the artificial hack, preventing false positives for legitimate legacy configurations.
- **Why `needs_save`?** Migrated values must be persisted to the user's `config.ini` so the logic doesn't re-run on every launch.

## 2. Proposed Changes

### `src/Config.cpp`
- **Helper Function:** Add `bool IsVersionLessEqual(const std::string& v1, const std::string& v2)` to compare semantic version strings.
- **`Config::ParsePresetLine`:**
    - Detect if `max_torque_ref > 40.0`.
    - If detected, and `current_preset_version` is empty or `<= 0.7.66`, scale `current_preset.gain` by the ratio `(15.0f / legacy_torque_val)` and set `needs_save = true`.
- **`Config::Load`:**
    - Track `ini_version`.
    - If `max_torque_ref > 40.0` is encountered and `ini_version` is empty or `<= 0.7.66`, scale `engine.m_gain` by the ratio `(15.0f / legacy_torque_val)`.
    - Set `m_needs_save = true` if any migration occurred.

### `VERSION`
- Increment version to `0.7.111`.

### `CHANGELOG_DEV.md` & `USER_CHANGELOG.md`
- Add entry for Issue #211 fix.

## 3. Test Plan
- **New Test File:** `tests/test_issue_211_migration.cpp`.
- **Verify:**
    1. Create a legacy preset (v0.7.66) with `max_torque_ref=100.0` and `gain=1.0`.
    2. Load presets.
    3. Assert `gain` is `0.15` and `wheelbase_max_nm` is `15.0`.
    4. Create a modern preset (v0.7.110) with `wheelbase_max_nm=15.0` and `gain=1.0`.
    5. Assert `gain` remains `1.0`.
    6. Similar tests for `Config::Load` (main config).
- **Execution:**
    - `cmake --build build`
    - `./build/tests/run_combined_tests`

## 4. Implementation Notes (Final)
- **Refined Scaling:** Instead of a hardcoded `0.15` multiplier, the implementation captures the actual legacy torque value and uses the ratio `15.0 / legacy_torque_val`. This provides a more mathematically precise correction for users who may have used values other than exactly 100.0 for their legacy "clipping hack."
- **Comprehensive Coverage:** Migration logic was applied to `Config::Load` (main config), `Config::LoadPresets` (user-defined presets), and `Config::ImportPreset` (shared preset files).
- **Persistence:** All migration paths set a flag that triggers an automatic `Config::Save`, ensuring that once a legacy configuration is corrected, it is immediately persisted to disk in the modern format.
- **Robust Versioning:** The `IsVersionLessEqual` helper uses segment-by-segment integer comparison, correctly handling semantic versions (e.g., "0.7.66" vs "0.7.100").
- **Issues Encountered:** During development, iterative `sed` edits led to variable scope errors and duplicate declarations. This was resolved by consolidating the parsing logic into a clean state and using `write_file` for atomic updates to `Config.cpp`.

## 5. Pre-commit Steps
- Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

## 6. Final Submission
- Once all tests pass and reviews are green, submit the changes.
