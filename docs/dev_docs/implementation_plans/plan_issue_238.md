# Implementation Plan - Fix: avoid spamming the console (v0.7.116+) #238

The goal of this task is to prevent the console from being spammed with FFB normalization and vehicle identification messages. These messages should only appear once when a vehicle change is detected.

## Design Rationale
The current vehicle change detection in `FFBEngine::calculate_force` is vulnerable to re-triggering if the internal state (`m_vehicle_name`) is not immediately synchronized with the triggering input (`vehicleName` from scoring info). Furthermore, the optimization used to update context strings for UI/Logging is brittle and can lead to stale state. By ensuring immediate synchronization in the initialization path and using robust string comparisons, we guarantee that car-change logic executes exactly once per change, improving system reliability and reducing log noise.

## Proposed Changes

### 1. FFB Engine Seeding Logic (`src/GripLoadEstimation.cpp`)
- **Technical Change:** Update `m_vehicle_name` inside `FFBEngine::InitializeLoadReference`.
- **Design Rationale:** `InitializeLoadReference` is the logical authority for vehicle initialization. By updating `m_vehicle_name` here, we immediately satisfy the car-change detection condition for subsequent frames, preventing the logic from re-running.
- **Affected Function:** `FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName)`
- **Implementation:**
    ```cpp
    #ifdef _WIN32
             strncpy_s(m_vehicle_name, sizeof(m_vehicle_name), vehicleName ? vehicleName : "Unknown", _TRUNCATE);
    #else
             strncpy(m_vehicle_name, vehicleName ? vehicleName : "Unknown", STR_BUF_64 - 1);
             m_vehicle_name[STR_BUF_64 - 1] = '\0';
    #endif
    ```

### 2. Context String Synchronization (`src/FFBEngine.cpp`)
- **Technical Change:** Replace the partial character index check with a full `strcmp` for `m_vehicle_name` and `m_track_name`.
- **Design Rationale:** The current check (`m_vehicle_name[0] != ... || m_vehicle_name[VEHICLE_NAME_CHECK_IDX] != ...`) is a micro-optimization that can fail if two different vehicle names share the same characters at those specific indices. `strcmp` is robust and its performance cost is negligible at the physics frequency.
- **Affected Function:** `FFBEngine::calculate_force`
- **Implementation:**
    ```cpp
    if (strcmp(m_vehicle_name, upsampled_data->mVehicleName) != 0 || strcmp(m_track_name, upsampled_data->mTrackName) != 0) {
        // ... (strncpy logic)
    }
    ```

### 3. Version and Changelog
- Increment version in `VERSION` to `0.7.119`.
- Update `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.

## Test Plan

### 1. Reproduction & Verification Test (`tests/test_issue_238_spam.cpp`)
- **Test Description:** Verifies that normalization messages are printed only once when a vehicle is detected, even if `calculate_force` is called repeatedly.
- **Test Steps:**
    1. Redirect `std::cout` to a string stream.
    2. Call `calculate_force` 10 times with "Car A".
    3. Verify "[FFB] Normalization state reset" appears exactly once.
    4. Call `calculate_force` with "Car B".
    5. Verify the message appears a total of 2 times.
- **Design Rationale:** Capturing standard output is the most direct way to prove that the "spamming" behavior is resolved.

### 2. Regression Testing
- Run all existing tests using `./build/tests/run_combined_tests`.
- **Design Rationale:** Ensures that changes to vehicle initialization and string handling do not break existing physics or logic tests.

## Deliverables
- [x] Modified `src/GripLoadEstimation.cpp`
- [x] Modified `src/FFBEngine.cpp`
- [x] New `tests/test_issue_238_spam.cpp`
- [x] Updated `VERSION`, `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`
- [x] Implementation Notes updated in this plan.

## Implementation Notes
- **Encountered Issues:** The `TEST_CASE_TAGGED` macro in `test_ffb_common.h` required the `tags` argument to be wrapped in parentheses to avoid being parsed as multiple macro arguments when using initializer lists.
- **Deviations from Plan:** None.
- **Future Recommendations:** Consider a more centralized approach to vehicle state management to avoid redundant string copies and comparisons.
