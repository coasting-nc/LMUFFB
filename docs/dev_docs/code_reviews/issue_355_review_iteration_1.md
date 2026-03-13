# Code Review - Issue #355: Fix Suspension Force Interpretation

## Evaluation of the Solution

### Core Functionality
- **Physics Normalization:** The patch successfully implements Motion Ratio (MR) normalization for suspension force derivatives. By multiplying the pushrod impulse by the MR, it correctly derives the wheel impulse, which prevents "false-positive" bottoming triggers on prototypes (which have a low MR of ~0.5).
- **Encryption Support:** The patch correctly implements a fallback to `approximate_load()` when `mTireLoad` is unavailable (telemetry encryption), using the `ctx.frame_warn_load` flag. This ensures safety triggers for bottoming and lockup grounding checks remain functional for all cars.
- **Architectural Refactoring:** The agent centralized the car-class-specific constants (Motion Ratio and Unsprung Weight) into `VehicleUtils`, removing magic numbers from the physics engine and improving maintainability.
- **Documentation:** The requested documentation comments were added verbatim, providing critical context for future contributors regarding the pushrod/wheel load distinction and the `mGripFract` interpretation.

### Safety & Side Effects
- **Clamping:** The refactored `approximate_load` functions include `std::max(0.0, ...)` to ensure physical sanity (load cannot be negative).
- **State Integrity:** The patch adds an explicit update for `m_prev_susp_force` at the end of the `calculate_force` loop, ensuring that Method 1 impulse calculations are always using the correct delta from the previous frame.
- **Regressions:** The unit tests are comprehensive and verify that the new normalization logic behaves as expected for different car classes.

### Completeness
- All call-sites identified in the issue were updated.
- The patch includes a verbatim copy of the GitHub issue and a detailed implementation plan.
- The project builds and the new tests cover the logic changes thoroughly.
- VERSION and CHANGELOG_DEV.md have been updated.

## Final Rating: #Greenlight#
