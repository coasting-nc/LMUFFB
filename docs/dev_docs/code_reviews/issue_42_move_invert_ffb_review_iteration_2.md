The proposed patch effectively addresses the core requirement of Issue #42: moving the "Invert FFB Signal" setting out of individual tuning presets and into a global application setting. This ensures that hardware-specific inversion preferences remain constant when switching between different car profiles.

### **Analysis and Reasoning:**

1.  **User's Goal:**
    To make the "Invert FFB Signal" setting a global application preference rather than a per-preset setting.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:**
        *   **Removal from Preset:** The patch correctly removes the `invert_force` member from the `Preset` struct in `Config.h`.
        *   **Decoupling:** It updates `Preset::Apply`, `Preset::UpdateFromEngine`, and `Preset::Equals` to ensure that applying or updating a preset no longer affects the engine's inversion state.
        *   **Persistence:** It correctly updates `Config::Load` in `Config.cpp` to treat `invert_force` as a global setting (moving it to the block containing other global settings like `log_path` and `show_graphs`). It also removes it from the per-preset serialization logic (`ParsePresetLine` and `WritePresetFields`).
        *   **Defaults:** It adds a default value (`true`) to `m_invert_force` in `FFBEngine.h`.
    *   **Safety & Side Effects:**
        *   The logic for handling the inversion in the physics calculation (`FFBEngine::calculate_force`) remains unchanged, which is correct as it already used the engine's runtime state.
        *   The patch includes necessary updates to existing tests (`test_coverage_boost_v5.cpp`, `test_ffb_dynamic_weight.cpp`) to accommodate the removal of the member from the `Preset` struct.
    *   **Completeness:**
        *   **Build Failure (Blocking):** The patch is incomplete regarding the test infrastructure. A new test file `tests/test_issue_42_ffb_inversion.cpp` is added, which relies on a helper method `FFBEngineTestAccess::SetInvertForce`. However, the modification to `tests/test_ffb_common.h` (where this helper should be defined) is **missing** from the patch. Even though the developer claims in the implementation notes that the helper is present, it is not included in the provided diff. Consequently, the project will fail to compile.
        *   **Documentation:** The patch correctly updates `VERSION`, `CHANGELOG_DEV.md`, and `USER_CHANGELOG.md`.

3.  **Merge Assessment (Blocking vs. Non-Blocking):**
    *   **Blocking:** The project will fail to compile because the new test references a symbol (`FFBEngineTestAccess::SetInvertForce`) that is not defined in the included changes. The developer's implementation notes acknowledge a discrepancy regarding this file, but the patch itself remains broken.
    *   **Nitpicks:** There is a minor date inconsistency between the dev changelog (March 5) and the user changelog (March 2).

### **Final Summary:**
The architectural changes are correct and solve the user's problem logically. However, the patch is not commit-ready because it introduces a build failure in the test suite due to a missing header file modification.

### Final Rating: #Partially Correct#
