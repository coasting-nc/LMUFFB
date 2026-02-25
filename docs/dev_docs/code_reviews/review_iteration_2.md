The proposed patch effectively addresses the reported issue where the steering "soft lock" was perceived as too weak while driving. The root cause—normalization of an absolute physical force (soft lock) by the dynamic session peak torque—is correctly identified and resolved.

### User's Goal
The goal is to ensure the steering soft lock (physical stop) remains consistently strong and independent of the dynamic force normalization that occurs during a driving session.

### Evaluation of the Solution

#### Core Functionality
The patch correctly moves the `soft_lock_force` from the `structural_sum` (which undergoes dynamic normalization based on learned session peaks) to the `texture_sum_nm` group. In this project's architecture, the texture group is scaled by the wheelbase's maximum torque rather than the session's peak, ensuring that the soft lock always provides the same physical resistance regardless of driving intensity.

#### Safety & Side Effects
*   **Regressions:** No regressions are introduced. The summation logic in `FFBEngine::calculate_force` remains mathematically sound.
*   **Security:** No security vulnerabilities (secrets, injection, etc.) were introduced.
*   **Thread Safety:** The modification occurs within a method already protected by the `g_engine_mutex`, maintaining thread-safe access to the FFB state.
*   **Physics:** The use of absolute Nm scaling for a physical limit like a soft lock is the correct physical model.

#### Completeness
The patch is highly complete:
*   It includes a robust regression test (`tests/test_issue_181_soft_lock_weakness.cpp`) that explicitly verifies the force remains constant across different session peak torque values.
*   It updates versioning in both `VERSION` and a new `src/Version.h` (to provide a C++ macro for the version).
*   It provides comprehensive documentation updates in `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`, and a detailed `implementation_plan.md`.

### Merge Assessment

**Nitpicks:**
*   **Changelog Noise:** The `CHANGELOG_DEV.md` includes an entry for version `0.7.76` (#175) which is not part of this specific code change. While this violates the "isolation of concerns" constraint slightly in the documentation, it does not affect the code quality or functionality.
*   **Redundant Plans:** The implementation plan is provided in two locations (`docs/dev_docs/plans/plan_181.md` and `implementation_plan.md`). This is slightly redundant but ensures the documentation is discoverable.

The patch is functional, safe, and well-tested. The logic change directly maps to the physical problem reported by users.

### Final Rating: #Correct#
