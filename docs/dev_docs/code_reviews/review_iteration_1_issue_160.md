The proposed code change addresses the issue where the In-Game FFB (400Hz native signal) was excessively strong and lacked independent adjustability. The solution is comprehensive, including engine logic corrections, configuration persistence, UI enhancements, and unit testing.

### Analysis and Reasoning:

1.  **User's Goal:** Correct the excessively strong In-Game FFB output and provide a dedicated gain slider for independent adjustment.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:**
        *   **Math Fix:** The patch identifies a critical normalization error. Previously, the In-Game FFB signal (which is already normalized to `[-1, 1]` by the game) was treated as a raw torque value without being normalized against the wheelbase's maximum torque before applying the user's "Target Rim Torque" scaling. This resulted in an output that was typically 10x-20x too strong. The fix correctly uses `1.0 / (m_wheelbase_max_nm)` as the normalization factor for this source.
        *   **Adjustability:** A new `m_ingame_ffb_gain` parameter is introduced, allowing users to tune the native signal strength without affecting the legacy "Steering Shaft Gain".
    *   **Safety & Side Effects:**
        *   The changes are well-isolated to the `m_torque_source == 1` path. The default/legacy logic remains intact.
        *   Input validation and clamping are applied to the new parameter in all relevant places (Config load, Preset apply, Preset validate).
        *   The UI change is "context-aware," showing only the relevant gain slider based on the selected torque source, which prevents UI bloat and user confusion.
    *   **Completeness:**
        *   The parameter is fully integrated into the `Preset` system (saving, loading, fluent setters, comparison).
        *   The global configuration handling in `Config.cpp` is updated.
        *   The `FFBEngine` logic is updated to use the correct gain based on the active source.
        *   Documentation is updated in both developer and user changelogs.
        *   A new test suite (`test_ffb_ingame_scaling.cpp`) specifically verifies the math fix and the independent gain control.

3.  **Merge Assessment:**
    *   **Blocking:** None. The solution is robust and follows the existing architecture of the project.
    *   **Nitpicks:** None. The implementation is remarkably thorough, even including a fluent setter and proper BBCode formatting in the user changelog.

The patch is of high quality, addresses the root cause of the physics issue, and provides the requested feature with no regressions.

### Final Rating: #Correct#
