### Code Review: Issue #309 - Tire Load Fallback Standardization

The proposed code change addresses the core requirements of Issue #309 by standardizing the tire load fallback logic and removing the obsolete kinematic estimation model.

#### 1. User's Goal
Standardize tire load fallback logic to use suspension-based approximation instead of kinematic estimation when telemetry is missing, and remove all traces of the now-unnecessary kinematic model.

#### 2. Evaluation of the Solution

##### Core Functionality
- **Logic Correction:** The patch correctly modifies `FFBEngine::calculate_force` and `FFBEngine::calculate_sop_lateral` to use `approximate_load()` and `approximate_rear_load()` whenever tire load telemetry is missing.
- **Simplification:** It removes the nested fallback logic that attempted to choose between suspension-based and kinematic models, opting for the more accurate suspension-based method as the sole fallback.
- **Code Removal:** The patch successfully deletes `calculate_kinematic_load()` and its associated member variables (mass, aero coefficient, etc.), cleaning up the `FFBEngine` class significantly.
- **Consistency:** It uses `ctx.frame_warn_load` as the central state for determining when to apply fallbacks, ensuring consistency across different physics calculation methods.

##### Safety & Side Effects
- **Regression Testing:** The patch includes updates to a wide range of tests (issue 213, 306, 322, etc.) that previously relied on the kinematic model or had assumptions about load calculation. This demonstrates a thorough understanding of the cross-component impact.
- **New Coverage:** A new regression test (`test_issue_309_load_fallback.cpp`) was added to verify the accuracy of the new fallback path.
- **Logging:** Log messages were updated to correctly describe the fallback being used.

##### Completeness
- **Logic completeness:** All necessary call-sites and configurations appear to be updated.
- **Administrative incompleteness:** The patch fails to fulfill several mandatory mission constraints:
    - The `VERSION` file was not incremented.
    - The `USER_CHANGELOG.md` was not updated.
    - No code review records (`review_iteration_X.md`) were included in the submission, despite being required by the "Iterative Quality Loop" process.
    - The implementation plan was not updated with final implementation notes as required.

#### 3. Merge Assessment (Blocking vs. Non-Blocking)
- **Blocking:** The missing `VERSION` update and `USER_CHANGELOG.md` entry are blocking for a production merge, as they are part of the project's mandatory release process and the agent's specific instructions.
- **Blocking:** The missing review records and final implementation notes indicate the "Fixer" workflow was not fully completed, making the submission incomplete according to the provided "mission" requirements.
- **Nitpick:** Some tests were relaxed (e.g., in `test_issue_322_yaw_kicks.cpp`) from checking specific torque values to checking for general non-zero activity. While understandable given the change in the underlying physics model, it represents a minor loss in test precision.

### Final Rating: #Mostly Correct#
