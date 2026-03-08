**Analysis and Reasoning:**

1.  **User's Goal:** The user wants to refine the Force Feedback (FFB) gating logic to prevent "punches" (spikes) when transitioning out of driving (e.g., to AI control or menus), while ensuring that the "Soft Lock" effect remains active in the garage and during pause for hardware safety.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch correctly implements the requested refinement. By changing the gate from `!is_driving` (which was too broad and disabled FFB in the garage) to `scoring.mControl != 0`, the system correctly identifies when the player is no longer in control of the vehicle (e.g., AI takeover, Replay, or Main Menu). In these cases, it targets zero force, which is then smoothly slewed via the existing safety layer. When the player is in the garage or paused, `mControl` remains `0` (Player control), allowing Soft Lock to persist and protect the steering rack from hitting physical stops.
    *   **Safety & Side Effects:** The solution is safe as it utilizes the existing `ApplySafetySlew` rate limiter to ensure that the transition to zero force is never instantaneous, preventing mechanical "clunks." It also restores a safety feature (Soft Lock in the garage) that was inadvertently disabled in a previous version. No security risks or unrelated logic changes were introduced.
    *   **Completeness:** The patch is comprehensive. It updates the main logic loop in `src/main.cpp`, provides an updated unit test in `tests/test_issue_281_spikes.cpp` to verify both the garage scenario and the AI takeover scenario, and correctly increments the version and updates the changelog and implementation plan.

3.  **Merge Assessment (Blocking vs. Nitpicks):**
    *   There are no blocking issues. The implementation perfectly aligns with the project's reliability standards and the specific technical direction provided in the issue's discussion history.
    *   The test changes (adding `SetLastOutputForce` helper) are appropriate for ensuring the fix works as expected during transitions.

### Final Rating: #Correct#
