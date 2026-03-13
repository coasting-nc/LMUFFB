# Implementation Plan - Fix: car class detected as "Unknown" for hypercars #346

## Context
Hypercars in LMU are sometimes detected as "Unknown" class because the class name string provided by the game ("Hyper") is not matched by the current parser, and some vehicle names (like "Cadillac WTR 2025") lack the specific keywords currently used for fallback identification.

## Design Rationale
The vehicle identification system relies on string matching against class names and vehicle names. Reliability is improved by:
1. Expanding the primary identification to include "HYPER" (matching the raw class name "Hyper" observed in logs).
2. Adding more vehicle-specific keywords for better fallback identification when class names are ambiguous or missing.
3. Ensuring case-insensitivity (already handled) and robust substring matching.

## Codebase Analysis Summary
- **Impact Zone:** `src/VehicleUtils.cpp` (`ParseVehicleClass` function).
- **Affected Functionalities:**
    - FFB Seed Load: If the class is "Unknown", it defaults to 4500N instead of 9500N for Hypercars, leading to incorrect initial FFB scaling.
    - Logging: Binary log filenames and debug logs will show "Unknown" instead of "Hypercar".
- **Design Rationale:** `VehicleUtils.cpp` is the centralized location for vehicle categorization logic. Modifying it ensures all downstream systems (FFB, Logger, UI) benefit from the improved detection.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| **Initial Force Scaling** | `m_auto_peak_front_load` will correctly initialize to 9500N for Hypercars. | FFB will feel "correct" immediately upon entering the car, rather than requiring the auto-peak-learning to adjust from a lower baseline. |
| **Log Filenames** | No change to logic, just better metadata. | Easier to identify log files for Hypercar sessions. |

## Proposed Changes

### `src/VehicleUtils.cpp`
- **Modify `ParseVehicleClass`**:
    - Add `cls.find("HYPER") != std::string::npos` to the Hypercar identification block.
    - Add `name.find("CADILLAC") != std::string::npos` to the Hypercar name fallback block.
    - **Design Rationale:** "HYPER" covers the observed "Hyper" class name. "CADILLAC" is added because currently LMU only features the Cadillac Hypercar, making it a reliable identifier for the class in this context.

### `VERSION` & `src/Version.h.in`
- Increment version to the smallest possible increment.
- **Design Rationale:** Standard versioning procedure for small fixes.

### `CHANGELOG_DEV.md`
- Add entry for the fix.

## Test Plan (TDD-Ready)
- **New Test Cases in `tests/test_vehicle_utils.cpp`**:
    - `test_issue_346_repro`: Verify that `ParseVehicleClass("Hyper", "Cadillac WTR 2025 #101:LM")` returns `ParsedVehicleClass::HYPERCAR`.
    - `test_hyper_short_name`: Verify `ParseVehicleClass("Hyper", "")` returns `ParsedVehicleClass::HYPERCAR`.
- **Design Rationale:** These tests directly target the failure case reported in the issue and ensure the expanded keyword matching works as expected.

## Implementation Steps
1. **Create reproduction tests** in `tests/test_vehicle_utils.cpp`.
2. **Run tests** to verify failure (`./build/tests/run_combined_tests`).
3. **Apply fixes** to `src/VehicleUtils.cpp`.
4. **Verify changes** in `src/VehicleUtils.cpp` using `read_file`.
5. **Run tests** to verify fix.
6. **Perform code review** and iterate if necessary.
7. **Update version and changelog**.
8. **Pre-commit steps** for final verification.
9. **Submit**.

## Deliverables
- [ ] Modified `src/VehicleUtils.cpp`
- [ ] Updated `tests/test_vehicle_utils.cpp`
- [ ] Updated `VERSION` and `CHANGELOG_DEV.md`
- [ ] Implementation Plan with final notes.

## Implementation Notes
No significant issues encountered. Implementation proceeded as planned.

### Encountered Issues
- None.

### Plan Deviations
- None.

### Challenges
- Ensuring "CADILLAC" is a reliable keyword for Hypercars in LMU. Currently, LMU only has the Cadillac Hypercar, making it a safe identifier.

### Recommendations for Future Plans
- None.
