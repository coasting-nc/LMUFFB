This code change implements **Context-Aware Yaw Kicks** to provide more nuanced and early feedback for oversteer situations, specifically focusing on "Unloaded" (braking/lift-off) and "Power" (traction loss) scenarios.

### Analysis and Reasoning:

1.  **User's Goal:** Implement hyper-sensitive, context-aware FFB yaw kicks that trigger specifically when the car is vulnerable to spinning, while avoiding noise during normal driving.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch successfully implements the requested physics logic. It introduces "Unloaded Yaw Kick" (gated by rear load drop) and "Power Yaw Kick" (gated by throttle and rear wheel slip). It also adds "Punch" (using yaw jerk to overcome wheelbase stiction) and "Gamma" curves for non-linear amplification of early slide onset.
    *   **Safety & Side Effects:**
        *   The patch maintains thread safety by using the existing `g_engine_mutex` when updating shared state in `update_static_load_reference`.
        *   It implements a robust blending strategy using the sign-preserving maximum absolute value (`max_abs`), ensuring that the most critical haptic warning takes priority without additive clipping.
        *   Input validation and clamping are consistently applied in the `Preset` and `Config` classes.
        *   Default gains are set to `0.0f` as requested, ensuring no immediate impact on existing user profiles until they opt-in.
    *   **Completeness:** The solution is exceptionally thorough. It covers:
        *   **Physics Logic:** Cross-frame derivative calculations for jerk and load-based gating.
        *   **State Persistence:** Expanding the car-specific load learning system to include the rear axle, including migration logic for existing car data.
        *   **Configuration:** Full synchronization across `Preset`, `Config` (loading/saving), and `FFBEngine`.
        *   **UI/UX:** Organized sliders in the tuning window and detailed tooltips explaining the physics behind the new settings.
        *   **Testing:** A dedicated test suite (`test_issue_322_yaw_kicks.cpp`) verifying activation thresholds, jerk-based "punch," blending logic, and load tracking.

3.  **Merge Assessment:**
    *   **Blocking:** None.
    *   **Nitpicks:** None. The implementation follows the project's established patterns for physics estimation and configuration management perfectly.

The patch is highly maintainable, safe, and fully functional. It addresses the issue comprehensively and includes necessary tests to prevent regressions.

### Final Rating: #Correct#