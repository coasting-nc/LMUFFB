The patch provides a comprehensive and high-quality solution to the steering wheel oscillation issue reported in GitHub issue #418. It implements a "Stationary Damping" effect that is specifically designed to eliminate oscillations when the car is stopped (including in menus and the garage) while ensuring zero interference with the Force Feedback (FFB) once the car reaches driving speeds.

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to stop steering wheel oscillations when the car is stationary by applying damping that automatically fades out as the car moves, preserving FFB purity and responsiveness.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch correctly implements the `stationary_damping_force` in the `FFBEngine`. It uses the inverse of the existing speed gate (`1.0 - ctx.speed_gate`), which mathematically guarantees that the damping is 100% effective at 0 km/h and reduces to exactly 0.0 once the car reaches the upper speed gate (default 18 km/h).
    *   **Safety & Side Effects:** The implementation is safe. It uses smoothed steering velocity and is decoupled from the driving gyro damping. Crucially, it preserves this force when the car is in a "muted" state (e.g., ESC menu or Garage), which is when these oscillations are often most intrusive. This was a specific requirement for the fix.
    *   **Completeness:** The patch is exceptionally thorough:
        *   **Physics Engine:** Updates the force calculation and summation logic.
        *   **Persistence:** Integrates the new setting into the `Preset` and `Config` systems, including parsing, saving, and clamping logic.
        *   **GUI:** Adds a slider for user adjustment and includes the force in the telemetry debug plots.
        *   **Telemetry/Logging:** Updates `FFBSnapshot` and `AsyncLogger` to include the new force component.
        *   **Verification:** Adds a new test suite (`test_issue_418_stationary_damping.cpp`) covering stationary behavior, speed fading, and menu preservation. It also updates the binary log size regression test.
        *   **Administrative:** Correctly updates `VERSION`, `CHANGELOG_DEV.md`, and includes the mandatory implementation plan and previous code review records.

3.  **Merge Assessment:**
    *   **Blocking:** None. All functional requirements and workflow constraints (versioning, changelogs, documentation) have been met.
    *   **Nitpicks:** None. The logic is sound, the math is efficient, and the testing is comprehensive.

The patch is ready for production as it solves the problem completely while adhering to the project's reliability and transparency standards.

### Final Rating: #Correct#
