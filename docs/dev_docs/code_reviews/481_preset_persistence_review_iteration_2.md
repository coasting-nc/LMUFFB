**Analysis and Reasoning:**

1.  **User's Goal:** The objective is to ensure that the user's selected force feedback (FFB) preset and normalization state (peak torque/load) are persisted across application restarts and correctly reflected in the UI.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:**
        *   **UI Synchronization:** The patch correctly moves the UI index initialization logic for `selected_preset` to the start of the `DrawTuningWindow` function. This ensures that the UI index is synchronized with `Config::m_last_preset_name` on the first possible frame, solving the issue where synchronization failed if the "Presets and Configuration" section was collapsed on startup.
        *   **Normalization State Persistence:** The patch adds `session_peak_torque` and `auto_peak_front_load` to the configuration serialization (`Save`/`Load`). This is a critical reliability improvement that prevents sudden FFB surges by maintaining session-learned peaks across restarts.
        *   **Regression Test:** A new test (`tests/repro_issue_481.cpp`) verifies the configuration persistence and UI matching logic, ensuring the fix is verified and protected against regressions.
    *   **Safety & Side Effects:**
        *   **Encapsulation:** The patch uses `friend class Config;` in `FFBEngine`, which is the correct way to allow the configuration system to serialize private members without exposing them publicly.
        *   **Input Validation:** The code includes `std::isfinite` and range checks (e.g., `*val > 0.01`, `*val > 10.0`) when loading normalization peaks, ensuring hardware safety against corrupted config files.
        *   **Workflow Compliance:** The patch correctly increments the `VERSION`, updates `CHANGELOG_DEV.md`, and includes the required implementation plan and review records.
    *   **Completeness:** All necessary call-sites and configurations are updated. The developer correctly reverted unrelated changes (Issue #475) to maintain a single-issue focus as requested in the constraints.

3.  **Merge Assessment (Blocking vs. Non-Blocking):**
    *   **Blocking:** None. The patch addresses all requirements, follows the mandatory workflow, and provides a safe, maintainable fix.
    *   **Nitpicks:** None. The use of `static` state in `DrawTuningWindow` is consistent with the project's existing ImGui implementation style.

**Final Rating:**

### Final Rating: #Correct#
