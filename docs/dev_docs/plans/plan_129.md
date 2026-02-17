# Implementation Plan - Issue #129: FFB Sample Rate Verification & Monitoring (v2)

## Context
Address user concerns regarding a reduced FFB sample rate (100Hz instead of 400Hz). Implement verification, monitoring, and precise timing in the FFB loop.

## Reference Documents
- [Investigation Report - Issue #129](../investigations/investigation_report_issue_129.md)

## Codebase Analysis Summary
- **Current Architecture**: The FFB loop runs in `FFBThread` in `main.cpp`, using a fixed 2ms sleep. Telemetry is polled from shared memory. Effects are calculated in `FFBEngine.h`.
- **Impacted Functionalities**:
    - `FFBThread` (`main.cpp`): Main loop timing logic.
    - `FFBEngine` (`src/FFBEngine.h`): Stores and exposes rate statistics via `FFBSnapshot`.
    - `GuiLayer` (`src/GuiLayer_Common.cpp`): Displays rates in the Tuning window.
    - `DirectInputFFB` (`src/DirectInputFFB.cpp`): Update frequency target.

## FFB Effect Impact Analysis
| Effect | Technical Changes | Expected User Change |
| :--- | :--- | :--- |
| All Effects | Higher/more consistent update frequency. | Smoother, more detailed feel. Reduced jitter. |
| Road Texture | Better high-frequency fidelity. | Sharper road feel, less aliasing. |
| Lockup Vibration | More consistent vibration pitch. | Clearer tactile feedback. |

## Proposed Changes

### 1. Timing & Monitoring Logic (`src/RateMonitor.h`)
- Define a `RateMonitor` class to track event frequencies over a 1-second sliding window.
- Use `std::chrono::steady_clock` for timing.

### 2. `src/main.cpp`
- **Precise Loop Timing**: Implement dynamic sleep to maintain a steady 2.5ms (400Hz) interval.
- **Rate Tracking**: Record Loop, Telemetry, and Hardware events.
- **Warning System**: Log to console/logger if telemetry rate < 380Hz for 3+ seconds during a session.

### 3. `src/FFBEngine.h`
- Add member variables for `m_ffb_rate`, `m_telemetry_rate`, `m_hw_rate`.
- Update `FFBSnapshot` struct to include these fields.
- Update `calculate_force` to receive and store these rates.

### 4. `src/GuiLayer_Common.cpp`
- Add "Telemetry Status" section to Tuning Window.
- Display FFB, Telemetry, and Hardware rates with status colors.

## Version Increment Rule
- Increment version in `VERSION` by smallest possible increment (+1 to rightmost number).

## Test Plan
- **test_rate_monitor**: Create `tests/test_rate_monitor.cpp`. Verify frequency calculation with mocked timestamps.
- **Regression**: Run all tests using `build_commands.txt` instructions.

## Deliverables
- [ ] `src/RateMonitor.h`
- [ ] Updated `src/main.cpp`
- [ ] Updated `src/FFBEngine.h`
- [ ] Updated `src/GuiLayer_Common.cpp`
- [ ] Updated `VERSION` and `CHANGELOG_DEV.md`
- [ ] New `tests/test_rate_monitor.cpp`
