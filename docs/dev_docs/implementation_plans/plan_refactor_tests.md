# Context
The current test suite has a mix of manual failure logging (`std::cout << "[FAIL] ..."`) and macro-based assertions (`ASSERT_TRUE`, etc.). The attempt to refactor all manual failure logging into a centralized `FAIL_TEST` macro resulted in multiple build errors due to syntactical replacement issues (e.g., missing closing parentheses, stray `std::endl` within the macro call, and dangling unused variables like `g_tests_failed`). Creating this Implementation Plan ensures the refactor from scratch is executed cleanly and safely.

# Design Rationale
Using a unified `FAIL_TEST` macro ensures all test failures are strictly appended to the new `g_failure_log` collection, providing a centralized summary at the end of a test run. To prevent developers from bypassing the macro, `g_tests_failed` was renamed to `g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO`. This ensures future assertions consistently report back to the suite runner correctly without manual boilerplate.

# Reference Documents
- Broken reference commit: `c5fe6b808815e323cf115b13fe91fa7d8cafd5d8`

# Codebase Analysis Summary

- **Current Code of the macro**:
```cpp
#define FAIL_TEST(msg) do { \
    std::stringstream ss_fail; \
    ss_fail << "[FAIL] " << g_current_test_name << ": " << msg; \
    std::cout << ss_fail.str() << std::endl; \
    g_failure_log.push_back(ss_fail.str()); \
    g_tests_failed++; \
} while(0)
```

- **Affected Functionalities**: Test Suite failure recording and reporting logic.
- **Affected Files**:
  - `tests/test_ffb_common.h`
  - `tests/test_ffb_common.cpp`
  - `tests/test_ffb_config.cpp`
  - `tests/test_ffb_coordinates.cpp`
  - `tests/test_ffb_core_physics.cpp`
  - `tests/test_ffb_features.cpp`
  - `tests/test_ffb_internal.cpp`
  - `tests/test_ffb_lockup_braking.cpp`
  - `tests/test_ffb_slip_grip.cpp`
- **Design Rationale**: Identifying all test `.cpp` files is critical because the global renaming of the increment counter necessitates codebase-wide macro adoption. Partial adoption leaves compilation errors.

# FFB Effect Impact Analysis
*Not Applicable.* This task strictly modifies testing reporting infrastructure. Physics and output data will remain unchanged.

# Proposed Changes
1. **`tests/test_ffb_common.h`**
   - The `FAIL_TEST(msg)` macro natively accepts any string-streamable expression. For example, `FAIL_TEST("Got value " << value)` leverages the underlying `std::stringstream ss_fail << msg;`.
   - No direct structural changes are needed to the macro itself, but tests must strictly follow the updated call structure.

2. **`tests/test_ffb_common.cpp`**
   - Update the final test statistics printout to use the renamed `g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO` instead of `g_tests_failed`. (e.g. Line ~361).
   - Verify that exception catch blocks correctly invoke `FAIL_TEST` and successfully supply the string stream arguments without throwing syntax errors.

3. **Remaining `tests/test_*.cpp` Refactoring**
   - Identify all manual error logs matching:
     `std::cout << "[FAIL] message" << variables << std::endl;`
     `g_tests_failed++;` (or `g_tests_failed_DO_NOT...++;`)
   - Replace these meticulously with:
     `FAIL_TEST("message" << variables);`
   - **Crucial Refactoring Checks**:
     - **Do NOT** add `<< std::endl` inside `FAIL_TEST`, as the macro already appends it natively.
     - **Ensure** proper syntax with a unified closing parenthesis `)` and semicolon `;`. (The previous broken attempt resulted in syntax like `FAIL_TEST("message";` or trailing `)`) compilation errors).

4. **Version Increment Rule**
   - The version numbers (in `VERSION` and `src/Version.h`) MUST be incremented by the **smallest possible increment** (+1 to the rightmost number), conforming strictly to the versioning policies.

# Test Plan (TDD-Ready)
- **Validation**: 
  - Ensure zero `g_tests_failed` compilation errors across the main test suite build (`cmake --build build`).
  - Intentionally fail a dummy assertion temporarily (e.g., `FAIL_TEST("Artificial Debug Failure");`) and verify that it appears within the concluding "!!! list of FAILED test cases !!!" summary log.
- **Design Rationale**: Modifying compiler-wide macros necessitate strict build system validations as opposed to numerical TDD assertions. 

# Deliverables
- [ ] Implement robust `FAIL_TEST` adoption across all test source `.cpp` files.
- [ ] Fix syntax in `test_ffb_common.cpp` referencing broken global test counters.
- [ ] Update Version numbers for the project.
- [ ] **Implementation Notes**: Update this implementation plan document with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations" after execution.
