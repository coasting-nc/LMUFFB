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
