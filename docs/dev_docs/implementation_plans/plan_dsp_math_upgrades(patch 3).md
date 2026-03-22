# Implementation Plan - DSP Mathematical Upgrades & Time-Aware Telemetry Upsampling

## Context
Following the structural categorization of auxiliary telemetry channels in Patch 2 (Issue #466), this patch (Patch 3) implements critical mathematical corrections to the `HoltWintersFilter` and the channel tuning parameters. Based on deep research into vehicle dynamics DSP, the current implementation is vulnerable to telemetry jitter (variable 100Hz frame times) and catastrophic extrapolation overshoot during dropped frames. This patch upgrades the filter to be "Time-Aware", introduces "Trend Damping" to safely arrest runaway extrapolation, and corrects the Alpha/Beta tuning parameters to respect Nyquist-Shannon sampling limits.

## Design Rationale
**Why upgrade the math?**
1.  **Time-Awareness:** Game telemetry does not arrive at a perfect 10ms cadence due to OS scheduling and game engine rendering fluctuations. Dividing a change in signal by a hardcoded `0.01s` when the actual elapsed time was `0.015s` calculates an artificially steep, incorrect derivative. The filter must measure the *actual* time between frames.
2.  **Trend Damping:** If a telemetry frame drops while the driver is rapidly counter-steering, standard Holt-Winters will extrapolate that high velocity into infinity, causing a violent FFB spike. We must decay the trend to zero during starvation events so the signal safely plateaus.
3.  **Nyquist Limits:** Extrapolating a 30Hz tire vibration sampled at 100Hz is mathematically volatile. We must dynamically force the trend tracker (Beta) to `0.0` for Group 2 signals when "Zero Latency" is selected to prevent harmonic ringing and metallic grinding noises.

## Reference Documents
*   `docs\dev_docs\investigations\FFB Telemetry Upsampling Research.md` (Deep Research Report on DSP Architectures)
*   `docs\dev_docs\implementation_plans\plan_issue_466_aux_channel_differentiation.md` implementation plan that was completed for Patch 2
*   `docs\dev_docs\reports\HoltWintersFilter recommendations comparison.md`
*   Issue #461 & #466 (Previous Holt-Winters integration and channel grouping)


---

## Codebase Analysis Summary
*   **`src/utils/MathUtils.h` (`HoltWintersFilter`)**: The core DSP class. Currently uses a hardcoded `m_game_tick = 0.01` and maintains a static `m_trend` during extrapolation. Needs new state variables to accumulate actual `dt` between frames and a decay multiplier for the trend.
*   **`src/ffb/FFBEngine.cpp`**: The `FFBEngine` constructor and `ApplyAuxReconstructionMode()` method. Needs updated `Configure()` calls with the mathematically corrected Alpha/Beta values.

### Design Rationale
Modifying `HoltWintersFilter` at the utility level ensures that both the main Steering Shaft Torque and all auxiliary channels instantly benefit from jitter immunity and dropped-frame protection without requiring changes to the 400Hz hot path in `calculate_force`.

---

## FFB Effect Impact Analysis

| Affected FFB Effect | Technical Change | User-Facing Change (Feel) |
| :--- | :--- | :--- |
| **All FFB Effects** | `HoltWintersFilter` becomes Time-Aware. | **Jitter Immunity.** Eliminates micro-stutters and "graininess" caused by fluctuating game framerates or CPU spikes. |
| **Gyro Damping** | Group 1 Beta reduced from 0.40 to 0.10. Trend Damping added. | **Safe Responsiveness.** Maintains zero-latency feel but eliminates the risk of the wheel violently snapping if a frame drops during a rapid counter-steer. |
| **Road / Slide Texture** | Group 2 Beta dynamically forced to 0.0 when in Zero Latency mode. | **Acoustic Purity.** Eliminates high-frequency metallic "squealing" or harmonic ringing caused by extrapolating under-sampled vibrations. |

### Design Rationale
These changes shift the FFB from being "mathematically naive" to "physically robust." By respecting sampling limits and handling network/OS imperfections gracefully, the wheelbase will deliver high fidelity without risking hardware damage or driver injury.

---

## Proposed Changes

### 1. Upgrade `HoltWintersFilter` (`src/utils/MathUtils.h`)
*   **Add State Variables:**
    *   `double m_accumulated_time = 0.01;` (Tracks actual time between 100Hz frames).
    *   `double m_trend_damping = 0.95;` (Multiplier applied to trend during extrapolation).
*   **Update `Process()` Logic:**
    *   *When `is_new_frame == false`:* 
        *   Add `dt` to `m_accumulated_time`.
        *   Apply trend damping: `m_trend *= m_trend_damping;` (Decays the slope toward zero to safely plateau the signal).
    *   *When `is_new_frame == true`:*
        *   Use `m_accumulated_time` (instead of `m_game_tick`) as the denominator when calculating the new `m_trend`.
        *   Reset `m_accumulated_time = 0.0;` after the calculation.

### 2. Correct Tuning Parameters (`src/ffb/FFBEngine.cpp`)
*   **Update Constructor Initialization:**
    *   *Group 1 (Driver Inputs):* Change from `Configure(0.95, 0.40)` to `Configure(0.95, 0.10)`.
    *   *Group 2 (Texture/Tires):* Change from `Configure(0.80, 0.20)` to `Configure(0.80, 0.05)`.
    *   *Group 3 (Impacts):* Verify it remains `Configure(0.50, 0.00)`.
*   **Update `ApplyAuxReconstructionMode()`:**
    *   Add logic to dynamically override Beta for Group 2 based on the user's UI toggle.
    ```cpp
    // Inside ApplyAuxReconstructionMode()
    bool user_wants_raw = (m_advanced.aux_telemetry_reconstruction == 0);
    
    // Group 2: Dynamic Beta Forcing
    double group2_beta = user_wants_raw ? 0.00 : 0.05; // Force Beta=0.0 to prevent Nyquist ringing in Zero Latency mode
    for(int i=0; i<4; i++) {
        m_upsample_vert_deflection[i].Configure(0.80, group2_beta);
        m_upsample_vert_deflection[i].SetZeroLatency(user_wants_raw);
        // ... apply to rest of Group 2
    }
    ```

### 3. Parameter Synchronization Checklist
*   *Note: No new user-facing settings are added in this patch. The existing `aux_telemetry_reconstruction` toggle is reused.*

### 4. Initialization Order Analysis
*   `HoltWintersFilter::Configure()` can be called safely at any time. Calling it inside `ApplyAuxReconstructionMode()` will not cause circular dependencies or reset the filter's internal `m_level` state, ensuring seamless transitions if the user changes the setting while driving.

### 5. Version Increment Rule
*   Increment `VERSION` in `src/Version.h` and `CMakeLists.txt` by the **smallest possible increment** (+1 to the rightmost number, e.g., `0.7.222`).
*   Add an entry to `CHANGELOG_DEV.md`.

---

## Test Plan (TDD-Ready)

### Design Rationale
The mathematical upgrades to `HoltWintersFilter` require precise unit testing to ensure time-awareness correctly calculates slopes and that trend damping safely arrests runaway signals.

**Test 1: `test_holt_winters_time_awareness`**
*   **Description:** Feeds the filter a constant slope but varies the time between `is_new_frame == true` events (e.g., 8ms, then 12ms).
*   **Expected Output:** The calculated `m_trend` should remain constant because the filter correctly divides the delta by the actual accumulated time, not a hardcoded 10ms.
*   **Assertion:** Will fail until `m_accumulated_time` logic is implemented.

**Test 2: `test_holt_winters_trend_damping`**
*   **Description:** Feeds the filter a sharp step change to establish a high `m_trend`, then simulates a dropped frame by calling `Process(dt, false)` 10 times in a row.
*   **Expected Output:** The extrapolated output should initially rise but gracefully curve into a flat plateau as `m_trend` decays to zero.
*   **Assertion:** Will fail (output will shoot to infinity) until `m_trend *= m_trend_damping` is implemented.

**Test 3: `test_group2_dynamic_beta_forcing`**
*   **Description:** Verifies that toggling `m_advanced.aux_telemetry_reconstruction` correctly reconfigures the Beta parameter for Group 2 channels.
*   **Expected Output:** When config is `0` (Zero Latency), Group 2 Beta is `0.0`. When config is `1` (Smooth), Group 2 Beta is `0.05`.

**Test Count Specification:** Baseline + 3 new unit tests.

---

## Deliverables
- [ ] **Code Changes:** `src/utils/MathUtils.h`, `src/ffb/FFBEngine.cpp`, `src/Version.h`.
- [ ] **Tests:** New test cases in `tests/test_math_utils.cpp` and `tests/test_reconstruction.cpp`.
- [ ] **Documentation:** Update `CHANGELOG_DEV.md`.
- [ ] **Implementation Notes:** Update this plan document with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations" upon completion.