# Implementation Plan: Migrate Legacy Tests to Auto-Registration Pattern

## Context
Refactor the remaining legacy test files to use the new `TEST_CASE` / `TEST_CASE_TAGGED` auto-registration system. This prevents the issue where tests are defined (e.g., as `static void`) but never actually called in the runner, and it unifies the test reporting and filtering across the entire suite.

## Reference Documents
*   `docs/dev_docs/implementation_plans/completed/fix_tests_defined_but_not_called.md` (Original design for the registry)
*   `tests/test_ffb_common.h` (Registry definitions)

## Codebase Analysis Summary
### Current Architecture Overview
The test suite utilizes a `TestRegistry` singleton that collects `TestEntry` objects during static initialization via the `AutoRegister` helper (invoked by `TEST_CASE` macros). The `FFBEngineTests::Run()` function then iterates through all registered tests, applying tag filters and category ordering.

### Impacted Functionality
*   **Test Management**: Moving from manual `Run_Category()` functions to a decentralized registration model.
*   **Test Reporting**: Global counters `g_tests_passed` and `g_tests_failed` are now shared across all translation units.
*   **Windows-specific Tests**: These were previously siloed and required manual coordination in `main_test_runner.cpp`.

## FFB Effect Impact Analysis
*   **None**: This change only affects the testing infrastructure and does not modify the FFB engine logic or parameters.

## Proposed Changes

### 1. `tests/test_ffb_common.cpp`
*   Update `CATEGORY_ORDER` to include: `"Windows"`, `"Screenshot"`, `"Persistence"`, `"GUI"`.
*   Ensure the sort order remains logical (Physics first, then features, then platform tests).

### 2. `tests/test_ffb_common.h`
*   Centralize common assertion macros to reduce boilerplate in test files.
*   Add `ASSERT_EQ(a, b)` for standard equality checks.
*   Add `ASSERT_EQ_STR(a, b)` for `std::string` or `const char*` comparisons.

### 3. Migration of Test Files
For each of the following files, replace `static void test_xxx()` with `TEST_CASE(test_xxx, "Category")` and remove the local `Run()` function and manual counters:
*   `tests/test_screenshot.cpp` (Category: "Screenshot")
*   `tests/test_windows_platform.cpp` (Category: "Windows")
*   `tests/test_gui_interaction.cpp` (Category: "GUI")
*   `tests/test_persistence_v0625.cpp` (Category: "Persistence")
*   `tests/test_persistence_v0628.cpp` (Category: "Persistence")

### 4. `tests/main_test_runner.cpp`
*   Remove forward declarations for legacy namespace `Run()` functions.
*   Remove all `try/catch` blocks for individual test suites.
*   Simplify the `main()` function to call only `FFBEngineTests::Run()`.

### 5. Version Increment
*   The version should be incremented by the smallest possible increment in `VERSION` and `src/Version.h` once these changes are verified.
*   Current version: `0.7.16` -> Proposed: `0.7.17`.

### 6. Rationale for Code Deletions and Cleanup
To maintain a high-quality codebase, several redundant blocks were removed during this migration:
*   **`tests/main_test_runner.cpp`**: Removed manual namespace forward declarations and individual `Run()` calls. These are now obsolete because the `TestRegistry` automatically discovers and executes tests via the `TEST_CASE` macro's static initialization.
*   **Assertion Macros (`ASSERT_TRUE`, etc.)**: Removed local definitions from individual test files (e.g., `test_persistence_v0625.cpp`, `test_screenshot.cpp`). These were consolidated into `test_ffb_common.h` to ensure a single source of truth for test logic and to avoid naming collisions.
*   **Global Counters**: Local `g_tests_passed` and `g_tests_failed` variables were removed from individual files. These are now `extern` variables defined in `test_ffb_common.cpp`, allowing all tests across different files to contribute to a single, unified score.
*   **Redundant Includes**: Removed standard library headers (e.g., `iostream`, `vector`, `string`) from files like `test_windows_platform.cpp` where they are already provided by the mandatory include of `test_ffb_common.h`. This reduces compilation overhead and clutter.

## Test Plan (TDD-Ready)
### Verification Strategy
1.  **Baseline Check**: Run the existing test suite and note the total count (expect ~867 assertions).
2.  **Build Verification**: Compile with MSVC (Windows) to ensure DirectInput and GUI dependencies are correctly resolved in the unified namespace.
3.  **Linux Compatibility**: Verify that `CMakeLists.txt` still correctly excludes Windows-only source files from the build, ensuring the registry only contains platform-agnostic tests on Linux.
4.  **Final Execution**: Run `run_combined_tests.exe` and verify:
    *   No tests are skipped unintentionally.
    *   The "Auto-Registered Tests" section at the end contains the platform results.
    *   Total test count matches or exceeds the baseline.

## Deliverables
*   [x] Updated `test_ffb_common.h/cpp` with expanded categories and macros.
*   [x] Migrated all Windows and Persistence test files.
*   [x] Streamlined `main_test_runner.cpp`.
*   [x] Verified build and test execution (867/867 Passed).

## Implementation Notes
*   **Unforeseen Issues**: Some legacy tests mixed `printf` and `std::cout`. Standardized on `std::cout` for consistency with the runner's output.
*   **Plan Deviations**: Originally planned to keep `PersistenceTests` separate, but decided to unify them since they are platform-agnostic and should run on Linux as well.
*   **Challenges**: Handling the `friend` unit tests for `FFBEngine` required careful namespace management to ensure the registry could see the friend functions.
*   **Documentation Update**: Added "Rationale for Code Deletions and Cleanup" section to document architectural de-duplication decisions (Feb 2026).
