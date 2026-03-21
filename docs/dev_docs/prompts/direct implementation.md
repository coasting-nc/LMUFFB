Read `docs\dev_docs\investigations\redesign presets system.md` and `docs\dev_docs\investigations\redesign presets system - Phase 1.md`.
Then follow the plan defined in "## Adopt the 'Strangler Fig' pattern in refactoring" (and where necessary, also what is described in "## Logical Categories Plan").

Note that the refactoring for these logical categories has already been performed: 
* `GeneralConfig`
* `FrontAxleConfig`
* `RearAxleConfig` 

Your task is to implement the **`LoadForcesConfig`** category. 
The specific variables that belong in this struct are: `lat_load_effect`, `lat_load_transform`, `long_load_effect`, `long_load_smoothing`, and `long_load_transform`. Do not move any other variables.

**⚠️ CRITICAL REFACTORING RULE:**
You are performing a structural refactor. You must **NOT** change any default values (e.g., in `Config.cpp` or `FFBEngine.h`), you must **NOT** alter any physics logic, and you must **NOT** modify the assertions of existing tests to make them pass. If an existing test fails, your refactor broke the state machine or initialization logic. Fix your refactor, do not change the test.

**Test-Driven Development (TDD) Requirement:**
Before changing any production code, you must create a new test file (`tests/test_refactor_load_forces.cpp`) and write 3 specific safety tests:
1. **Consistency Test:** Hardcode a specific telemetry state and specific `LoadForcesConfig` values. Run this test on the *current* codebase to get the exact `final_force` output, hardcode that expected value into the test, and `ASSERT_NEAR` against it.
2. **Round-Trip Test:** Create a `Preset` with wild `LoadForcesConfig` values, call `Apply(engine)`, extract it back with `UpdateFromEngine(engine)`, and `ASSERT_TRUE` that the original and extracted presets are `Equals()`.
3. **Validation Test:** Create a `Preset` with malicious out-of-bounds `LoadForcesConfig` values. Call `Apply()`, and assert that the engine clamped them to the safe ranges defined in `LoadForcesConfig::Validate()`.

**Documentation Requiremendocs\dev_docs\prompts\direct implementation.mdt:**
Document your work by creating `docs/dev_docs/implementation_plans/redesign_preset_system_phase1_load_forces.md`. 
Describe what you did; periodically update this document as you progress through the work for the patch.
Include also these sections: encountered issues, deviations from the plan, and suggestions for the future. 
Document deviations and build issues. Note any specific issues encountered during the Build/Test loop. In the implementation notes also discuss any issues raised by the code reviews and how you addressed them; also discuss there any discrepancies between you and the code reviews (e.g., the code review said the patch would not build, but it does).
See also the implementation notes that were created in previous iterations: 
* `docs\dev_docs\implementation_plans\redesign_preset_system_phase1_general.md`
* `docs\dev_docs\implementation_plans\redesign_preset_system_phase1_front_axle.md`
* `docs\dev_docs\implementation_plans\redesign_preset_system_phase1_rear_axle.md`

**⚠️ CRITICAL WORKFLOW CONSTRAINTS:**
1. **Scope:** Work ONLY on `LoadForcesConfig`. Do not touch `BrakingConfig`, `VibrationConfig`, etc.
2. **Autonomous Execution:** **Do not stop** to ask the user for confirmation or permission to proceed. You must loop through the implementation, testing, and review process autonomously until the task is complete, all 580+ tests pass, and the code is perfect.