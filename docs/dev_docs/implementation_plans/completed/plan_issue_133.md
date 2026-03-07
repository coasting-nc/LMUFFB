# Implementation Plan - Issue #133: Fix: [WARNING] Low Sample Rate detected

## Context
Issue #133 reports that users are receiving frequent "[WARNING] Low Sample Rate detected" messages in the console and logs, even when their system is performing as expected for standard LMU telemetry (100Hz). The previous logic hardcoded a 400Hz target for all monitored channels, which is incorrect for legacy Shaft Torque and standard Telemetry.

## Reference Documents
- GitHub Issue #133: https://github.com/coasting-nc/LMUFFB/issues/133

## Codebase Analysis Summary
- **main.cpp (FFBThread):** Previously contained hardcoded rate checks. Updated to use `HealthMonitor`.
- **FFBEngine.h:** Provides `m_torque_source` setting.
- **RateMonitor.h:** Provides Hz monitoring.
- **HealthMonitor.h (New):** Encapsulates source-aware health check logic.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
| :--- | :--- | :--- |
| **Console Warnings** | Thresholds adjusted based on source. | Fewer false-positive warnings, better trust in the system. |

## Proposed Changes

### 1. Logic Extraction (`src/HealthMonitor.h`)
- Created `HealthMonitor` class to encapsulate rate health rules.
- **Loop Rate:** Target 400Hz. Warn < 360Hz.
- **Telemetry Rate:** Target 100Hz. Warn < 90Hz.
- **Torque Rate:**
  - Source 0 (Legacy): Target 100Hz. Warn < 90Hz.
  - Source 1 (Direct): Target 400Hz. Warn < 360Hz.

### 2. Main Loop Update (`src/main.cpp`)
- Integrated `HealthMonitor::Check` into `FFBThread`.
- Added `std::lock_guard` for thread-safe access to engine settings.
- Improved warning message clarity with specific details of which channel is low.

### 3. Versioning
- Incremented `VERSION` to `0.7.64`.
- Updated `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.

## Test Plan
- Created `tests/test_health_monitor.cpp` with 7 test cases covering:
  - Healthy states (Direct & Legacy).
  - Low Loop Rate.
  - Low Telemetry Rate.
  - Low Torque Rate (Direct).
  - Borderline cases (380Hz for Direct, 95Hz for Telemetry).

## Deliverables
- [x] Modified `src/main.cpp`
- [x] New `src/HealthMonitor.h`
- [x] New `tests/test_health_monitor.cpp`
- [x] Updated `VERSION`, `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`.

## Implementation Notes
### Unforeseen Issues
- Initial build of `tests/test_health_monitor.cpp` failed due to missing `using namespace FFBEngineTests;`, which was required for the `ASSERT_*` macros. Fixed immediately.

### Plan Deviations
- Decided to put `HealthMonitor` in its own file `src/HealthMonitor.h` rather than `RateMonitor.h` to keep it modular, as suggested by the plan review feedback.

### Challenges
- Ensuring the warning message was informative but concise. Added logic to only list the specific failing channels in the console output.

### Recommendations
- Future iterations could expose these thresholds in the "Advanced Settings" GUI if users find them too strict or too loose for their specific hardware/network conditions.
