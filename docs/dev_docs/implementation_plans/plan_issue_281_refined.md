# Implementation Plan - Issue #281: Refined FFB Gating (Refining Fix)

## Context
Issue #281 reported FFB spikes when transitioning out of driving. An initial fix in v0.7.148 (Implementation Plan `plan_issue_281.md`) targetted zero force when `!IsPlayerActivelyDriving()`. However, this was too aggressive as it disabled the **Soft Lock** effect in the garage and during pause, where the player is still physically at the wheel.

This refined implementation (Iteration 2) targets zero force specifically when `mControl != 0` (non-player control), allowing Soft Lock to remain functional when stationary/paused while still preventing spikes when AI takes control.

## Design Rationale
- **Hardware Safety**: Soft Lock must remain active as long as the player is in control to prevent the wheel from hitting physical stops without resistance.
- **State Selection**: `mControl != 0` is the most direct indicator that the player is no longer responsible for the steering rack's safety.
- **Transition Safety**: Re-uses the existing slew rate limiter to handle the transition to zero force.

## Proposed Changes

### `src/main.cpp`
- Refine the force gating logic:
  ```cpp
  // From:
  if (!is_driving) force_physics = 0.0;
  // To:
  if (scoring.mControl != 0) force_physics = 0.0;
  ```

### Metadata & Documentation
- Increment version to `0.7.153` in `VERSION`.
- Update `CHANGELOG_DEV.md`.

## Test Plan
- **Updated Test**: `tests/test_issue_281_spikes.cpp`
- **Scenario 1: Garage/Pause (`mControl = 0`)**
  - Verify FFB output is Soft Lock (~1.0), NOT zero.
- **Scenario 2: AI Takeover (`mControl = 1`)**
  - Verify FFB output targets 0.0 and slews towards it.

## Deliverables
- [x] Modified `src/main.cpp`
- [x] Modified `tests/test_issue_281_spikes.cpp`
- [x] Updated `VERSION`
- [x] Updated `CHANGELOG_DEV.md`
- [x] Updated `tests/test_ffb_common.h` (added `SetLastOutputForce` helper)

## Implementation Notes
- **Refined Selection**: Using `mControl` instead of `IsPlayerActivelyDriving` ensures we only mute FFB when it is truly safe/necessary to do so, preserving useful safety features in non-driving player states.
- **CI Trigger**: Initial submission failed to trigger CI. Verified that non-MD files are included to bypass `paths-ignore`.
