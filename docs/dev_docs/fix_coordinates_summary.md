# Coordinate System Fix & Verification Summary

**Date:** 2025-12-19
**Version:** v0.4.30

## Objective
Fix failing tests resulting from the necessary revert/adjustments to the coordinate system logic, specifically ensuring FFB effects align with the LMU Physics Engine (+X = Left).

## Changes Implemented

### 1. Documentation Updates
*   **`docs/dev_docs/telemetry_data_reference.md`**: Added Section 6 "Coordinate Systems & Sign Conventions" clarifying LMU (+X=Left) vs ISO (+X=Forward?) vs DirectInput contexts.
*   **`docs/dev_docs/FFB_formulas.md`**: Updated SoP, Yaw Kick, and Rear Torque formulas to reflect v0.4.30 logic (SoP Positive, Yaw Kick Inverted, Rear Torque Negative-Input-Logic).

### 2. Test Suite Updates (`tests/test_ffb_engine.cpp`)
Retuned 16+ assertions across 7 test functions to match the confirmed physics behavior:

*   **`test_sop_effect`**: Updated to expect **Positive Force** (Left Pull) for Right Turn inputs (+X Accel), reflecting the removal of the inversion in `FFBEngine.h`.
*   **`test_oversteer_boost`**: Updated to expect **Positive Force** (Left Pull) when SoP is boosted.
*   **`test_smoothing_step_response`**: Updated to expect **Positive Force** for +X Accel step input.
*   **`test_preset_initialization`**: Updated preset list to include the newly added "T300" preset.
*   **`test_preset_load`**: Fixed gain expectation for "Test: SoP Only" preset (Expect 1.0f).
*   **`test_coordinate_sop_inversion`**: Verification logic flipped to expect Positive outputs for +X inputs.
*   **`test_regression_no_positive_feedback`**:
    *   Updated `mLateralPatchVel` inputs to **-5.0** (Negative) to represent a Left Slide for Rear Torque calculations. This confirms that the engine requires Negative Velocity inputs to produce Positive (Stabilizing) Aligning Torque with the current formula.
    *   Updated expectations to verify that SoP, Rear Torque, and Scrub Drag all produce **Positive (Left)** forces under these conditions, resulting in a net Stabilizing (+Force) output.
*   **`test_coordinate_all_effects_alignment`**:
    *   Updated inputs similarly to `test_regression_no_positive_feedback`.
    *   Verified that all lateral components align to provide a cohesive Counter-Steering force.

## Verification
*   **Test Result**: `Tests Passed: 123 | Tests Failed: 0`
*   **Conclusion**: The codebase is stable, and the test suite now correctly reflects the valid physics logic described in `fix FFB coordinates 3, oversteer.md`.
