

# Implementation Plan: Up-sample FFB Part 2 (400Hz to 1000Hz)

## Context
The goal of this task is to up-sample the final Force Feedback output from the internal 400Hz physics loop to a 1000Hz output loop. This is Part 2 of the upsampling initiative, focusing strictly on **Output Conditioning** to match the native USB polling rates of modern Direct Drive wheelbases.

## Design Rationale (Overall)
Modern Direct Drive wheelbases (Simucube, Fanatec, Moza) operate internally at 1000Hz or higher. Sending them a 400Hz signal forces the wheelbase to perform its own interpolation (often poorly) or results in a "stepped" signal that feels grainy or robotic. 

By decoupling the main thread (running at 1000Hz) from the physics engine (running at 400Hz) and up-sampling the final calculated force using a **5/2 Polyphase FIR Filter**, we mathematically reconstruct the continuous analog signal. This eliminates aliasing and digital stepping artifacts, providing a buttery-smooth, high-fidelity force feedback experience without increasing the CPU burden of the core physics engine.

## Reference Documents
*[Upsampling Research Report](docs/dev_docs/reports/upsampling.md)
*   Issue #216 (Part 2)
*   Implementation plan for Part 1: `docs\dev_docs\implementation_plans\plan_upsample_ffb_part1.md`
---

## Notes on updates to this plan after Part 1 was implemented

While the core mathematical concept of Part 2 (the 5/2 Polyphase FIR filter) remains the same, Part 1 introduced specific structural changes to `main.cpp`, `FFBEngine.h`, and the diagnostic monitors that Part 2 must now carefully navigate.

### Why Part 2 needs updating based on Part 1:

1. **Telemetry Polling Location:** In Part 1, `GameConnector::Get().CopyTelemetry()` is called every tick of the 400Hz loop. If Part 2 blindly increases the main loop to 1000Hz, it will poll the shared memory at 1000Hz. Since the physics engine will only run at 400Hz, polling at 1000Hz wastes CPU and mutex locks. The plan must explicitly state to move the telemetry polling *inside* the 400Hz phase-accumulator block.
2. **The `dt` Parameter:** Part 1 hardcoded the physics `dt` to `0.0025` (400Hz) inside `calculate_force` to ensure DSP stability. Part 2 must ensure that the main loop's new 1000Hz delta-time (`0.001s`) is **not** accidentally passed into the physics engine, which would break all the filters Part 1 just fixed.
3. **Diagnostic Rates (`RateMonitor`):** Part 1 heavily integrated `m_ffb_rate`, `m_telemetry_rate`, etc., into the GUI and Logger. By decoupling the USB output (1000Hz) from the Physics (400Hz), Part 2 needs to add a new `m_physics_rate` variable to `FFBEngine.h` and `FFBSnapshot` so the user can verify both loops are running at their correct, independent speeds.

## Codebase Analysis Summary

### Current Architecture Overview
*   **`src/main.cpp`**: The `FFBThread` runs at a fixed 400Hz (`target_period(2500)` microseconds). It polls telemetry via `GameConnector`, runs `calculate_force`, and immediately sends the result to `DirectInputFFB`.
*   **`src/FFBEngine.h` / `src/GuiLayer.cpp`**: Tracks system health via `m_ffb_rate`, `m_telemetry_rate`, etc.
*   **`src/DirectInputFFB.cpp`**: Contains an optimization (`if (magnitude == m_last_force) return false;`) which prevents unnecessary USB spam if the force hasn't changed.

### Impacted Functionalities
1.  **`FFBThread` (Main Loop)**: Must be promoted to run at 1000Hz (1000 microsecond period). It needs a phase accumulator to trigger the 400Hz physics rate exactly 2 times for every 5 loop iterations.
2.  **`HealthMonitor` & `RateMonitor`**: Must be updated to track the new 1000Hz loop rate alongside the 400Hz physics rate.
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
    RateMonitor physicsMonitor; // New monitor for the 400Hz loop

    while (g_running) {
        // ... timing logic ...

        phase_accumulator += 2; // M = 2
        bool run_physics = false;

        if (phase_accumulator >= 5) { // L = 5
            phase_accumulator -= 5;
            run_physics = true;
        }

        if (run_physics) {
            // CRITICAL: Move Telemetry Polling INSIDE this block
            if (g_ffb_active && GameConnector::Get().IsConnected()) {
                bool in_realtime = GameConnector::Get().CopyTelemetry(g_localData);
                // ... existing telemetry extraction ...
                
                // Call calculate_force (it will default to dt=0.0025 internally)
                current_physics_force = g_engine.calculate_force(...);
                physicsMonitor.RecordEvent();
            }
        }

        // Always process the resampler at 1000Hz
        double output_force = resampler.Process(current_physics_force, run_physics);

        // Send to wheel
        DirectInputFFB::Get().UpdateForce(output_force);
        loopMonitor.RecordEvent();
        
        std::this_thread::sleep_until(next_tick);
    }
    ```

### 3. Update Diagnostics (`FFBEngine.h`, `HealthMonitor.h`, `GuiLayer.cpp`)
*   **`FFBEngine.h` & `FFBSnapshot`**: Add `double m_physics_rate = 0.0;` and `float physics_rate;` to track the 400Hz loop separately from the 1000Hz `m_ffb_rate`.
*   **`HealthMonitor.h`**: Update `HealthStatus` to expect a `loop_rate` of 1000Hz (warn below 950Hz) and add a `physics_rate` check (expect 400Hz, warn below 380Hz).
*   **`GuiLayer_Common.cpp`**: Update the "System Health" UI panel to display both "USB Loop (1000Hz)" and "Physics (400Hz)".

### Parameter Synchronization Checklist
*No new user-facing settings are required. This is a core pipeline upgrade.*

### Initialization Order Analysis
The `PolyphaseResampler` has no external dependencies and can be instantiated directly inside the `FFBThread` function in `main.cpp` before the `while(g_running)` loop begins.

### Version Increment Rule
Increment the version number in `VERSION` and `src/Version.h` by the **smallest possible increment** (e.g., `0.7.116` -> `0.7.117`).

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
- [ ] Modify `src/main.cpp` to implement the 1000Hz loop, phase accumulator, and nested telemetry polling.
- [ ] Update `src/FFBEngine.h`, `src/HealthMonitor.h`, and `src/GuiLayer_Common.cpp` to track and display the new `physics_rate`.
- [ ] Create `tests/test_upsampler.cpp` with the specified TDD tests.
- [ ] Increment version in `VERSION` and `src/Version.h`.
- [ ] Update this plan document with "Implementation Notes" (Unforeseen Issues, Plan Deviations, Challenges, Recommendations) upon completion.