# Implementation Plan: Up-sample FFB Part 2 (1000Hz Output)


Here is the implementation plan for Part 2 of the FFB upsampling feature.

### Design Rationale (Overall)
While Part 1 focused on fixing the mathematical derivatives of the 100Hz input telemetry for the physics engine, Part 2 focuses entirely on the **hardware output stage**. Modern Direct Drive wheelbases (Simucube, Fanatec, Moza) operate internally at 1000Hz or higher. Sending them a 400Hz signal forces the wheelbase to perform its own interpolation (often poorly) or results in a "stepped" signal that feels grainy or robotic. 

By up-sampling the final calculated force from 400Hz to 1000Hz using a **5/2 Polyphase FIR Filter**, we mathematically reconstruct the continuous analog signal. This eliminates aliasing and digital stepping artifacts, providing a buttery-smooth, high-fidelity force feedback experience that matches the native resolution of high-end hardware.

## Reference Documents
*   [Upsampling Research Report](docs/dev_docs/reports/upsampling.md)
*   docs\dev_docs\implementation_plans\plan_upsample_ffb_part1.md
---

## Codebase Analysis Summary

### Current Architecture Overview
*   **`src/main.cpp`**: The `FFBThread` runs at a fixed 400Hz (`target_period(2500)` microseconds). It polls telemetry, runs `calculate_force`, and immediately sends the result to `DirectInputFFB`.
*   **`src/HealthMonitor.h`**: Monitors the loop rate, expecting 400Hz.
*   **`src/DirectInputFFB.cpp`**: Contains an optimization (`if (magnitude == m_last_force) return false;`) which is already perfectly suited for higher polling rates, as it prevents unnecessary USB spam.

### Impacted Functionalities
1.  **`FFBThread` (Main Loop)**: Must be promoted to run at 1000Hz (1ms period). It needs a phase accumulator to decouple the 1000Hz output rate from the 400Hz physics rate.
2.  **`HealthMonitor` & `RateMonitor`**: Must be updated to track and validate the new 1000Hz loop rate alongside the 400Hz physics rate.
3.  **New DSP Module**: A new `PolyphaseResampler` class is required to handle the 5/2 rational resampling.

### Design Rationale
Instead of running the entire physics engine at 1000Hz (which would waste CPU and amplify noise since the source data is only 100Hz), we decouple the loop. The main thread ticks at 1000Hz, pushing interpolated samples to the wheel. Every 2.5 ticks (on average), it triggers the 400Hz physics engine to calculate a new base sample. This "Stream Processing" approach ensures zero buffer bloat and absolute minimum latency (< 2ms).

---

## FFB Effect Impact Analysis

| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| **All FFB Output** | The final `norm_force` is passed through a 15-tap Polyphase FIR filter before being sent to DirectInput. | **Silky smooth FFB.** The wheel will feel significantly more organic and less "digital". High-frequency effects (like ABS and Road Texture) will feel like pure tones rather than jagged buzzes. |
| **Latency** | The FIR filter introduces a deterministic group delay of exactly 1.5ms (at 1000Hz). | Imperceptible to the user, but the trade-off yields a massive increase in signal quality. |

---

## Proposed Changes

### 1. Create `src/UpSampler.h` and `src/UpSampler.cpp`
Implement a `PolyphaseResampler` class specifically designed for a 5/2 ratio (400Hz -> 1000Hz).

*   **Design Rationale:** A polyphase filter is vastly superior to linear interpolation. It acts as a low-pass filter (cutoff at 200Hz) to remove the "images" (aliasing) created by upsampling, perfectly reconstructing the original curve.
*   **Implementation Details:**
    *   Ratio: L=5 (Upsample), M=2 (Downsample).
    *   Phase Accumulator: `m_phase` (0 to 4).
    *   History Buffer: `std::array<double, 3> m_history` (stores the last 3 outputs from the 400Hz physics engine).
    *   Filter Kernel: A 15-tap windowed-sinc FIR filter (cutoff 0.2 * Nyquist), multiplied by 5 (DC gain), split into 5 branches of 3 taps each.
    *   *Method:* `double Process(double latest_physics_sample, bool is_new_physics_tick)`

### 2. Modify `src/main.cpp` (`FFBThread`)
Decouple the 1000Hz USB output loop from the 400Hz physics loop.

*   **Timing Update:** Change `target_period(2500)` to `target_period(1000)`.
*   **Phase Logic:**
    ```cpp
    PolyphaseResampler resampler;
    int phase_accumulator = 0;
    double current_physics_force = 0.0;

    while (g_running) {
        // ... timing logic ...

        phase_accumulator += 2; // M = 2
        bool run_physics = false;

        if (phase_accumulator >= 5) { // L = 5
            phase_accumulator -= 5;
            run_physics = true;
        }

        if (run_physics) {
            // 1. Poll Telemetry
            // 2. Call g_engine.calculate_force()
            // 3. Store result in current_physics_force
            physicsMonitor.RecordEvent();
        }

        // 4. Always process the resampler at 1000Hz
        double output_force = resampler.Process(current_physics_force, run_physics);

        // 5. Send to wheel
        DirectInputFFB::Get().UpdateForce(output_force);
        loopMonitor.RecordEvent();
        
        std::this_thread::sleep_until(next_tick);
    }
    ```

### 3. Update `src/HealthMonitor.h` and `main.cpp` Diagnostics
*   Update `HealthStatus` to expect a `loop_rate` of 1000Hz (warn below 950Hz).
*   Add a new parameter to `HealthMonitor::Check` for `physics_rate` (expect 400Hz, warn below 380Hz).
*   Update the console logging in `main.cpp` to print both the 1000Hz Loop rate and the 400Hz Physics rate.

### Parameter Synchronization Checklist
*No new user-facing settings are required. This is a core pipeline upgrade.*

### Initialization Order Analysis
The `PolyphaseResampler` has no external dependencies and can be instantiated directly inside the `FFBThread` function in `main.cpp` before the `while(g_running)` loop begins.

### Version Increment Rule
Increment the version number in `VERSION` and `src/Version.h` by the **smallest possible increment** (e.g., `0.7.108` -> `0.7.109`).

---

## Test Plan (TDD-Ready)

### Design Rationale
The tests must prove that the polyphase resampler correctly outputs 5 samples for every 2 input samples, maintains a DC gain of 1.0 (so FFB strength doesn't change), and that the main loop phase accumulator correctly triggers the physics engine at exactly 40% of the loop rate.

### 1. `test_upsampler_dc_gain`
*   **Description:** Verifies the FIR filter coefficients are correctly normalized.
*   **Inputs:** Feed a constant `1.0` into the resampler for 20 ticks (simulating a steady cornering force).
*   **Expected Output:** After the initial filter delay (3 ticks), the output should be exactly `1.0` across all 5 phases.
*   **Assertion:** `ASSERT_NEAR(output, 1.0, 0.001)`

### 2. `test_upsampler_polyphase_routing`
*   **Description:** Verifies the phase accumulator logic correctly routes inputs to outputs.
*   **Inputs:** Manually drive the `phase_accumulator` logic from `main.cpp` in a test loop for 10 iterations.
*   **Expected Output:** `run_physics` should be `true` on iterations: 0, 3, 5, 8, 10. (Exactly 2 physics ticks for every 5 loop ticks).

### 3. `test_health_monitor_1000hz`
*   **Description:** Verifies the updated `HealthMonitor` correctly identifies healthy vs. degraded states at the new rates.
*   **Inputs:** 
    *   Loop: 990Hz, Physics: 395Hz -> Expected: Healthy
    *   Loop: 800Hz, Physics: 395Hz -> Expected: Degraded (Loop Low)
    *   Loop: 990Hz, Physics: 300Hz -> Expected: Degraded (Physics Low)

**Test Count Specification:** Baseline + 3 new test cases.

---

## Deliverables
- [ ] Create `src/UpSampler.h` and `src/UpSampler.cpp` with the `PolyphaseResampler` class.
- [ ] Modify `src/main.cpp` to implement the 1000Hz loop and phase accumulator.
- [ ] Update `src/HealthMonitor.h` thresholds.
- [ ] Create `tests/test_upsampler.cpp` with the specified TDD tests.
- [ ] Increment version in `VERSION` and `src/Version.h`.
- [ ] Update this plan document with "Implementation Notes" (Unforeseen Issues, Plan Deviations, Challenges, Recommendations) upon completion.

