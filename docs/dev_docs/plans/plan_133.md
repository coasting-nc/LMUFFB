# Implementation Plan - Telemetry Sample Rate Monitoring (Issue #133)

**Context:**
The goal is to increase telemetry monitoring by tracking the sample rates of various individual telemetry channels (fields) from Le Mans Ultimate (LMU) and logging these rates to both the console and the debug log file. This will help determine if some channels are updated at 400Hz while others are at 100Hz.

**Reference Documents:**
- `docs/telemetry_monitoring_report.md`

**Codebase Analysis Summary:**
- **Current Architecture:** The `FFBThread` in `src/main.cpp` polls shared memory at 400Hz. It uses `RateMonitor` (defined in `src/RateMonitor.h`) to track rates by checking for value changes in `mElapsedTime`, `mSteeringShaftTorque`, and `FFBTorque`.
- **Impacted Functionalities:**
    - `FFBThread`: Additional monitoring logic will be added here.
    - `RateMonitor`: Used for tracking.
    - `Logger`: Used for outputting the results to console and file.

**FFB Effect Impact Analysis:**
- **Affected FFB Effects:** None. This is a monitoring-only task.
- **User Perspective:** No change in feel. Users will see more detailed sample rate information in their logs, helping with troubleshooting "dull" FFB or low-frequency updates.

**Proposed Changes:**

### 1. `src/main.cpp` - Extend `FFBThread` monitoring
- Add additional `RateMonitor` instances for the following channels:
    - `mLocalAccel` (X, Y, Z)
    - `mLocalVel` (X, Y, Z)
    - `mLocalRot` (X, Y, Z)
    - `mLocalRotAccel` (X, Y, Z)
    - `mUnfilteredSteering`
    - `mFilteredSteering`
    - `mEngineRPM`
    - `mWheel[0].mTireLoad` (and other wheels)
    - `mWheel[0].mLateralForce` (and other wheels)
    - `mPos.x`
    - `mDeltaTime`
- Add state variables (e.g., `lastAccelX`, `lastVelX`, etc.) to track the last seen value for each channel.
- Inside the `if (in_realtime && ...)` block in `FFBThread`:
    - Update each `RateMonitor` by comparing current values with state variables.
- Implement a periodic logging block:
    - Use a `static auto lastLogTime` and `std::chrono::steady_clock`.
    - Every 5 seconds, log a multi-line summary of all rates using `Logger::Get().Log`.

### 2. Version Increment
- Increment `VERSION` file: `0.7.49` -> `0.7.50`.
- Increment `src/Version.h` accordingly.

**Test Plan:**

### Verification
- **Manual Verification:** Run the application (even in headless mode if a shared memory mock is available) and verify that `lmuffb_debug.log` contains the new sample rate logs.
- **Log Inspection:** Ensure the format is readable and all requested channels are present.
- **Code Review:** Ensure no performance regression is introduced by the additional comparisons (minimal impact expected).

**Deliverables:**
- [x] Code changes in `src/main.cpp`.
- [x] Updated `VERSION` and `src/Version.h`.
- [x] Implementation Notes in `docs/dev_docs/plans/plan_133.md` (updated after implementation).

---

### Implementation Notes
- **Unforeseen Issues:** None.
- **Plan Deviations:** Added a `ChannelMonitor` helper struct in `FFBThread` to reduce boilerplate when updating multiple telemetry channels. Added a unit test in `tests/test_rate_monitor.cpp` to verify this helper logic.
- **Challenges:** Ensuring the `RateMonitor` window timing was correctly handled in the new unit test.
- **Recommendations:** Once logs from users are collected, we should analyze if specific telemetry channels (like `mSteeringShaftTorque` vs `FFBTorque`) consistently provide 400Hz updates to decide on the best default torque source.

---

### Parameter Synchronization Checklist
N/A (No new settings added)

### Initialization Order Analysis
N/A (Changes limited to `main.cpp` local variables in `FFBThread`)
