# Implementation Plan - Issue #184: Soft Lock in Menus and Stationary

## Context
Issue #184: Fix soft lock should work when the car is stationary as well.
Currently, Soft Lock is disabled when the game is paused or in the Esc menu because the FFB thread in `main.cpp` explicitly zeroes the output force when `in_realtime` is false. Additionally, telemetry is marked as "stale" when the game is paused because the elapsed time doesn't increment, causing the FFB loop to skip force calculations entirely.

## Design Rationale
Soft Lock is a critical safety and immersion feature that prevents the physical steering wheel from rotating beyond the simulated car's steering rack limits. This protection should remain active whenever the player is "in" the vehicle, regardless of whether the simulation is actively running (realtime) or paused.

To achieve this:
1.  **Heartbeat from Input:** We will treat changes in the user's steering input as a "heartbeat" for telemetry freshness. This ensures that as long as the user is moving the wheel, the FFB engine continues to process and output the Soft Lock force, even if the game's simulation time is frozen.
2.  **Selective Muting:** We rely on `FFBEngine::calculate_force`'s existing logic to mute all physics-based and vibration effects when `allowed` is false (which happens in menus), while explicitly preserving the `soft_lock_force`.
3.  **Removal of Redundant Zeroing:** The explicit `force = 0.0` override in `main.cpp` for non-realtime states is removed to allow the Soft Lock force to reach the hardware.

## Codebase Analysis Summary
-   **`src/GameConnector.cpp` / `src/GameConnector.h`**: Responsible for determining if telemetry is "stale". Currently only uses `mElapsedTime`.
    -   *Impact:* Needs to also monitor `mUnfilteredSteering`.
    -   *Rationale:* Steering movement indicates the user is interacting with the wheel and needs feedback/protection.
-   **`src/main.cpp`**: The main FFB loop (`FFBThread`). Explicitly zeroes force if `!in_realtime`.
    -   *Impact:* Remove the zeroing line. Ensure `full_allowed` correctly signals the engine to mute other effects.
    -   *Rationale:* `FFBEngine` is already designed to handle partial muting safely.
-   **`src/FFBEngine.cpp`**: `calculate_force` handles the `!allowed` state.
    -   *Impact:* Already preserves `soft_lock_force`, but we must ensure `full_allowed` in `main.cpp` is passed correctly.

## FFB Effect Impact Analysis
| Effect | Technical Change | User-Facing Change |
| :--- | :--- | :--- |
| **Soft Lock** | Will now be active in menus and when stationary. | The wheel will have a firm stop at the car's steering limit even when paused. |
| **All Other Effects** | Remains muted in menus via `full_allowed = false`. | No change; road noise, bumps, etc. are still muted when paused. |

## Proposed Changes

### 1. VERSION Increment
-   Update `VERSION` from `0.7.117` to `0.7.118`.

### 2. src/GameConnector.h
-   Add `double m_lastSteer = 0.0;` to the private members of `GameConnector`.

### 3. src/GameConnector.cpp
-   In `GameConnector::CopyTelemetry`:
    -   Retrieve `mUnfilteredSteering` from the player's telemetry.
    -   If `mUnfilteredSteering != m_lastSteer`, update `m_lastSteer` and set `m_lastUpdateLocalTime = std::chrono::steady_clock::now();`.
    -   This prevents the `IsStale()` check from returning `true` while the user is turning the wheel in menus.

### 4. src/main.cpp
-   In `FFBThread`:
    -   Locate the line: `if (!in_realtime) force = 0.0;` (added in v0.7.108 for Issue #174).
    -   Remove this line.
    -   Verify `full_allowed` calculation: `bool full_allowed = g_engine.IsFFBAllowed(scoring, g_localData.scoring.scoringInfo.mGamePhase) && in_realtime;`. This ensures `calculate_force` receives `false` when in menus, triggering its internal muting of all effects *except* Soft Lock.

## Test Plan (TDD-Ready)

### Design Rationale
We need to verify that:
1.  Soft Lock is active when `in_realtime` is false.
2.  Other effects are muted when `in_realtime` is false.
3.  `GameConnector` stays "fresh" when steering changes even if time is frozen.

### New Test: `tests/test_issue_184_softlock.cpp`
-   **Test Case 1: Soft Lock in Menu**
    -   Input: `in_realtime = false`, `full_allowed = false`, `steering = 1.1` (beyond limit).
    -   Expected: `calculate_force` returns a non-zero negative force (Soft Lock pushing back).
-   **Test Case 2: Muting in Menu**
    -   Input: `in_realtime = false`, `full_allowed = false`, `road_noise` active.
    -   Expected: `calculate_force` returns only Soft Lock (or 0 if within limits), and `snap.texture_road` is 0.
-   **Test Case 3: GameConnector Heartbeat (Mocked)**
    -   Simulate `CopyTelemetry` with same `mElapsedTime` but different `mUnfilteredSteering`.
    -   Verify `IsStale(100)` returns `false` after a short wait that would otherwise trigger staleness.

## Deliverables
-   [x] Modified `src/GameConnector.h` and `src/GameConnector.cpp`.
-   [x] Modified `src/main.cpp`.
-   [x] New test file `tests/test_issue_184_softlock.cpp`.
-   [x] Updated `VERSION` and `CHANGELOG_DEV.md`.
-   [x] Implementation Notes in this plan.

## Implementation Notes

### Unforeseen Issues
- The code review pointed out missing mandatory files (`VERSION`, `CHANGELOG_DEV.md`) and version inconsistencies which were promptly addressed.
- The initial plan missed including `thread` header in the new test file, causing a build error which was fixed.
- CI failures on Windows revealed that the heartbeat test was sensitive to background thread interference and required more robust mock initialization (specifically setting up events to allow `CopyTelemetry` to succeed).

### Plan Deviations
- Expanded the test plan to include a functional mock test for the `GameConnector` heartbeat logic as recommended by the code review.
- Added a `Disconnect()` call at the start of the heartbeat test and reset heartbeat state in `GameConnector::_DisconnectLocked()` to ensure test isolation.
- Removed some version-specific comments in `main.cpp` and `FFBEngine.cpp` that were redundant or confusing (as pointed out in the review).

### Challenges
- Testing `GameConnector` logic on Linux required setting up a mock shared memory segment using `CreateFileMappingA` and `MapViewOfFile` which are part of the mock layer.

### Recommendations
- The heartbeat logic relies on steering input changes. While effective for active users, a user holding the wheel perfectly steady might still hit the timeout. However, sensor noise usually prevents this in practice. A more robust solution might involve the game plugin providing a "menu active" heartbeat.
