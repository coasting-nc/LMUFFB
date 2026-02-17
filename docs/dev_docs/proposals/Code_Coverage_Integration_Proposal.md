# Proposal: C++ Code Coverage Integration

## Overview
To achieve local and verifiable 100% test coverage for the refactored modules, we need a way to measure which lines of code are executed during our test runs. Since this project uses **MSVC (Visual Studio)** and **CMake** on Windows, I propose a solution centered around **OpenCppCoverage**, combined with a CMake helper to automate the process.

---

## Suggested Tooling

### 1. OpenCppCoverage (Recommended)
**OpenCppCoverage** is the most reliable open-source code coverage tool for Windows/MSVC.
- **Why**: It does not require reallocating or re-instrumenting your code with special compiler flags (unlike `gcov` or `llvm-cov`). It analyzes the debug information (.pdb) while the executable runs.
- **Output**: Generates beautiful HTML reports or XML files (Cobertura format) for CI/CD integration.

### 2. Visual Studio Coverage (Alternative)
If you have access to Visual Studio Enterprise/Professional, there is a built-in "Analyze Code Coverage" feature. However, **Community Edition** (which you currently use) does not include this by default, making OpenCppCoverage the superior cross-user choice.

---

## Proposed Workflow

### A. Manual Execution (CLI)
Once installed, you can generate a report for the combined tests with a single command:
```powershell
OpenCppCoverage.exe --sources src -- .\build\tests\Release\run_combined_tests.exe
```
This will run the tests and automatically open an HTML report showing exactly which lines were hit in the `src/` directory.

### B. CMake Integration
We can add a custom target to `CMakeLists.txt` so coverage can be run via `cmake --build`:

```cmake
# Example CMake addition
find_program(OPENCPPCOVERAGE_EXE OpenCppCoverage)
if(OPENCPPCOVERAGE_EXE)
    add_custom_target(coverage
        COMMAND ${OPENCPPCOVERAGE_EXE} --sources ${CMAKE_SOURCE_DIR}/src -- $<TARGET_FILE:run_combined_tests>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating Code Coverage Report..."
    )
endif()
```

---

---

## VS Code Integration

To see coverage results directly in your editor instead of a browser, you can use the **Coverage Gutters** extension.

### 1. Setup
- Install the **Coverage Gutters** extension in VS Code.
- Use the updated command below to generate a `cobertura.xml` file in your root directory.

### 2. Updated Command (HTML + VS Code XML + Filtering)
To generate the HTML report folder AND the XML file for VS Code in one go, while excluding the tests:
```powershell
OpenCppCoverage.exe --sources src --excluded_sources tests --modules build\tests\Release --export_type=html --export_type=cobertura:cobertura.xml -- .\build\tests\Release\run_combined_tests.exe
```
- `--sources src`: Only includes files in your source directory.
- `--excluded_sources tests`: Explicitly prevents test logic from appearing in the report.
- `--modules build\tests\Release`: Ensures the tool finds the debug symbols for your binary.
- `--export_type=html`: Generates the interactive browser report.
- `--export_type=cobertura:cobertura.xml`: Generates the file needed for VS Code's **Coverage Gutters**.

### 3. Viewing in VS Code
1. Open any source file (e.g., `src/FFBEngine.h`).
2. Click the **Watch** button in the VS Code Status Bar (bottom right, look for a small umbrella or the word "Watch").
3. Green/Red bars will appear in the "gutter" (the area next to line numbers) indicating covered vs. uncovered lines.

---

## Benefits
1.  **In-Editor Visibility**: The **Coverage Gutters** integration eliminates the need to switch to a browser to check coverage.
2.  **Strict Reports**: By excluding the `tests` folder, the coverage percentage accurately reflects your business logic readiness.
3.  **No Code Changes**: Works on binary debug symbols without polluting source code.

## Proposals for Implementation
1.  **Strict Filtering**: Ensure we exclude `vendor/` and `tests/` to keep reports clean.
2.  **Automation**: Create a PowerShell script to automate the "Build + Coverage + Export" cycle.

## Next Steps
- Would you like me to create an automation script for this workflow?
