# Refactoring Report: FFBEngine `calculate_force` Modularization

## Purpose
The primary goal of this refactoring was to improve the maintainability, readability, and testability of the core FFB calculation logic. The `FFBEngine::calculate_force` method had grown into a monolithic function of over 600 lines, making it difficult to understand, debug, and extend. By splitting the logic into discrete helper methods and sharing state via a context structure, the code becomes more modular and easier to manage.

## Changes Implemented

### 1. Introduction of `FFBCalculationContext`
A new struct, `FFBCalculationContext`, was introduced in `FFBEngine.h`. This struct serves as a data transfer object (DTO) that holds:
-   **Derived Telemetry Data:** Values calculated once per frame and reused across multiple effects (e.g., `car_speed`, `decoupling_scale`, `avg_load`, `speed_gate`).
-   **Intermediate Results:** Forces calculated by one stage and used by another (e.g., `sop_base_force`, `grip_factor`).
-   **Effect Outputs:** The final force contribution from each specific effect (e.g., `road_noise`, `abs_pulse_force`).
-   **Diagnostic Flags:** Warning states for missing telemetry.

All members of this struct are initialized to safe default values to prevent undefined behavior.

### 2. Extraction of Helper Methods
The logic for each distinct FFB effect was extracted from the main loop into dedicated private helper methods within `FFBEngine`. These methods take the raw telemetry pointer (`TelemInfoV01*`) and a reference to the context (`FFBCalculationContext&`):

-   `calculate_sop_lateral`: Handles Seat of Pants (SoP) lateral G-force, rear grip loss calculation, Oversteer Boost, Rear Aligning Torque, and Yaw Kick.
-   `calculate_gyro_damping`: Computes gyroscopic damping based on steering velocity.
-   `calculate_abs_pulse`: Detects ABS activation and generates the corresponding vibration pulse.
-   `calculate_lockup_vibration`: Handles the complex logic for predictive wheel lockup, including axle differentiation and gamma response.
-   `calculate_wheel_spin`: Computes traction loss vibration and torque drop.
-   `calculate_slide_texture`: Generates the "scrubbing" vibration during lateral slides.
-   `calculate_road_texture`: Handles vertical deflection (bumps) and Scrub Drag logic.
-   `calculate_suspension_bottoming`: detects suspension bottoming events.

### 3. Logic Preservation and Fixes
The refactoring aimed for exact mathematical equivalence with the original code, but several regressions identified during review were fixed:
-   **ABS Pulse Restoration:** The ABS pulse force is now explicitly stored in `ctx.abs_pulse_force` and added to the final sum (previously it was calculated but ignored).
-   **Torque Drop Restoration:** The traction loss "torque drop" logic (gain reduction) is now stored in `ctx.gain_reduction_factor` and applied to the total force at the end of the pipeline.
-   **Snapshot Consistency:** The `FFBSnapshot` logic was updated to use `ctx.sop_unboosted_force` for `snap.sop_force` and derive the boost amount dynamically. This preserves the semantic meaning of the debug graph channels (separating base lateral force from the added boost).

### 4. Code Cleanup
-   **Standardization:** Replaced Windows-specific `strcpy_s` with cross-platform string copy (using `#ifdef _MSC_VER` to select `strncpy_s` on Windows, `strncpy` elsewhere).
-   **Thread Safety:** Maintained `std::lock_guard` usage for thread-safe access to the debug buffer.

### 5. Additional Improvements (v0.6.36 Code Review Follow-up)
-   **`calculate_wheel_slip_ratio` Helper:** Extracted duplicated lambda (`get_slip`) from `calculate_lockup_vibration` and `calculate_wheel_spin` into a unified public helper method. This reduces code duplication and improves testability.
-   **`apply_signal_conditioning` Method:** Extracted ~70 lines of signal conditioning logic (idle smoothing, frequency estimation, dynamic/static notch filters) from `calculate_force` into a dedicated private helper. This improves readability and makes the main method a cleaner high-level pipeline.
-   **Unconditional State Update Fix:** Moved `m_prev_vert_accel` update from inside `calculate_road_texture` (conditional) to the unconditional state updates section at the end of `calculate_force`. This prevents stale data issues if road texture is disabled but other effects depend on vertical acceleration history.
-   **Build Warning Fixes:** Fixed MSVC warnings C4996 (strncpy unsafe) and C4305 (double-to-float truncation) in test files.

## Justification
This modular architecture allows developers to:
-   **Isolate Effects:** Work on a single effect (e.g., "Improve ABS feel") without navigating hundreds of lines of unrelated code.
-   **Test Independently:** Future unit tests can target specific helper methods by setting up a context state, rather than running the full pipeline.
-   **Reduce Cognitive Load:** The main `calculate_force` method now reads as a high-level pipeline (Signal Conditioning -> Pre-calculations -> Effect Calls -> Summation), making the data flow clear.
