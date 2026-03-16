# Implementation Plan - Issue #384: Fix class of bugs: State Contamination during Context Switches

## Context
Users have reported violent wheel jolts to full lock during session transitions (e.g., Practice to Qualy) or when returning to menus. This is identified as "NaN Infection" where garbage telemetry data from the game corrupts internal smoothing filters, leading to undefined behavior in hardware commands.

## Design Rationale (MANDATORY)
The core philosophy is "Defense in Depth". We implement multiple layers of protection:
1.  **Prevention**: Reject NaN at the input stage for core physics.
2.  **Containment**: Sanitize auxiliary telemetry to allow graceful fallbacks rather than total FFB loss.
3.  **Recovery**: Ensure filters are wiped clean whenever a context switch occurs (bidirectional reset).
4.  **Safety Net**: Intercept NaN at the final output stage before it hits the DirectInput driver.

This approach ensures that even if one layer is bypassed (e.g., a new effect is added that doesn't sanitize its inputs), the hardware remains protected. Using `std::isfinite` is the standard and most reliable way to detect NaNs and Infinities in C++.

## Reference Documents
- GitHub Issue: #384 "Fix class of bugs: State Contamination during Context Switches" (saved in `docs/dev_docs/github_issues/issue_384.md`)

## Codebase Analysis Summary
- **FFBEngine.cpp**: The main physics loop. It contains upsamplers and smoothing filters (EMA) that are stateful and susceptible to NaN infection.
- **DirectInputFFB.cpp**: The hardware interface. It performs the cast from double to long magnitude, which is the trigger for the full-lock bug when NaN is present.

### Impacted Functionalities
- **Telemetry Input**: `calculate_force` reads raw telemetry.
- **Upsampling & Smoothing**: Holt-Winters and EMA filters in `FFBEngine`.
- **Hardware Output**: `DirectInputFFB::UpdateForce` sends the final command to the wheel.

**Design Rationale**: These specific modules are the entry and exit points of the haptic signal. `FFBEngine` is where the state resides, and `DirectInputFFB` is where the dangerous type cast happens.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-facing Changes |
| :--- | :--- | :--- |
| **All Effects** | Input telemetry is sanitized. | Elimination of violent jolts during session transitions. |
| **Load/Grip Fallbacks** | NaN in wheel data triggers fallbacks (0.0). | More robust behavior when telemetry glitches. |

**Design Rationale**: By sanitizing auxiliary data to 0.0, we leverage existing robust fallback systems (like `approximate_load`), ensuring continuity of feel even if some sensors glitch.

## Proposed Changes

### 1. Hardware Sanitization (DirectInputFFB.cpp)
- Update `DirectInputFFB::UpdateForce` to check `std::isfinite(normalizedForce)`.
- If not finite, force to 0.0.

**Design Rationale**: This is the "Last Line of Defense". It prevents the specific integer overflow/min-value bug that causes full-lock jolts.

### 2. Core Telemetry Sanitization (FFBEngine.cpp)
- In `calculate_force`, before any processing, check core chassis data: `mUnfilteredSteering`, `mLocalRot.y`, `mLocalAccel.x`, `mLocalAccel.z`, `mSteeringShaftTorque`, `genFFBTorque`.
- If any are non-finite, return 0.0 immediately.

**Design Rationale**: If core chassis data is NaN, the whole physics frame is garbage. Aborting protects all subsequent filters from infection.

### 3. Auxiliary Data Sanitization (FFBEngine.cpp)
- After copying telemetry to `m_working_info`, iterate through the 4 wheels and sanitize auxiliary channels (`mTireLoad`, `mGripFract`, etc.) by replacing non-finite values with 0.0.

**Design Rationale**: Allows the engine to continue running using fallbacks if only a subset of data (like encrypted wheel loads) is NaN.

### 4. Bidirectional Filter Reset (FFBEngine.cpp)
- Change `if (m_was_allowed && !allowed)` to `if (m_was_allowed != allowed)`.
- This ensures filters are reset when entering AND exiting the muted state.

**Design Rationale**: Ensures the engine starts from a clean slate when the user clicks "Drive", even if the garage/menu period was noisy.

### 5. Final Output Sanitization (FFBEngine.cpp)
- At the very end of `calculate_force`, check `std::isfinite(norm_force)` and set to 0.0 if invalid.

**Design Rationale**: Catch-all for any math errors (like division by zero) that might occur within the engine's internal logic.

### Version Increment Rule
- Version in `VERSION` will be incremented by +1 to the rightmost number (0.7.193 -> 0.7.194).

## Test Plan (TDD-Ready)
### Design Rationale
The tests must prove that NaNs are rejected at the gate, that they don't permanently infect the engine state, and that context switches correctly flush all internal filters.

### Test Cases
1.  **test_nan_rejection_core**:
    - Inject NaN into `mUnfilteredSteering`.
    - Assert `calculate_force` returns 0.0.
    - Inject valid data in next frame, assert return value is valid (non-zero/non-NaN).
2.  **test_nan_sanitization_aux**:
    - Inject NaN into `mTireLoad` for one wheel.
    - Assert `calculate_force` still returns a valid number.
    - Assert snapshot shows fallback warning (`warn_load`).
3.  **test_bidirectional_reset**:
    - Drive at high force (fill filters).
    - Set `allowed = false`, verify filters are reset (0.0).
    - Set `allowed = true`, verify filters are still clean (no ramp-up spike).
4.  **test_hardware_nan_protection**:
    - Call `UpdateForce(NaN)`.
    - Verify magnitude sent to driver is 0 (mock check).

**Test Count Specification**: Baseline + 4 new tests.

## Deliverables
- Modified `src/ffb/DirectInputFFB.cpp`
- Modified `src/ffb/FFBEngine.cpp`
- New test file `tests/test_issue_384_nan_infection.cpp`
- Updated `VERSION` and `CHANGELOG_DEV.md`
- Implementation Notes (updated with deviations and issues).

## Implementation Notes
- **Encountered issues**:
    - Initial build failed because `std::isfinite` required `<cmath>` in `DirectInputFFB.cpp`.
    - `test_nan_sanitization_aux` initially failed because I only injected NaN into one front wheel, but the load fallback logic for the front axle required *both* wheels to be missing/zero to trigger the `warn_load` flag.
- **Deviations from plan**:
    - Expanded auxiliary sanitization to include non-core chassis data like throttle, brake, and local velocities to ensure all inputs to upsamplers are clean.
    - Added `<cmath>` include to `DirectInputFFB.cpp`.
- **Suggestions for the future**:
    - Consider adding a "Health Dashboard" to the GUI that explicitly flags NaN events as they occur in real-time, helping users identify which cars or track sections are sending bad telemetry.
