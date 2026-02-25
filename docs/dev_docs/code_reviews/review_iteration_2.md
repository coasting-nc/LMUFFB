The proposed code change is a high-quality, comprehensive fix for Issues #180 and #175. It successfully restores manual torque scaling for the 100Hz FFB source by providing a user-controlled toggle to disable dynamic normalization.

### Analysis and Reasoning:

1.  **User's Goal:** The objective was to restore the "predictable" behavior of the legacy fixed torque reference for the 100Hz Steering Shaft Torque source, allowing users to opt out of the dynamic (session-learned) normalization that could cause "limp" steering after torque spikes.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch correctly implements the `m_dynamic_normalization_enabled` toggle across the `FFBEngine`, `Config`, and `Preset` systems. The logic in `calculate_force` is updated to gate the peak learner and switch the structural scaling reference to `m_wheelbase_max_nm` (the logical successor to the legacy `max_torque_ref`) when the toggle is disabled.
    *   **Safety & Side Effects:**
        *   **Thread Safety:** The patch specifically uses `std::lock_guard<std::recursive_mutex> lock(g_engine_mutex)` when updating the toggle from the GUI, which is a critical requirement for this project to prevent race conditions with the high-frequency FFB thread.
        *   **Robustness:** The implementation includes division-by-zero protection (`+ 1e-9`) and maintains existing behavior for the 400Hz source and for users who keep the default settings.
    *   **Completeness:** The solution is exceptionally complete. It includes:
        *   Persistence (loading/saving settings).
        *   UI integration (checkbox with a clear tooltip).
        *   Detailed documentation (Developer and User Changelogs, Version bump).
        *   A new regression test (`test_ffb_normalization_toggle.cpp`) that verifies the peak learner behavior and the final force output for both toggle states.
        *   Updated implementation plans and review records.

3.  **Merge Assessment:**
    *   **Blocking:** None. The solution is functional, safe, and follows the project's architectural patterns.
    *   **Nitpicks:** None. The agent successfully addressed previous iteration feedback (as evidenced by the inclusion of the mutex and documentation which were highlighted as missing in the `review_iteration_1.md` log included in the patch).

The patch is of production quality and ready for merge.

### Final Rating: #Correct#
