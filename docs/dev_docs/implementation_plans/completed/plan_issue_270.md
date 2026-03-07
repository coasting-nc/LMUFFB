# Implementation Plan - Fix car brand detection for LMP2 and LMP3

**Issue:** #270 - Fix car brand being detected as "unknown" for the LMP2 cars.

## Context
In the telemetry logs (`.bin` files), some cars are being identified with "Unknown" as their brand. Specifically, LMP2 cars (which are mostly Oreca 07 chassis) and newer LMP3 cars (Ligier JS P325, Duqueine D09) are not being correctly mapped to their manufacturers/brands in `VehicleUtils.cpp`.

## Design Rationale
The current brand detection logic in `ParseVehicleBrand` relies on keyword matching in the vehicle name. To improve reliability:
1.  **LMP2 Special Case:** Since almost all LMP2s in the context of LMU/WEC are Oreca 07 chassis, we should default to "Oreca" if the class is LMP2 and no other brand is found.
2.  **LMP3 Expansion:** Add missing keywords for newer LMP3 chassis (P325, D09) to ensure they are correctly identified as Ligier and Duqueine respectively.
3.  **Robustness:** Maintain the existing hierarchical check (Vehicle Name first, Class Name fallback) but improve the keyword coverage.

## Reference Documents
- GitHub Issue #270

## Codebase Analysis Summary
- **Impacted File:** `src/VehicleUtils.cpp`
- **Function:** `ParseVehicleBrand(const char* className, const char* vehicleName)`
- **Impacted Functionality:** Telemetry logging metadata and potentially GUI display of the car brand.

## Proposed Changes

### `src/VehicleUtils.cpp`
- Updated `ParseVehicleBrand` to:
    - Include "P325" in the Ligier check.
    - Include "D09" in the Duqueine check.
    - Added a check for LMP2 class: if `className` contains "LMP2" and the brand is still "Unknown", return "Oreca".
    - Added class-based fallbacks for Ginetta, Ligier, and Duqueine.

## Test Plan
- **New Test File:** `tests/test_issue_270_brand_detection.cpp`
- **Test Cases:**
    1.  `LMP2` class with non-Oreca vehicle name (e.g. "Gibson GK428") should return "Oreca". (Passed)
    2.  `LMP3` class with "Ligier JS P325" should return "Ligier". (Passed)
    3.  `LMP3` class with "Duqueine D09" should return "Duqueine". (Passed)
    4.  Verify existing brands (Ferrari, Toyota, etc.) still work. (Passed)
    5.  Verify class name fallbacks like "LMP3 (Ligier)". (Passed)

## Deliverables
- [x] Modified `src/VehicleUtils.cpp`
- [x] New `tests/test_issue_270_brand_detection.cpp`
- [x] Updated `VERSION` file (incremented to 0.7.144)
- [x] Updated `CHANGELOG_DEV.md`
- [x] Code review records `docs/dev_docs/code_reviews/issue_270_review_iteration_1.md`

## Implementation Notes
- **LMP3 Keyword Mapping:** Added `P325` for Ligier and `D09` for Duqueine as requested.
- **LMP2 Fallback:** Confirmed that most LMP2 cars in LMU use the Gibson engine but the chassis is almost exclusively Oreca. Defaulting to "Oreca" for the `LMP2` class is the correct behavior for log organization.
- **Class Fallback:** Improved the backup check at the end of `ParseVehicleBrand` to catch "LMP3 (Ligier)" and similar patterns.
- **Testing:** All 11 assertions in the new test suite passed.
- **Code Review:** Received feedback on missing administrative steps (VERSION/CHANGELOG). These were addressed in the second iteration.
