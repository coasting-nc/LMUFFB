# Implementation Plan - Fix Porsche Brand Detection and Improve Diagnostics (#368)

## 1. Analysis of Issue #368
- **Problem:** Porsche LMGT3 cars (specifically "Proton Competition" entries) are detected as "Unknown" brand.
- **Evidence:** User log shows: `Vehicle: Proton Competition 2025 #60:ELMS`, `Car Class: GT3`, `Car Brand: Unknown`.
- **Findings:**
    - The existing `ParseVehicleBrand` in `src/physics/VehicleUtils.cpp` uses keywords like "PORSCHE", "911", "RSR".
    - "Proton Competition" and "Manthey" entries often lack "Porsche" in the vehicle name string in some game packs (like ELMS).
    - LMU specific car class for GT3-like cars is "LMGT3", which was previously being mapped to generic GT3 without distinction.
    - Hidden whitespace in telemetry strings was identified as a likely contributor to matching failures.
- **Proposed Solution:**
    - Harden string parsing with a `Trim` helper.
    - Expand Porsche detection keywords to include "992" (modern GT3 chassis), "PROTON" (common team name), and "MANTHEY" (factory-supported team).
    - Explicitly support the `LMGT3` vehicle class in `VehicleUtils` with LMU-specific seed loads (5000N).
    - Improve diagnostic logging by recording more telemetry fields (`mVehicleClass`, `mVehicleName`, `mPitGroup`, `mVehFilename`) when a vehicle change occurs.
    - Use quoted strings in logs to reveal hidden whitespace.

## 2. Proposed Changes
- **Robust Parsing and Keywords:**
    - Added `Trim` helper to `src/physics/VehicleUtils.cpp`.
    - Added "992", "PROTON", and "MANTHEY" to Porsche keywords in `ParseVehicleBrand`.
- **LMGT3 Class Support:**
    - Added `LMGT3` to `ParsedVehicleClass` enum.
    - Updated `ParseVehicleClass` to distinguish between "LMGT3" and "GT3" (checking "LMGT3" first to avoid partial matches).
    - Updated all physics lookup tables (Seed Load, Motion Ratio, Unsprung Mass) to support `LMGT3`.
- **Enhanced Diagnostics:**
    - Updated `FFBMetadataManager::UpdateMetadata` to log `mVehicleClass`, `mVehicleName`, `mPitGroup`, and `mVehFilename` whenever a vehicle change is detected.
    - Updated `FFBEngine::InitializeLoadReference` to log strings with single quotes to reveal whitespace.

## 3. Verification Plan
- **Automated Tests:**
    - `tests/test_vehicle_utils.cpp` updated with Porsche LMGT3 cases and `LMGT3` class verification.
    - `tests/test_ffb_load_normalization.cpp` updated to verify `LMGT3` class seeding and avoid regressions in GT3 mapping.
    - Full suite verified: `./build/tests/run_combined_tests`.
- **Manual Verification:**
    - Verified `Trim` logic and keyword matching on Linux using the updated test suite.

## 4. Implementation Notes

### Issues and Challenges
- **Ambiguous Strings:** Some game packs for LMU (like ELMS) use team names like "Proton Competition" without including "Porsche" in the vehicle name field, which caused the legacy keyword-based detection to fail.
- **Identification Examples:** Enhanced logging revealed the following metadata for the affected Porsche:
  `[17:01:48] [Metadata] Vehicle Change Detected: 'Proton Competition 2025 #60:ELMS' (Class: 'GT3', PitGroup: 'Group38', Filename: '60_25_PROTA8B000C7')`
  This confirms that some cars in LMU still report "GT3" in the class field, requiring our parser to map both "LMGT3" and "GT3" to the new internal `LMGT3` class.
- **Hidden Whitespace:** Sim racing telemetry strings often contain trailing spaces or non-printable characters. The introduction of the `Trim` helper was critical to ensure robust matching.
- **Class vs. Brand:** There was initial confusion between car class (LMGT3) and brand (Porsche). LMU uses "LMGT3" as a distinct class from "GT3", requiring separate handling for physics constants (seed load).
- **Sub-string Collision:** Since "LMGT3" contains "GT3", the parser must check for the more specific "LMGT3" string first to avoid incorrectly identifying LMU cars as legacy GT3.

### Deviations from Plan
- **Class Differentiation:** The plan was expanded to explicitly support `LMGT3` as a distinct class after identifying that LMU-specific GT3-like cars have different specs (5000N load) than generic GT3 cars.
- **Improved Metadata Logging:** Added logging for `mPitGroup` and `mVehFilename` in `FFBMetadataManager` to provide a complete "Identification Snapshot" whenever a vehicle change is detected.
- **REST API Integration:** Added asynchronous retrieval of car manufacturer from the REST API (`/rest/race/car`) upon vehicle change. This provides a clean "Ground Truth" for brand identification even when Shared Memory strings are cryptic.

## 6. Related Documentation
- [REST API Car Info Specification](../reports/get%20car%20info%20from%20REST%20API.md)

## 7. Shared Memory Field Analysis (Brand Information)

| Field Name | Struct | Used? | Raw Printed? | Parsed? | Parsed Version Printed? | Notes |
|------------|--------|-------|--------------|---------|-------------------------|-------|
| `mVehicleName` | `TelemInfoV01`, `VehicleScoringInfoV01` | Yes | No | Yes (Trim, Upper) | Yes (Quoted) | Primary source for brand keywords (e.g., "Porsche", "Cadillac"). |
| `mVehicleClass` | `VehicleScoringInfoV01` | Yes | No | Yes (Trim, Upper) | Yes (Quoted) | Used for class-based brand fallbacks (e.g., LMP2 -> Oreca). |
| `mPitGroup` | `VehicleScoringInfoV01` | Yes (Logging) | No | No | Yes (Quoted) | Often contains team names which can imply brands (e.g., "Proton Competition"). |
| `mVehFilename` | `VehicleScoringInfoV01` | Yes (Logging) | No | No | Yes (Quoted) | The `.veh` file path/name often contains brand or team strings. |
| `manufacturer` | REST API (`/rest/race/car`) | Yes | No | No | Yes (Quoted) | Clean manufacturer name retrieved via async matching of `mVehicleName` against `desc`. |
| `mTrackName` | `TelemInfoV01`, `ScoringInfoV01` | Yes | No | No | Yes | Context only, unlikely to contain car brand info. |
| `mFrontTireCompoundName` / `mRearTireCompoundName` | `TelemInfoV01` | No | No | No | No | Unlikely to contain brand info, but sometimes mentions specific manufacturers. |
| `mResultsStream` | `ScoringInfoV01` | No | No | No | No | XML/Text stream containing session results; may contain brand info but expensive to parse. |

### Analysis of Parsing Impacts:
- **`Trim` & `ToUpper`:** Our current parsing uses `Trim` and `ToUpper`. This is **safe** as brand names are generally alphanumeric and Case-Insensitive matching is the goal. Trimming removes leading/trailing spaces which were identified as a primary cause for "Unknown" detections despite keywords being present.
- **Lost Information:** We do NOT currently discard any specific brand-identifying information during parsing. The use of `std::string::find` allows us to match substrings (keywords) anywhere within the provided fields.
- **Logging Strategy:** By logging `mPitGroup` and `mVehFilename` on car change, we capture the most likely secondary sources of brand information without the complexity of parsing them for every frame.

## 6. Submission
- VERSION incremented to `0.7.189`.
- CHANGELOG updated with Issue #368 and LMGT3 support details.
