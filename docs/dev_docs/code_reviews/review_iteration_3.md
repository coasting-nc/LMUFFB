The proposed patch addresses the issue where the steering "soft lock" (the physical stop at the steering rack limit) becomes weaker as the driving session progresses and the "session peak torque" increases. This occurred because the soft lock force was being treated as a "structural" force and thus normalized by the dynamic session peak.

### User's Goal
Ensure the steering soft lock provides a consistent, strong physical resistance that is independent of the dynamic force normalization learned during driving.

### Evaluation of the Solution

#### Core Functionality
The solution is physically and logically sound. By moving the `soft_lock_force` from the `structural_sum` (normalized by session peak) to the `texture_sum_nm` group (scaled by the absolute wheelbase maximum torque), the patch ensures that the resistance felt at the steering limit is always relative to the hardware's maximum capability, not the intensity of the driving. This directly resolves the reported weakness.

#### Safety & Side Effects
*   **Physics:** The change correctly models the soft lock as an absolute physical limit rather than a relative steering effect.
*   **Thread Safety:** The changes are contained within the `FFBEngine::calculate_force` method, which operates under the protection of the existing `g_engine_mutex`.
*   **Regressions:** No regressions are introduced. The summation logic remains mathematically consistent, and the shift in scaling groups is appropriate for this specific force type.
*   **Security:** No security issues (exposed secrets, etc.) were introduced.

#### Completeness
The patch is comprehensive and follows the project's specific procedural requirements:
*   **Verification:** Includes a new regression test (`tests/test_issue_181_soft_lock_weakness.cpp`) that explicitly verifies the force remains constant (and strong) across varying session peak torque levels.
*   **Process Compliance:** Includes the required `implementation_plan.md` and `review_iteration_1.md` as requested in the user's interaction history.
*   **Versioning:** Correctly updates the `VERSION` file. It specifically avoids manually editing `src/Version.h`, adhering to the user's constraint despite mentioning it in the documentation files (a minor documentation staleness that does not affect the code).
*   **Changelog:** Updates both the developer and user-facing changelogs.

### Merge Assessment

**Nitpicks:**
*   **Documentation Staleness:** The `implementation_plan.md` and `review_iteration_1.md` claim that `src/Version.h` was updated. While the agent correctly omitted this change from the code (following user instructions), the documentation files weren't updated to reflect that omission.
*   **Plan Duplication:** The implementation plan is provided in two locations (root and `docs/dev_docs/plans/`).
*   **Changelog Noise:** The `CHANGELOG_DEV.md` includes an entry for an unrelated version (0.7.76).

These nitpicks are non-blocking as the core code change and the provided tests are correct and follow the most critical project constraints.

### Final Rating: #Correct#
