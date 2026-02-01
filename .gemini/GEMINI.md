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

## 📦 Version Increment Rule

When incrementing the version number (in `VERSION` and `src/Version.h`), you **MUST** always use the **smallest possible increment**:
*   Add **+1 to the rightmost number** in the version string.
*   Example: `0.6.39` → `0.6.40`
*   Example: `0.7.0` → `0.7.1`

**Do NOT** increment higher-level numbers unless explicitly instructed by the user.


## 📁 File Encoding Issues

When working with source files, you may encounter encoding issues that prevent file reading/editing tools from working correctly.

**Common Error:**
```
Error: unsupported mime type text/plain; charset=utf-16le
```

**Quick Workaround:**
```powershell
# Convert file to UTF-8
Get-Content "file.cpp" | Out-File -FilePath "file_utf8.cpp" -Encoding utf8
```

**Full Documentation:** See [`docs/dev_docs/unicode_encoding_issues.md`](docs/dev_docs/unicode_encoding_issues.md) for:
- Root causes and detection methods
- Multiple solution approaches
- Prevention best practices
- Batch conversion scripts
