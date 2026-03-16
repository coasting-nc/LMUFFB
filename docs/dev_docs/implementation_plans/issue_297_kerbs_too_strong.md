# Implementation Plan - Issue #297: Kerbs too strong

This plan implements fixes and features to reduce the impact of kerbs in self-aligning torque, as detailed in the investigation report: [docs/dev_docs/investigations/reduce kerbs impact in self-aligning torque.md](../../investigations/reduce%20kerbs%20impact%20in%20self-aligning%20torque.md).

## GitHub Issue
- **Number:** 297
- **Title:** Kerbs too strong

## Context
Multiple users have reported that kerb effects are too strong, specifically causing violent jolts in Direct Drive wheels. This is primarily due to the mathematical interaction between spiked tire load and spiked slip angle when hitting a kerb.

### Design Rationale
> **Why this matters:** Violent jolts can be physically jarring for Direct Drive wheel users and can obscure useful force feedback details. A fix that reduces these spikes without adding latency is critical for maintaining realism and drivability.

## Analysis
The SoP Self-Aligning Torque calculation currently uses:
`ctx.calc_rear_lat_force = rear_slip_angle * ctx.avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;`
On kerbs, both `rear_slip_angle` and `ctx.avg_rear_load` spike simultaneously, causing the product to explode.
The proposed solution involves:
1. **Physics Saturation:** Capping the dynamic load and soft-clipping the slip angle.
2. **Hybrid Kerb Rejection:** Using surface detection and suspension velocity to temporarily attenuate the torque.

### Design Rationale
> **Why 1.5x load cap?** Normal cornering rarely exceeds 1.5x static weight. Kerb strikes can reach 3x-4x. Capping at 1.5x removes the extreme spikes while preserving 100% of the cornering feel.
> **Why tanh for slip angle?** `std::tanh` mimics real pneumatic trail falloff (tires don't produce infinite torque as slip angle increases) and prevents mathematical explosions while remaining linear near the center.
> **Why Hybrid detection?** `mSurfaceType` works on all cars (including encrypted DLC). Suspension velocity (Method B) catches sausage kerbs or bumps not marked as rumblestrips on unencrypted cars.
> **Why 100ms timer?** Prevents the force from snapping back instantly while the suspension is still settling, providing a smoother transition.

## Proposed Changes

### 1. FFBEngine Settings & State
- Add `m_kerb_strike_rejection` (float) to `FFBEngine` for user tuning (0.0 to 1.0).
- Add `m_kerb_timer` (double) to track attenuation duration.
- Add constants for thresholds (`KERB_LOAD_CAP_MULT`, `KERB_DETECTION_THRESHOLD_M_S`, `KERB_HOLD_TIME_S`).

### 2. Core Math in `FFBEngine::calculate_sop_lateral`
- Implement the saturation and rejection logic described in the analysis.
- Ensure `m_kerb_timer` is updated every frame.
- Apply `kerb_attenuation` to `ctx.rear_torque`.

### 3. GUI & Tooltips
- Add `KERB_STRIKE_REJECTION` tooltip.
- Add "Kerb Strike Rejection" slider to the Tuning UI.

### 4. Configuration & Persistence
- Integrate the new setting into the `Preset` struct and `Config` class for persistence.

### 5. Versioning
- Increment version in `VERSION`.
- Add entry to `CHANGELOG_DEV.md`.

### Design Rationale
> **Why user configurable?** While the physics saturation is always on (safe math), the level of attenuation preferred for kerb strikes can vary between users and hardware.

## Test Plan
- **Unit Test:** `test_kerb_strike_rejection` in `tests/test_ffb_engine.cpp`.
    - Verify normal torque is preserved.
    - Verify surface-type-triggered attenuation.
    - Verify suspension-velocity-triggered attenuation.
    - Verify hold timer keeps attenuation active.
    - Verify 1.5x load cap limits output torque during extreme spikes even if rejection slider is 0.
- **Build & Regression:** Run all existing tests to ensure no regressions in other physics effects.

### Design Rationale
> **Why these tests?** They cover all new logic paths (always-on saturation and optional rejection) and ensure the hybrid triggers (surface vs velocity) work as intended.

## Implementation Notes

### Key Technical Decisions
1.  **Always-On Physics Saturation**:
    *   Capping effective load at 1.5x static weight. This is based on typical cornering dynamics; anything above is usually an impact.
    *   `tanh` soft-clipping: This mimics the real SAT curve which peaks and then falls off/plateaus. It naturally limits the maximum torque contribution from slip angle.
2.  **User-Adjustable Rejection**:
    *   The rejection slider (0.0 to 1.0) allows users to blend in additional attenuation based on specific triggers.
    *   Dual-trigger detection (Surface Type and Suspension Velocity) ensures coverage for all content types.

### Iterative Improvement during Build/Test
*   **Coordinate Inversion Issue**: During testing, I noticed that the new `tanh` logic slightly reduced the peak torque in existing regression tests. I updated `test_coordinate_rear_torque_inversion` to account for this saturation, while still verifying correct directional feedback.
*   **Code Review Fixes**:
    *   Added division-by-zero protection for `optimal_slip_ref` to ensure no FFB loss if the car's optimal slip angle isn't yet identified.
    *   Ensured `m_kerb_timer` is reset during simulation transitions.
    *   Confirmed `m_prev_vert_deflection` is correctly updated at the end of the frame (it was already handled in the post-calc state update section).

### Build Issues Encountered
*   Linux CI environment lacked `glfw3`, causing build failures in graphical mode. Switched to `-DBUILD_HEADLESS=ON` for the verification loop.

### Suggestions for the Future
1.  **Front Axle Saturation**: If users report similar harshness on the front axle (Understeer effect), consider implementing similar load capping and `tanh` soft-clipping for front slip angles.
2.  **User Feedback on `tanh` Curve**: Monitor user feedback regarding the change from a linear to a `tanh` torque curve. While physically more realistic, users used to the old "signature" feel might require a way to tune the "curvature" of the falloff.
3.  **Advanced Surface Detection**: Explore using other surface types (grass, gravel) for specialized haptic behaviors now that the `mSurfaceType` infrastructure is proven to work on all cars.

## Additional Questions
None.
