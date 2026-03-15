# Implementation Plan - Fix Porsche Brand Detection and Improve Diagnostics (#368)

## 1. Analysis of Issue #368
- **Problem:** Porsche LMGT3 cars (specifically "Proton Competition" entries) are detected as "Unknown" brand.
- **Evidence:** User log shows: `Vehicle: Proton Competition 2025 #60:ELMS`, `Car Class: GT3`, `Car Brand: Unknown`.
- **Findings:**
    - The existing `ParseVehicleBrand` in `src/physics/VehicleUtils.cpp` uses keywords like "PORSCHE", "911", "RSR".
    - "Proton Competition" and "Manthey" entries often lack "Porsche" in the vehicle name string in some game packs (like ELMS).
    - Hidden whitespace in telemetry strings might also interfere with matching.
- **Proposed Solution:**
    - Harden string parsing with a `Trim` helper.
    - Expand Porsche detection keywords to include "992" (modern GT3 chassis), "PROTON" (common team name), and "MANTHEY" (factory-supported team).
    - Improve diagnostic logging by recording more telemetry fields (`mVehicleClass`, `mVehicleName`, `mPitGroup`, `mVehFilename`) when a vehicle change occurs.
    - Use quoted strings in logs to reveal hidden whitespace.

## 2. Proposed Changes
- **Robust Parsing and Keywords:**
    - Added `Trim` helper to `src/physics/VehicleUtils.cpp`.
    - Added "992", "PROTON", and "MANTHEY" to Porsche keywords in `ParseVehicleBrand`.
- **Enhanced Diagnostics:**
    - Updated `FFBMetadataManager::UpdateMetadata` to log `mVehicleClass`, `mVehicleName`, `mPitGroup`, and `mVehFilename` on vehicle change.
    - Updated `FFBEngine::InitializeLoadReference` to log strings with single quotes.

## 3. Verification Plan
- **Automated Tests:**
    - `tests/test_vehicle_utils.cpp` updated with Porsche LMGT3 cases.
    - Full suite verified: `./build/tests/run_combined_tests`.
- **Manual Verification:**
    - Verified `Trim` logic and keyword matching on Linux.

## 4. Final Implementation Notes
- The fix correctly identifies "Proton Competition" as Porsche.
- Enhanced logging will significantly simplify debugging for any future "Unknown" brand reports by showing all raw strings received from the game.
- The `Trim` robustness is applied globally to brand and class parsing.

## 5. Shared Memory Field Analysis (Brand Information)

| Field Name | Struct | Used? | Raw Printed? | Parsed? | Parsed Version Printed? | Notes |
|------------|--------|-------|--------------|---------|-------------------------|-------|
| `mVehicleName` | `TelemInfoV01`, `VehicleScoringInfoV01` | Yes | No | Yes (Trim, Upper) | Yes | Primary source for brand keywords (e.g., "Porsche", "Cadillac"). |
| `mVehicleClass` | `VehicleScoringInfoV01` | Yes | No | Yes (Trim, Upper) | Yes | Used for class-based brand fallbacks (e.g., LMP2 -> Oreca). |
| `mPitGroup` | `VehicleScoringInfoV01` | Yes (Logging only) | No | No | Yes | Often contains team names which can imply brands (e.g., "Proton Competition"). |
| `mVehFilename` | `VehicleScoringInfoV01` | Yes (Logging only) | No | No | Yes | The `.veh` file path/name often contains brand or team strings. |
| `mTrackName` | `TelemInfoV01`, `ScoringInfoV01` | Yes | No | No | Yes | Context only, unlikely to contain car brand info. |
| `mFrontTireCompoundName` / `mRearTireCompoundName` | `TelemInfoV01` | No | No | No | No | Unlikely to contain brand info, but sometimes mentions specific manufacturers. |
| `mResultsStream` | `ScoringInfoV01` | No | No | No | No | XML/Text stream containing session results; may contain brand info but expensive to parse. |

### Analysis of Parsing Impacts:
- **`Trim` & `ToUpper`:** Our current parsing uses `Trim` and `ToUpper`. This is **safe** as brand names are generally alphanumeric and Case-Insensitive matching is the goal. Trimming removes leading/trailing spaces which were identified as a primary cause for "Unknown" detections despite keywords being present.
- **Lost Information:** We do NOT currently discard any specific brand-identifying information during parsing. The use of `std::string::find` allows us to match substrings (keywords) anywhere within the provided fields.

## 6. Submission
- VERSION incremented to `0.7.189`.
- CHANGELOG updated with Issue #368 fix.
