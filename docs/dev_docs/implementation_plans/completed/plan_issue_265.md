# Implementation Plan - Issue #265: Fix logger unknown car info

The logger currently records "Unknown" car information (vehicle name, class, brand, track) if logging starts before the first physics frame of a session has been processed by the `FFBEngine`. This affects both auto-start logging and potentially manual logging if triggered exactly at the session start.

## User Review Required

> [!IMPORTANT]
> None anticipated. The fix is internal to metadata synchronization.

## Proposed Changes

### 1. FFBEngine Metadata Synchronization

#### [src/FFBEngine.h]
- Add `UpdateMetadata(const SharedMemoryObjectOut& data)` method declaration.
- Add `UpdateMetadataInternal(const char* vehicleClass, const char* vehicleName, const char* trackName)` private method to centralize the update logic used by both `UpdateMetadata` and `calculate_force`.

#### [src/FFBEngine.cpp]
- Implement `UpdateMetadata(const SharedMemoryObjectOut& data)`.
  - It will extract track name from `data.scoring.scoringInfo.mTrackName`.
  - If `data.telemetry.playerHasVehicle` is true, it will extract vehicle name and class from the player's vehicle scoring info.
  - Call `UpdateMetadataInternal`.
- Implement `UpdateMetadataInternal`.
  - Check for changes in track/vehicle.
  - Call `InitializeLoadReference` if vehicle/class changed.
  - Update `m_vehicle_name`, `m_track_name`, `m_current_class_name`.
- Refactor `calculate_force` to use `UpdateMetadataInternal`.

#### [src/GripLoadEstimation.cpp]
- Modify `FFBEngine::InitializeLoadReference` to also update `m_vehicle_name` (the `char[64]` buffer used by GUI/Logger) to ensure consistency.

### 2. Main Loop Metadata Update

#### [src/main.cpp]
- In `FFBThread`, call `g_engine.UpdateMetadata(g_localData)` immediately after successful `GameConnector::Get().CopyTelemetry(g_localData)`.
- In the auto-start logging block, use `g_localData` fields directly instead of relying on `g_engine` metadata, as this provides the most up-to-date information at the moment of session entry.

## Design Rationale

### Why update metadata in FFBThread?
The `FFBThread` receives fresh shared memory data via `CopyTelemetry`. By updating the `FFBEngine` metadata immediately, we ensure that any other component (like the GUI or the auto-logger) sees the correct information even if a physics frame hasn't been calculated yet.

### Why use direct scoring data for auto-start?
The auto-start block triggers on the transition to `in_realtime_phys`. Using the data directly from `g_localData` ensures that we don't have a 1-frame lag where `FFBEngine` might not have processed the `UpdateMetadata` call yet (though in the current plan it would have). It's more robust.

## Test Plan

### Automated Tests
- Create a new test file `tests/test_issue_265_metadata.cpp`.
- Simulate a `SharedMemoryObjectOut` state with a specific car and track.
- Verify that calling `FFBEngine::UpdateMetadata` populates the engine's fields correctly.
- Simulate a car change and verify `InitializeLoadReference` is called (checking via `m_last_handled_vehicle_name`).

### Manual Verification
- Since I cannot run the game, I will rely on unit tests and code review.
- I will verify that the project builds on Linux.

## Implementation Notes

### Challenges & Solutions
- **Race Condition**: The logger was pulling data from `FFBEngine` which only updated during `calculate_force`. By introducing `UpdateMetadata` and calling it immediately after `CopyTelemetry`, we ensure synchronization.
- **REST API Spam**: An initial refactoring caused the `seeded` flag to be true every frame. This was fixed by making `UpdateMetadataInternal` return a boolean only when a change actually occurs.
- **Safety**: Added bounds checking for `playerVehicleIdx` to prevent crashes if the player is not in a car.

### Deviations from Plan
- Centralized `UpdateMetadataInternal` was made to return a `bool` to preserve the "once-per-car-change" trigger logic in `calculate_force`.
- Added `LmuSharedMemoryWrapper.h` include in `FFBEngine.cpp` to provide the definition of `SharedMemoryObjectOut`.

## Pre-commit Checklist
- [x] Ensure proper testing, verification, review, and reflection are done.
