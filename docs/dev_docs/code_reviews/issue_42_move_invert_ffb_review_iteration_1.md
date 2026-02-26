This review evaluates the proposed changes for Issue #42: "move Invert FFB Signal setting out of the saved profile".

### **Analysis and Reasoning:**

1.  **User's Goal:**
    The objective is to move the "Invert FFB Signal" setting from individual tuning profiles (Presets) to a global application setting, ensuring that hardware-specific inversion remains constant when switching car profiles.

2.  **Evaluation of the Solution:**

    *   **Core Functionality:**
        *   The patch successfully removes `invert_force` from the `Preset` struct in `Config.h`.
        *   It updates `Preset::Apply`, `Preset::UpdateFromEngine`, and `Preset::Equals` to no longer handle the inversion state, which correctly isolates the engine's inversion state from preset changes.
        *   It removes the per-preset serialization logic in `Config.cpp` (`ParsePresetLine`, `WritePresetFields`, and `LoadPresets`).
        *   It ensures `FFBEngine` initializes `m_invert_force` to `true` by default.
        *   It updates several tests to accommodate the change.
    *   **Safety & Side Effects:**
        *   **Persistence Regression (Major Bug):** The patch removes the parsing of `invert_force` in `Config::Load` (line 1349 in `Config.cpp`). Since this setting is intended to be a global hardware setting, it **must** be loaded from the configuration file on startup. By removing this line without ensuring it is handled in a global settings block, the patch breaks the persistence of this setting across application restarts. The setting will now revert to the default (`true`) every time the application is launched.
    *   **Completeness:**
        *   **Missing Test Helper (Build Failure):** The implementation plan mentions adding `SetInvertForce` to `FFBEngineTestAccess` in `tests/test_ffb_common.h`. However, this file (or the modification to it) is **entirely missing** from the `current_patch`. Consequently, the new test `test_issue_42_ffb_inversion_persistence.cpp` will fail to compile, as it calls `FFBEngineTestAccess::SetInvertForce`.
        *   The patch correctly updates `VERSION` and `USER_CHANGELOG.md`.

3.  **Merge Assessment (Blocking vs. Non-Blocking):**

    *   **Blocking:**
        1.  **Build Failure:** The missing change to `tests/test_ffb_common.h` prevents the project from building.
        2.  **Functional Regression:** The removal of the loading logic in `Config::Load` breaks the persistence of the very setting that was meant to be made "global." A hardware setting that resets on every launch is a major usability regression.
    *   **Nitpicks:**
        *   The changelog date "March 5, 2026" is in the future.

### **Final Summary:**
While the architectural decision to remove the setting from the `Preset` struct is correct and the developer followed through with updating several call-sites and tests, the patch is non-functional due to a missing file/helper and introduces a significant regression in configuration persistence. It is not ready for merge.

### Final Rating: #Partially Correct#
