Read `docs\dev_docs\investigations\redesign presets system.md` and `docs\dev_docs\investigations\redesign presets system - Phase 1.md`.
Then follow the plan defined in "## Adopt the 'Strangler Fig' pattern in refactoring" (and where necessary, also what is described in "## Logical Categories Plan").

Note that the refactoring for these logical categories has already been performed: 
* `GeneralConfig`
* `FrontAxleConfig`
* `RearAxleConfig`
* `LoadForcesConfig`

Your task is to implement the **`GripEstimationConfig`** category. 

**⚠️ CRITICAL REFACTORING RULE:**
You are performing a structural refactor. You must **NOT** change any default values (e.g., in `Config.cpp` or `FFBEngine.h`), you must **NOT** alter any physics logic, and you must **NOT** modify the assertions of existing tests to make them pass. If an existing test fails, your refactor broke the state machine or initialization logic. Fix your refactor, do not change the test.

**Test-Driven Development (TDD) Requirement:**
Before changing any production code, you must create a new test file (`tests/test_refactor_<name of logical category>.cpp`) and write 3 specific safety tests:
1. **Consistency Test:** Hardcode a specific telemetry state and specific `GripEstimationConfig` values. Run this test on the *current* codebase to get the exact `final_force` output, hardcode that expected value into the test, and `ASSERT_NEAR` against it.
2. **Round-Trip Test:** Create a `Preset` with wild `GripEstimationConfig` values, call `Apply(engine)`, extract it back with `UpdateFromEngine(engine)`, and `ASSERT_TRUE` that the original and extracted presets are `Equals()`.
3. **Validation Test:** Create a `Preset` with malicious out-of-bounds `GripEstimationConfig` values. Call `Apply()`, and assert that the engine clamped them to the safe ranges defined in `GripEstimationConfig::Validate()`.

**Documentation Requirement:**
Document your work by creating `docs/dev_docs/implementation_plans/redesign_preset_system_phase1_<name of logical category>.md`. 
Describe what you did; periodically update this document as you progress through the work for the patch.
Include also these sections: encountered issues, deviations from the plan, and suggestions for the future. 
Document deviations and build issues. Note any specific issues encountered during the Build/Test loop. In the implementation notes also discuss any issues raised by the code reviews and how you addressed them; also discuss there any discrepancies between you and the code reviews (e.g., the code review said the patch would not build, but it does).
See also the implementation notes that were created in previous iterations: 
* `docs\dev_docs\implementation_plans\redesign_preset_system_phase1_general.md`
* `docs\dev_docs\implementation_plans\redesign_preset_system_phase1_front_axle.md`
* `docs\dev_docs\implementation_plans\redesign_preset_system_phase1_rear_axle.md`
* `docs/dev_docs/implementation_plans/redesign_preset_system_phase1_load_forces.md`

You must also increase the app version in `VERSION` file (by the smallest increment possible), and add an entry to `CHANGELOG_DEV.md` file.

**⚠️ CRITICAL WORKFLOW CONSTRAINTS:**
1. **Scope:** Work ONLY on `GripEstimationConfig`. Do not work on other logical categories (those will be worked on in future iterations).
2. **Autonomous Execution:** **Do not stop** to ask the user for confirmation or permission to proceed. You must loop through the implementation, testing, and review process autonomously until the task is complete, all 580+ tests pass, and the code is perfect.

**⚠️ MANDATORY CHECKPOINTING & RECOVERY PROTOCOL:**
To prevent losing work due to file corruption and resetting your workspace, you **MUST** use version control to create incremental checkpoints. 

1. **Micro-Commits:** You must commit your work at every stable step. Do not wait until the entire task is finished. 
   * *Checkpoint 1:* Commit immediately after writing the 3 new safety tests (even if they don't compile yet).
   * *Checkpoint 2:* Commit immediately after defining the struct for the logical category in the header files.
   * *Checkpoint 3:* Commit after updating `Preset` and `FFBEngine` and fixing the compiler errors.
   * *Checkpoint 4:* Commit after all 580+ tests pass.
2. **Pushing/Submitting:** Push your branch or submit your intermediate patch after every successful checkpoint so your progress is permanently saved.
3. **Recovery (DO NOT START FROM SCRATCH):** If you corrupt a file with your editing tools, or if you get stuck in a loop of compiler errors that you cannot resolve, **DO NOT delete your files or reset the entire workspace.** 
   * If a single file is corrupted, use `git checkout -- <filename>` to restore just that file.
   * If the whole step is broken, use `git reset --hard HEAD` to revert to your last successful micro-commit. 
   * Analyze what went wrong, and try the step again from the last checkpoint.