The proposed code change is a high-quality, comprehensive fix for Issue #309, which also incorporates necessary architectural improvements for lateral load calculations (Issue #306).

### Analysis and Reasoning:

1.  **User's Goal:** The objective was to fix the fallback hierarchy in `calculate_sop_lateral` to prioritize suspension-based load approximation over pure kinematic estimation, and optionally (implied by the "like we do in `calculate_force`" comment) modernize the calculation to use a 4-wheel model with correct sign convention.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch correctly implements the requested hierarchy: it checks for primary telemetry (`mTireLoad`), and if missing, it attempts `approximate_load` (suspension-derived) before falling back to `calculate_kinematic_load`. This ensures the highest fidelity data is used for encrypted DLC or cases where certain telemetry is blocked.
    *   **4-Wheel Model & Sign Convention:** The developer correctly expanded the calculation to include all four wheels (`FL, FR, RL, RR`), which provides a more stable "global chassis" feel. Crucially, the sign convention was updated to `Left - Right`. Given LMU's coordinate system (+X = Left), a right turn results in positive centrifugal force and positive load shift to the outside (left) tires. Making both positive ensures the haptic feedback is additive, which is physically intuitive for Seat-of-the-Pants effects.
    *   **Safety & Side Effects:** The logic includes safety clamps and checks against division by zero (checking `total_load > 1.0`). The modifications to existing tests correctly account for the intentional sign flip and the transition to a 4-wheel model, ensuring no regressions in orientation logic.
    *   **Completeness:** The patch is exceptionally complete. It includes:
        *   A detailed implementation plan with design rationales.
        *   Updates to `VERSION` and both developer and user-facing changelogs.
        *   A new, robust test suite (`test_issue_309_fallback.cpp`) covering all fallback scenarios and the 4-wheel contribution.
        *   Updates to existing lateral load tests to maintain CI/CD integrity.

3.  **Merge Assessment:**
    *   **Blocking:** None.
    *   **Nitpicks:** None.

The solution is well-architected, thoroughly tested, and adheres to the project's reliability standards.

### Final Rating: #Correct#
