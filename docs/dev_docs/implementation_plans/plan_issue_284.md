# Implementation Plan - Automatically Download LZ4 Vendor Files (#284)

## Context
The project depends on `lz4.c` and `lz4.h` from the LZ4 library, which are expected to be in `vendor/lz4/`. Currently, these files are missing from the repository (ignored by `.gitignore`) and must be manually downloaded or provided by CI workflows. This issue aims to automate the download process within the CMake build system to ensure a seamless build experience for developers.

## Design Rationale
- **Automation**: Moving the download logic into `CMakeLists.txt` ensures that the dependencies are always present when configuring the project, reducing manual setup steps.
- **FetchContent vs. file(DOWNLOAD)**: `FetchContent` is the modern CMake way to manage external dependencies. It handles downloading, extraction, and making the source available during the configuration phase.
- **Consistency**: The project already uses `FetchContent` for ImGui. Following the same pattern for LZ4 maintains consistency in the build system.
- **Fallthrough**: Supporting local files in `vendor/lz4/` allows developers to provide their own version or work offline if the files are already present.

## Reference Documents
- GitHub Issue #284: Update the makefiles to automatically download the lz4.c/h vendor files required by CMake

## Codebase Analysis Summary
- **Current Architecture**:
    - `CMakeLists.txt` explicitly includes `vendor/lz4` and adds `vendor/lz4/lz4.c` to `CORE_SOURCES`.
    - `.gitignore` ignores the `vendor/` directory.
    - CI workflows (`.github/workflows/*.yml`) use `wget` to download these files manually.
- **Impacted Functionalities**:
    - Build System: The configuration step will now include a download phase for LZ4.
    - CI Workflows: Manual download steps in YAML files can eventually be removed or will become redundant.

## FFB Effect Impact Analysis
- N/A. This is a build system improvement and does not affect FFB logic or feel.

## Proposed Changes

### 1. `CMakeLists.txt`
- **Implement LZ4 detection and download**:
    - Add a block similar to the ImGui block to check for `vendor/lz4/lz4.c`.
    - If missing, use `FetchContent` to download the LZ4 repository (tag `v1.10.0`).
    - Set `LZ4_DIR` to either `vendor/lz4` or the fetched source `lib` directory.
- **Update include and source paths**:
    - Replace hardcoded `vendor/lz4` paths with `${LZ4_DIR}`.

### 2. Versioning
- Increment version in `VERSION` file.

## Test Plan
- **Verification of Build**:
    - Run `cmake` on a clean environment (where `vendor/lz4` is missing).
    - Verify that `lz4.c` and `lz4.h` are downloaded/made available.
    - Verify that the project compiles successfully.
    - Run `run_combined_tests` to ensure no regressions.

## Deliverables
- [x] Modified `CMakeLists.txt`
- [x] Updated `VERSION`
- [x] Updated `CHANGELOG_DEV.md`
- [x] Implementation Notes in this plan.

## Implementation Notes
- **Issue Resolved**: Automated the download of LZ4 vendor files (`lz4.c`/`lz4.h`) via CMake `FetchContent`.
- **Result**: The build system now automatically fetches LZ4 if it's not present in `vendor/lz4`, ensuring a "one-click" build experience and simplifying CI workflows.
- **Challenges**: Initial configuration failed because I didn't specify the `lib` subdirectory for `FetchContent` populated source. Fixed by setting `LZ4_DIR` to `${lz4_SOURCE_DIR}/lib`.
- **Deviations**: None.
- **Iterative Review Process**:
    - **Iteration 1**: Rated #Partially Correct#. Feedback highlighted unrelated log files in the root directory and missing documentation (review records and implementation notes).
    - **Resolution**: Removed artifact log files, added `docs/dev_docs/code_reviews/issue_284_review_iteration_1.md`, and updated this implementation plan with final notes.
    - Iteration 2: Rated #Correct#. Greenlight received.
