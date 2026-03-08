---
name: tdd_workflow
description: Strict Test-Driven Development (TDD) workflow for C++ logic changes and new features
---

# TDD Workflow (C++)

Use this skill to ensure all code changes are verified through a robust Test-Driven Development process.

## The TDD Cycle

1. **Write a Test (Red Phase)**
   - Add or update test cases in `tests/`.
   - Ensure the test covers the new feature or fix exactly as specified.
   
2. **Verify Failure**
   - Compile and run the tests.
   - **Confirm the test fails as expected.** This proves the test is valid and not a false positive.
   
3. **Implement (Green Phase)**
   - Write the **minimum code necessary** to make the test pass.
   - Follow project patterns (e.g., no heap allocation in `FFBEngine`).
   
4. **Verify Success**
   - Compile and run the tests again.
   - Ensure the new test passes AND no regressions were introduced in existing tests.

## Meaningful Tests
- **Actual Functionality**: Tests must verify actual behavior, not just "hit" lines to inflate coverage numbers.
- **Fail First**: Ensure the test fails before implementation to avoid false positives.
- **Boundary Conditions**: Test empty buffers, full buffers, and wraparound behavior for stateful algorithms.

## Iterative Quality Loop
1. **Build & Test**: Run the full suite (`run_combined_tests`).
2. **Commit**: Save intermediate progress (e.g., `git commit -am "WIP: Iteration N"`).
3. **Independent Review**: Request a code review and save it to `docs/dev_docs/code_reviews/review_iteration_X.md`.
4. **Fix & Repeat**: Address feedback and loop until a "Greenlight" is received.

## Linux/Windows Hybrid Testing
- The project is Windows-native but developed on Linux using **mocks** (e.g., `LinuxMock.h`).
- Ensure new tests are runnable on Linux via mocks.
- You **cannot** run the full GUI or drivers on Linux; rely on unit tests for logic verification.

## Commands

### Build everything (Main App + Tests)
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release
```

### Run all tests
```powershell
.\build\tests\Release\run_combined_tests.exe
```

## Exception Case
If writing tests is impractical (e.g., hardware timing, complex UI edge cases):
1. Prioritize feature completion.
2. Document what was not tested and why.
3. Add a backlog item for future testing.
