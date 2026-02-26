The proposed patch successfully addresses the structural requirements of moving the "Invert FFB Signal" setting out of individual presets and into the global configuration. It correctly modifies the `Preset` struct, updates the `Config::Load` logic to handle the setting globally, and removes it from per-preset serialization. It also includes a comprehensive new regression test and updates existing tests.

However, the patch is **not commit-ready** due to two significant blocking issues:

1.  **Missing `Config::Save` Logic (Functional Regression):** Despite explicit instructions from the user to update the `Config::Save` function to handle the setting globally, the patch does not contain any modifications to `Config::Save`. While it correctly removes the setting from `WritePresetFields` (per-preset saving), it fails to add the corresponding `file << "invert_force=" ...` logic to the global section of `Config::Save`. This means that while the setting can be loaded if manually added to the config file, any changes made via the UI will not be persisted to disk, and the setting will be lost upon application restart.
2.  **Missing Test Helper (Build Failure):** The new test `test_issue_42_ffb_inversion.cpp` relies on `FFBEngineTestAccess::SetInvertForce`. Although the developer claims in the implementation notes that this helper is "already present" or was added as a "deviation," the required changes to `tests/test_ffb_common.h` are missing from the patch. Given that a previous code review (included in the patch) specifically flagged this as a cause for build failure, and the developer did not include the file in this diff, the project will fail to compile in a clean environment.

**Blocking Issues:**
*   **Functionality:** Persistence of the global `invert_force` setting is broken because `Config::Save` was not updated in the patch.
*   **Build:** Missing definition for `FFBEngineTestAccess::SetInvertForce` in `tests/test_ffb_common.h` will lead to a compilation error.

**Nitpicks:**
*   The implementation notes mention a discrepancy with the reviewer but the developer failed to provide the code (the diff hunk) that would resolve the reviewer's concern in the final patch.

### Final Rating: #Partially Correct#
