# Implementation Plan: Up-sample FFB Part 2 (1000Hz Output)

## Context
Following the successful implementation of Part 1 (which up-sampled the 100Hz LMU telemetry to a continuous signal using Holt-Winters extrapolation), Part 2 focuses on the output stage. Currently, the `lmuFFB` physics loop and USB output run at a hardcoded 400Hz. High-end Direct Drive wheelbases (Simucube, Asetek, Moza, etc.) operate internally at 1000Hz or higher. 

By increasing the `lmuFFB` main loop to 1000Hz, we can feed these wheelbases a much higher resolution signal. This minimizes interpolation artifacts on the wheelbase side and raises the Nyquist frequency of our synthetic effects (Road Texture, ABS, Slide Rumble) from 200Hz to 500Hz, allowing for vastly richer and finer tactile details.

## Design Rationale
*   **Single-Threaded 1000Hz Loop:** Instead of creating a complex multi-threaded architecture (e.g., a 400Hz physics thread and a 1000Hz output thread communicating via lock-free queues), we will simply increase the frequency of the existing `FFBThread` to 1000Hz. Because Part 1 introduced a zero-latency Holt-Winters extrapolator, querying this extrapolator at 1000Hz will naturally yield a perfectly smooth 1000Hz signal from the underlying 100Hz/400Hz game data.
*   **Configurable Output Rate:** Not all wheelbases can handle a 1000Hz USB polling rate. Older belt/gear-driven wheels (e.g., Logitech G29, Thrustmaster T300) may drop packets, overheat their USB controllers, or crash if fed data too quickly. Therefore, the output rate must be a user-configurable global setting (e.g., 400Hz, 500Hz, 1000Hz), defaulting to 400Hz for maximum compatibility.
*   **Dynamic `dt` Integration:** All physics filters (Biquad, LPF) and oscillators (ABS, Spin, Slide) in `FFBEngine` already use `ctx.dt`. By passing `dt = 0.001` (at 1000Hz), the math will automatically scale, requiring no changes to the core effect algorithms.

## Reference Documents
*   [Upsampling Research Report](https://github.com/coasting-nc/LMUFFB/blob/main/docs/dev_docs/reports/upsampling.md)
*   Issue #216: FFB Up-sampling

## Codebase Analysis Summary
*   **`src/main.cpp` (`FFBThread`)**: Currently uses a hardcoded `const std::chrono::microseconds target_period(2500);` to achieve 400Hz. This needs to be dynamic.
*   **`src/HealthMonitor.h`**: Hardcodes the expected loop rate to 400Hz (`loop < 360.0` triggers a warning). This must be updated to respect the configured output rate.
*   **`src/Config.h` / `src/Config.cpp`**: Needs a new global setting for the output rate.
*   **`src/GuiLayer_Common.cpp`**: Needs UI controls to allow the user to select their desired output rate.
*   **Design Rationale**: Modifying the core loop timing is the most direct and robust way to achieve 1000Hz output, provided the telemetry upsampler (Part 1) is already in place to feed it smooth data.

## FFB Effect Impact Analysis

| Effect | Technical Change | User Perspective |
| :--- | :--- | :--- |
| **High-Frequency Textures (ABS, Slide, Road, Spin)** | The Nyquist limit increases from 200Hz to 500Hz. Oscillators will step at `dt = 0.001s`. | Textures will feel significantly sharper and more detailed on high-end wheelbases. ABS pulses can be felt with distinct mechanical clarity rather than a blur. |
| **Base FFB (Understeer, SoP)** | The signal sent to the wheelbase will have 2.5x more data points per second. | Smoother gradients during rapid weight transfer or snap oversteer. Less "grain" in the wheel. |
| **Filters (Notch, LPF)** | `dt` changes from `0.0025` to `0.001`. | No perceived change; the math automatically compensates to maintain the same time constants. |

## Proposed Changes

### 1. Update `src/Config.h` and `src/Config.cpp`
*   Add a global static variable: `static int m_output_rate_hz;`
*   Set the default value to `400` in `Config.cpp`.
*   **Parameter Synchronization Checklist**:
    *   [x] Declaration in `Config.h` (Global static, NOT in `Preset`).
    *   [x] Initialization in `Config.cpp`.
    *   [x] Entry in `Config::Save()` (under `[System & Window]`).
    *   [x] Entry in `Config::Load()`.
    *   [x] Validation logic in `Config::Load()`: `m_output_rate_hz = std::clamp(m_output_rate_hz, 100, 1000);`

### 2. Update `src/main.cpp` (`FFBThread`)
*   Remove the hardcoded `target_period`.
*   Inside the `while (g_running)` loop, dynamically calculate the target period based on the config:
    ```cpp
    int current_rate = std::clamp(Config::m_output_rate_hz, 100, 1000);
    std::chrono::microseconds target_period(1000000 / current_rate);
    next_tick += target_period;
    ```
*   Ensure the `actual_dt` passed to the Part 1 extrapolator and `calculate_force` accurately reflects the time since the last loop iteration (or use the ideal `1.0 / current_rate` if actual `dt` is too noisy, though actual `dt` is preferred for real-time sync).

### 3. Update `src/HealthMonitor.h`
*   Modify `HealthMonitor::Check` to accept the expected loop rate as a parameter.
    ```cpp
    static HealthStatus Check(double loop, double telem, double torque, int torqueSource, double expected_loop_rate)
    ```
*   Update the warning threshold:
    ```cpp
    if (loop > 1.0 && loop < (expected_loop_rate * 0.90)) { // Warn if dropping >10% frames
        status.loop_low = true;
        status.is_healthy = false;
    }
    ```
*   Update the call site in `main.cpp` to pass `Config::m_output_rate_hz`.

### 4. Update `src/GuiLayer_Common.cpp`
*   In the "System Health (Hz)" header, update the target display for the FFB Loop to show the configured rate instead of the hardcoded `400.0`.
    ```cpp
    DisplayRate("FFB Loop", engine.m_ffb_rate, (double)Config::m_output_rate_hz);
    ```
*   In the "General FFB" or "Advanced Settings" section, add a Combo box for "FFB Output Rate":
    ```cpp
    const char* rates[] = { "400 Hz (Default/Safe)", "500 Hz", "1000 Hz (High-End DD Only)" };
    int rate_idx = (Config::m_output_rate_hz == 1000) ? 2 : (Config::m_output_rate_hz == 500) ? 1 : 0;
    if (ImGui::Combo("FFB Output Rate", &rate_idx, rates, 3)) {
        Config::m_output_rate_hz = (rate_idx == 2) ? 1000 : (rate_idx == 1) ? 500 : 400;
        Config::Save(engine);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets the USB polling rate to the wheelbase.\n1000Hz provides maximum fidelity but may crash older wheels.");
    ```

### 5. Version Increment
*   Increment `VERSION` file and `src/Version.h` to the next patch version (e.g., `0.7.109`).

## Test Plan (TDD-Ready)

### 1. `test_ffb_output_rate.cpp`
*   **Design Rationale**: Ensures that the dynamic loop timing and health monitoring correctly adapt to the user's configured output rate.
*   **`test_health_monitor_dynamic_rate`**:
    *   Call `HealthMonitor::Check` with `loop = 950.0`, `expected_loop_rate = 1000.0`. Assert `is_healthy == true`.
    *   Call `HealthMonitor::Check` with `loop = 800.0`, `expected_loop_rate = 1000.0`. Assert `is_healthy == false` and `loop_low == true`.
    *   Call `HealthMonitor::Check` with `loop = 380.0`, `expected_loop_rate = 400.0`. Assert `is_healthy == true`.
*   **`test_oscillator_frequency_scaling`**:
    *   Initialize `FFBEngine` with ABS pulse enabled (e.g., 50Hz pulse).
    *   Simulate 1 second of execution at `dt = 0.0025` (400Hz). Count the number of zero-crossings in `ctx.abs_pulse_force`. Assert it matches ~50Hz.
    *   Simulate 1 second of execution at `dt = 0.001` (1000Hz). Count the number of zero-crossings. Assert it still matches ~50Hz, proving the oscillators are frame-rate independent.

## Deliverables
- [ ] Update `src/Config.h` and `src/Config.cpp` with `m_output_rate_hz`.
- [ ] Update `src/main.cpp` to use dynamic `target_period`.
- [ ] Update `src/HealthMonitor.h` to use dynamic expected loop rate.
- [ ] Update `src/GuiLayer_Common.cpp` with UI controls and dynamic health targets.
- [ ] Create `tests/test_ffb_output_rate.cpp`.
- [ ] Update `tests/CMakeLists.txt` to include the new test file.
- [ ] Increment version in `VERSION` and `src/Version.h`.
- [ ] Update this plan document with "Implementation Notes" (Unforeseen Issues, Plan Deviations, Challenges, Recommendations) upon completion.