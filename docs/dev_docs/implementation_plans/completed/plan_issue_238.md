# Implementation Plan - Fix: avoid spamming the console (v0.7.116+) #238

The goal of this task is to prevent the console from being spammed with FFB normalization, vehicle identification messages, and periodic diagnostic logs.

## Design Rationale (v0.7.119)
The current vehicle change detection in `FFBEngine::calculate_force` is vulnerable to re-triggering if the internal state (`m_vehicle_name`) is not immediately synchronized with the triggering input (`vehicleName` from scoring info). Furthermore, the optimization used to update context strings for UI/Logging is brittle and can lead to stale state. By ensuring immediate synchronization in the initialization path and using robust string comparisons, we guarantee that car-change logic executes exactly once per change, improving system reliability and reducing log noise.

## Design Rationale (v0.7.120)
Excessive logging, especially at high frequencies or periodically during normal operation, clutters the console and can slightly impact performance. By removing non-essential logs (like window resize events and periodic telemetry stats) and reducing the redundancy/frequency of health warnings, we improve the user experience and keep the logs focused on critical events.

## Proposed Changes

### 1. FFB Engine Seeding Logic (`src/GripLoadEstimation.cpp`) - v0.7.119
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

### 2. Context String Synchronization (`src/FFBEngine.cpp`) - v0.7.119
- **Technical Change:** Replace the partial character index check with a full `strcmp` for `m_vehicle_name` and `m_track_name`.
- **Design Rationale:** The current check (`m_vehicle_name[0] != ... || m_vehicle_name[VEHICLE_NAME_CHECK_IDX] != ...`) is a micro-optimization that can fail if two different vehicle names share the same characters at those specific indices. `strcmp` is robust and its performance cost is negligible at the physics frequency.
- **Affected Function:** `FFBEngine::calculate_force`
- **Implementation:**
    ```cpp
    if (strcmp(m_vehicle_name, upsampled_data->mVehicleName) != 0 || strcmp(m_track_name, upsampled_data->mTrackName) != 0) {
        // ... (strncpy logic)
    }
    ```

### 3. Remove Window Resize Logging (`src/GuiLayer_Win32.cpp`) - v0.7.120
- **Technical Change:** Remove the `Logger::Get().Log("ResizeBuffers: ...")` call in `WndProc`.
- **Design Rationale:** Window resizing is a common UI action and doesn't need to be logged to the console/file in production builds.

### 4. Remove Periodic Telemetry Rate Logging (`src/main.cpp`) - v0.7.120
- **Technical Change:** Remove the block in `FFBThread` that logs "--- Telemetry Sample Rates (Hz) ---" every 5 seconds.
- **Design Rationale:** These rates are already visible in the GUI (Graphs/Diagnostics) and don't need to be dumped to the console periodically unless specifically requested for debugging.

### 5. Refine Low Sample Rate Warnings (`src/main.cpp`) - v0.7.120
- **Technical Change:**
    - Increase the warning interval from 5 seconds to 60 seconds to reduce spam.
    - Remove the redundant `std::cout` call before `Logger::Get().Log`, as the logger already prints to `std::cout`.
- **Design Rationale:** A 5-second interval is too frequent for a persistent condition. 60 seconds still informs the user without flooding the console. Removing the duplicate print prevents "double logging".

### 6. Version and Changelog
- Increment version in `VERSION` to `0.7.120`.
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

### 2. Verification of Reduced Logging (v0.7.120)
- **Manual Verification (Conceptual):** Resize the window and verify no "ResizeBuffers" messages appear. Run the app and verify no periodic telemetry blocks appear every 5 seconds.

### 3. Regression Testing
- Run all existing tests using `./build/tests/run_combined_tests`.
- **Design Rationale:** Ensures that changes do not break existing physics or logic tests.

## Deliverables
- [x] Modified `src/GripLoadEstimation.cpp` (v0.7.119)
- [x] Modified `src/FFBEngine.cpp` (v0.7.119)
- [x] New `tests/test_issue_238_spam.cpp` (v0.7.119)
- [x] Modified `src/GuiLayer_Win32.cpp` (v0.7.120)
- [x] Modified `src/main.cpp` (v0.7.120)
- [x] Updated `VERSION`, `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`
- [x] Implementation Notes updated in this plan.

## Implementation Notes
- **Encountered Issues:**
    - The `TEST_CASE_TAGGED` macro in `test_ffb_common.h` required the `tags` argument to be wrapped in parentheses to avoid being parsed as multiple macro arguments when using initializer lists.
    - (v0.7.120) A redundant `now` variable was present in `src/main.cpp` after the periodic logging block removal; it was correctly removed during the patch application to maintain code cleanliness.
- **Code Review Feedback:**
    - Iteration 1: The review was positive, identifying the solution as correct and safe. It noted a minor documentation oversight where checkboxes for v0.7.120 were not marked as completed. This was addressed in the final documentation update.
- **Deviations from Plan:** None.
- **Future Recommendations:** Consider a more centralized approach to vehicle state management to avoid redundant string copies and comparisons. For diagnostics, consider a "Debug Console" in the GUI or an optional command-line flag to re-enable high-frequency console logging if needed for troubleshooting.
