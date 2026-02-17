# Implementation Plan - Split FFBEngine (.h/.cpp)

## Context
Split the monolithic `src/FFBEngine.h` into a declaration header (`src/FFBEngine.h`) and an implementation source file (`src/FFBEngine.cpp`).

## Motivation
-   **Compilation Speed**: Reduces build time for consumers of the header.
-   **Code Hygiene**: Hides implementation details (logging, snapshots, complex math) from the API contract.
-   **Maintainability**: enforcing a clear separation of interface and logic.
-   **Namespace Safety**: Allows moving `using namespace ffb_math;` from the header to the source file, preventing namespace pollution for downstream files.

## Codebase Analysis Summary
### Current Architecture
-   **`src/FFBEngine.h`**: A monolithic header-only file (approx. 2000 lines) containing the complete definition and implementation of the `FFBEngine` class. It includes core logic for FFB effects, state management, and telemetry processing.
-   **Dependencies**: The file has significant internal dependencies on `MathUtils.h`, `PerfStats.h`, and `VehicleUtils.h`. It is included by `main.cpp`, `Config.h`, and numerous test files.
-   **Build System**: The `LMUFFB_Core` library target in `CMakeLists.txt` is currently built from this header (and other sources), but lacks a dedicated `.cpp` file for the engine itself.

### Impacted Functionalities
1.  **FFB Engine**: The entire engine implementation will be moved. Function logic remains identical, but the physical location changes.
2.  **Build Process**: The build logic will change from compiling a header-heavy unit to compiling a dedicated source file. Linking steps for executables and tests must include the new object file.
3.  **Tests**: All tests that instantiate `FFBEngine` or link against `LMUFFB_Core`.

## Proposed Changes

### 1. Modify `src/FFBEngine.h`
-   **Keep**:
    -   Class definitions (`FFBEngine`, `FFBSnapshot`, `FFBCalculationContext`).
    -   Member variable declarations.
    -   Function declarations (prototypes).
    -   `using ParsedVehicleClass` alias (for backward compat).
-   **Remove**:
    -   Function bodies (definitions).
    -   `using namespace ffb_math;` (Move to .cpp to avoid pollution).
-   **Update**:
    -   Ensure all inline methods that remain (if any simple accessors) are explicitly marked `inline` or kept in header.

### 2. Create `src/FFBEngine.cpp`
-   **Include**: `"FFBEngine.h"`, `<cmath>`, `<algorithm>`, `"MathUtils.h"`, `"VehicleUtils.h"`, `"PerfStats.h"`, `Config.h` (if needed for circular dep checks).
-   **Content**:
    -   `using namespace ffb_math;`.
    -   Full implementation of all member methods (e.g., `FFBEngine::calculate_force(...)`, `FFBEngine::ParseVehicleClass(...)` wrappers if any).

### 3. Update `CMakeLists.txt`
-   Add `src/FFBEngine.cpp` to the `CORE_SOURCES` list.

## FFB Effect Impact Analysis
*   **No functional changes to FFB effects are intended.**
*   The refactoring is purely structural. Binary output must remain bit-exact.
*   **Performance**: Function call overhead is considered negligible for the 400Hz loop.

## Version Increment Rule
*   Increment version in `VERSION` and `src/Version.h` by **+1 to the rightmost number** (e.g., `0.7.57` -> `0.7.58`).

## Test Plan (TDD-Ready)
Since this is a refactor with no functional changes, existing tests serve as the verification suite.

**1. Compilation & Linking Verification**
*   **Goal**: Ensure the project builds with the new source file structure.
*   **Action**: Run full build.
*   **Success Criteria**: No linker errors (unresolved external symbols) for `FFBEngine` methods.

**2. `run_combined_tests.exe` (Regression)**
*   **Goal**: Verify logic integrity after the move.
*   **Test Count**: Baseline (253 tests) + 0.
*   **Assertions**: All 1107 assertions MUST pass.
*   **Specific Check**: Verify `FFBEngineTestAccess` interactions. Since `FFBEngineTestAccess` is a friend class, its ability to access private members should remain unchanged even if they are defined in a `.cpp` file, provided the linker can resolve the symbols.

## Deliverables Checklist
- [ ] `src/FFBEngine.cpp` created.
- [ ] `src/FFBEngine.h` stripped of implementation.
- [ ] `CMakeLists.txt` updated.
- [ ] `VERSION` incremented.
- [ ] Full build and test pass.
- [ ] Documentation updated (`CHANGELOG_DEV.md`).
- [ ] **Implementation Notes**: Update this plan with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations".

## Implementation Notes
-   **Unforeseen Issues:**
-   **Plan Deviations:**
-   **Challenges Encountered:**
-   **Recommendations for Future Plans:**
