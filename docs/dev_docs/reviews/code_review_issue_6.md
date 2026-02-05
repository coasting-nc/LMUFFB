# Code Review Report - Preset Handling Improvements

**Task ID:** Issue #6
**Review Date:** 2026-02-06
**Branch:** fix/preset-handling-improvements-15068727691974390498
**Commit:** bf59b1d4a415e48c8a5cdf1460902cd13b4f3ab5

## 1. Summary
This review covers the implementation of preset handling improvements, including:
- Persistence of the last used preset.
- Dirty state indication (*) in the UI.
- "Save Current Config" behavior updates.
- New "Delete" and "Duplicate" reset functionality.
- Associated tests and version increment.

## 2. Findings

### Critical Issues 🛑
1.  **Data Loss / Configuration Reset in `DeletePreset`**
    - **File:** `src/Config.cpp`
    - **Method:** `Config::DeletePreset(int index)`
    - **Description:** The function creates a default-constructed `FFBEngine` object (`FFBEngine temp;`) and passes it to `Config::Save(temp)`.
    - **Impact:** `Config::Save` writes the *entire* `config.ini` file, including global settings (Gain, Min Force, Effect Toggles, etc.). Because `temp` is initialized with default values (e.g., Gain=1.0), calling `Save(temp)` will **overwrite the user's current global FFB settings with factory defaults**. This results in data loss for the user whenever they delete a preset.
    - **Recommendation:** Modify `DeletePreset` to accept the current `FFBEngine& engine` as a parameter (similar to `DuplicatePreset`) and pass the active engine state from `GuiLayer`.

### Major Issues ⚠️
1.  **Missing Test Coverage for Global Config Preservation**
    - **File:** `tests/test_preset_improvements.cpp`
    - **Test:** `test_delete_user_preset`
    - **Description:** The test verifies that the preset is removed from the vector but does not check if other configuration values in the saved file are preserved. It would pass despite the critical bug identified above.
    - **Recommendation:** Update the test to:
        1. Set a non-default global value (e.g., `engine.m_gain = 0.5f`).
        2. Save the config.
        3. Delete a preset.
        4. Reload the config and assert that `m_gain` is still 0.5f.

### Minor Issues ℹ️
1.  **Maintenance Burden in `IsEngineDirtyRelativeToPreset`**
    - **File:** `src/Config.cpp`
    - **Description:** The dirty check manually compares over 40 individual fields. This is prone to drift; if a developer adds a new FFB parameter but forgets to update this function, the dirty flag logic will be incomplete.
    - **Recommendation:** Add a prominent comment at the top of `Config::IsEngineDirtyRelativeToPreset` (and near the `FFBEngine` class definition) warning developers to update this function when adding new parameters.

2.  **Redundant `Save` Call Logic**
    - **File:** `src/Config.cpp`
    - **Method:** `AddUserPreset`
    - **Description:** `AddUserPreset` calls `Save(engine)`. `ApplyPreset` also calls `Save(engine)`. This is generally fine but ensures strict io syncing. No action needed, just noted.

## 3. Checklist Results

### Functional Correctness
*   **Plan Adherence:** ✅ Yes (Mostly, except for the bug logic)
*   **Completeness:** ✅ Yes
*   **Logic:** ❌ FAIL (DeletePreset logic is flawed)

### Implementation Quality
*   **Clarity:** ✅ Good
*   **Simplicity:** ✅ Good
*   **Robustness:** ❌ FAIL (Data loss scenario)
*   **Performance:** ✅ Good (Dirty check is efficient enough)
*   **Maintainability:** ⚠️ Warning (Manual field comparison)

### Code Style & Consistency
*   **Style:** ✅ Good
*   **Consistency:** ✅ Good
*   **Constants:** ✅ Good

### Testing
*   **Test Coverage:** ⚠️ Partial (Missing regression test for config integrity)
*   **TDD Compliance:** ✅ Tests were written
*   **Test Quality:** ⚠️ Tests missed the side-effect bug

### Configuration & Settings
*   **User Settings:** ✅ Presets updated correctly
*   **Migration:** ✅ Logic exists
*   **New Parameters:** ✅ `m_last_preset_name` handle correctly

### Versioning & Documentation
*   **Version Increment:** ✅ 0.7.13 -> 0.7.14
*   **Documentation:** ✅ Plan updated
*   **Changelog:** ✅ Updated

### Safety & Integrity
*   **Unintended Deletions:** ✅ None found in code (logic preserves built-ins)
*   **Security:** ✅ N/A
*   **Resource Management:** ✅ Safe

### Build Verification
*   **Compilation:** ✅ PASS
*   **Tests Pass:** ✅ PASS

## 4. Verdict

**Outcome:** **FAIL**

**Justification:**
The implementation of `DeletePreset` introduces a critical regression that wipes out the user's global FFB configuration settings logic by overwriting them with defaults during the save operation. This must be fixed before merging.
