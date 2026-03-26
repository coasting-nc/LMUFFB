# Implementation Plan - Issue #475: Lateral Load slider value is not taken from config after reloading the app

## Context
The user reports that the "Lateral Load" slider value resets to 2.0 (200%) upon application restart, even if a higher value (up to 10.0 / 1000%) was saved in the configuration file. This is caused by a mismatch between the GUI's allowed range [0.0, 10.0] and the `LoadForcesConfig::Validate()` method's clamping range [0.0, 2.0].

## Design Rationale
The core philosophy of LMUFFB is reliability and consistency. The configuration system should faithfully preserve user settings as long as they are within safe physical limits. The GUI was previously updated to allow up to 10.0 for Lateral Load to accommodate high-end direct drive wheels and specific user preferences, but the validation logic was left restricted to 2.0. To fix this, the validation logic must be synchronized with the GUI limits.

## Codebase Analysis Summary
- **Affected File:** `src/ffb/FFBConfig.h`
- **Impacted Functionality:** `LoadForcesConfig::Validate()`
- **Data Flow:** `Config::Load()` reads the value from `config.toml` -> calls `engine.m_load_forces.Validate()` -> value is clamped if > 2.0 -> GUI displays the clamped value.
- **Design Rationale:** `FFBConfig.h` contains the POD structures for configuration and their validation logic. This is the correct place to enforce limits that should be consistent across the app.

## FFB Effect Impact Analysis
- **Affected Effect:** Lateral Load (SOP Lateral).
- **Technical/Developer Perspective:** Only `LoadForcesConfig::Validate()` needs modification to increase the upper clamp limit from 2.0f to 10.0f.
- **User Perspective:** Users will finally be able to persist Lateral Load settings above 200%. The feel will remain identical to what they set in the GUI during a session, but it will now persist across restarts.
- **Design Rationale:** Increasing the limit to 10.0 matches the existing GUI capability and allows users more flexibility while still providing a (larger) safety bound.

## Proposed Changes

### 1. Create Reproduction Test
- **Action:** Create `tests/repro_issue_475.cpp`.
- **Reason:** To programmatically confirm the bug (clamping to 2.0) before fixing it.

### 2. Update Test Build System
- **Action:** Add `repro_issue_475.cpp` to `tests/CMakeLists.txt`.
- **Reason:** Enable compilation and execution of the new test.

### 3. Build and Run Initial Test
- **Action:** Build the project and run tests to confirm `repro_issue_475.cpp` fails.

### 4. Modify Configuration Validation
- **File:** `src/ffb/FFBConfig.h`
- **Action:** Update `LoadForcesConfig::Validate()` to clamp `lat_load_effect` between `0.0f` and `10.0f`.
- **Design Rationale:** Using `std::clamp(val, 0.0f, 10.0f)` (or equivalent `min`/`max` calls already present) ensures consistency with the GUI's range.

### 5. Verify Fix
- **Action:** Build and run tests again. Confirm `repro_issue_475.cpp` now passes.
- **Verification:** Use `read_file` to confirm the change in `src/ffb/FFBConfig.h`.

### 6. Version Increment
- **File:** `VERSION`
- **Action:** Increment from `0.7.252` to `0.7.253`.
- **Rule:** Smallest possible increment.

### 7. Changelog Update
- **File:** `CHANGELOG_DEV.md`
- **Action:** Add an entry for the fix under version `0.7.253`.

### 8. Complete Pre-commit Steps
- **Action:** Ensure proper testing, verification, review, and reflection are done.

### 9. Submission
- **Action:** Submit the changes with a descriptive commit message.

## Test Plan
### New Test: `test_issue_475_lateral_load_clamping`
- **Location:** `tests/repro_issue_475.cpp`.
- **Description:** Verify that `LoadForcesConfig::Validate()` correctly handles values up to 10.0f and clamps anything above.
- **Assertions:**
  1. Set `lat_load_effect = 10.0f`. Call `Validate()`. Assert `lat_load_effect == 10.0f`. (Fails before fix)
  2. Set `lat_load_effect = 11.0f`. Call `Validate()`. Assert `lat_load_effect == 10.0f`.
- **Verification:** Run `./build/tests/run_combined_tests` and ensure all tests pass, including the new one.

## Deliverables
- [ ] Modified `src/ffb/FFBConfig.h`.
- [ ] New `tests/repro_issue_475.cpp`.
- [ ] Modified `tests/CMakeLists.txt`.
- [ ] Updated `VERSION`.
- [ ] Updated `CHANGELOG_DEV.md`.
- [ ] Quality Assurance Records (review_iteration_X.md).
- [ ] Updated Implementation Notes in this plan.

## Implementation Notes
- **Issue Identification:** Confirmed that `LoadForcesConfig::Validate()` was clamping `lat_load_effect` to 2.0f, while the GUI allowed up to 10.0f.
- **Verification:** Created a reproduction test `tests/repro_issue_475.cpp` which failed before the fix and passed after.
- **Code Review:** One round of review was performed. The reviewer confirmed the fix is correct and aligns with project standards.
- **Regressions:** Ran the full test suite (633 tests); all passed, ensuring no side effects.
- **Procedural Note:** As per the instructions, the code review output was saved to `docs/dev_docs/code_reviews/issue_475_review_iteration_1.md`.
