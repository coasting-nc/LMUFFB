# Code Review - Issue #281: Fix FFB Spikes on Driving State Transition

The proposed patch effectively addresses the issue of force feedback (FFB) spikes when transitioning the driving state (e.g., pausing the game).

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to ensure that when a player stops actively driving (pausing or entering menus), any active FFB forces (especially safety forces like Soft Lock) ramp down smoothly to zero instead of cutting off abruptly, which causes a mechanical "clunk" or spike.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch achieves the goal by explicitly setting the target physics force to `0.0` whenever `is_driving` is false. By placing this logic before the `ApplySafetySlew` call, it leverages the project's existing slew rate limiter to transition the current force to zero over several frames. This prevents the immediate jump from a high force (like a Soft Lock at maximum steering angle) to zero.
    *   **Safety & Side Effects:** The change is safe. It is localized to the FFB calculation loop. It correctly preserves Soft Lock functionality in the garage (where `is_driving` is true but `full_allowed` might be false), while ensuring zero force in menus/pause (where `is_driving` is false). No new security risks or global state issues are introduced.
    *   **Completeness:** The solution is complete. It includes the logic fix in the main thread loop, a comprehensive regression test that simulates the specific scenario (Soft Lock active while transitioning to a paused state), and the necessary updates to the changelog and version file.

3.  **Merge Assessment:**
    *   **Blocking:** None.
    *   **Nitpicks:** None. The implementation is concise and follows the existing architectural patterns of the project (using the safety layer for transitions).

### Final Rating: #Correct#
