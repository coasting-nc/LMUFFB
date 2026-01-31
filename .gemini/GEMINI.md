# GEMINI Agent Guidelines

## ⚠️ Critical Git Workflow Instruction

**NEVER** touch the git staging area.

*   **PROHIBITED COMMANDS:**
    *   `git add`
    *   `git commit`
    *   `git reset`
    *   `git push`
    *   `git restore --staged`

*   **NO EXCEPTIONS**: Do not stage files even if you created them or if you think it helps.
*   **The User** is exclusively responsible for reviewing and staging changes.
*   **Your Role:** Modify files on disk, run tests, verify the build, and then **stop**.


## 🧪 Test-Driven Development (TDD) Requirement

You MUST always use a TDD approach for all code changes:
1.  **Write a test** (or update an existing one) that covers the new feature or fix.
2.  **Verify failure**: Run the test and ensure it fails as expected.
3.  **Implement**: Write the minimum code necessary to make the test pass.
4.  **Verify success**: Run the tests again to ensure everything passes and no regressions were introduced.

## Build and Test Instructions

Use these commands to build and run tests:

### Build everything (Main App + Tests)
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release --clean-first
```

### Run all tests
```powershell
.\build\tests\Release\run_combined_tests.exe
```
