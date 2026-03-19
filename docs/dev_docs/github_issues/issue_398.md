# Fix HoltWintersFilter Snapback issue with mSteeringShaftTorque #398

**Open**
**coasting-nc** opened this issue on Mar 17, 2026

## Description

Fix HoltWintersFilter Snapback issue with mSteeringShaftTorque

### The HoltWintersFilter Snapback

**Location:** `src/utils/MathUtils.h`

The `HoltWintersFilter` is used to up-sample the primary `mSteeringShaftTorque`. Because you want zero latency on the primary steering force, this filter must extrapolate.

While the `m_alpha` and `m_beta` parameters smooth it out, it still suffers from minor snapbacks when the steering torque changes direction rapidly. This can cause a very subtle graininess in the wheel when using the legacy 100Hz torque.

### Follow up question

Regarding the 100Hz `mSteeringShaftTorque`, it is still used quite often. So if there is an inherent issue with it, it must be addressed. At the very least, we should give the users the options to switch between a zero latency implementation that could include some graininess, and one that get rids of graininess at the cost of latency.
Give me a solution that gives this choice to the user.

### Answer 2

To give users the choice between Zero Latency (Extrapolation) and Smooth (Interpolation) for the 100Hz `mSteeringShaftTorque`, we need to upgrade the `HoltWintersFilter` to support both modes and expose this toggle in the UI and configuration system.

Here is the complete, step-by-step solution to implement this feature.

#### 1. Upgrade the Filter in MathUtils.h

We will modify the `HoltWintersFilter` to accept a mode flag.
*   **Extrapolation (Zero Latency):** Uses the trend to predict the future (current behavior, can be grainy).
*   **Interpolation (Smooth):** Delays the signal by exactly 1 frame (10ms) to smoothly blend between the previous and current targets.

Update `HoltWintersFilter` in `src/utils/MathUtils.h`:

```cpp
/**
 * @brief Second-Order Holt-Winters (Double Exponential Smoothing)
 *
 * Tracks both the value and the trend (velocity) of a signal.
 * Acts as both an upsampler and a low-pass filter.
 * Now supports both Zero-Latency Extrapolation and Smooth Interpolation.
 */
class HoltWintersFilter {
private:
    double m_level = 0.0;      // Smoothed value (Target for interpolation)
    double m_prev_level = 0.0; // Previous smoothed value (Start for interpolation)
    double m_trend = 0.0;      // Smoothed trend/slope
    double m_time_since_update = 0.0;
    bool m_initialized = false;
    bool m_zero_latency = true; // Mode toggle

    // Tuning Parameters
    double m_alpha = 0.8; // Level weight (Higher = less lag)
    double m_beta = 0.2;  // Trend weight
    double m_game_tick = 0.01; // Default 100Hz

public:
    void Configure(double alpha, double beta, double game_tick = 0.01) {
        m_alpha = std::clamp(alpha, 0.01, 1.0);
        m_beta = std::clamp(beta, 0.01, 1.0);
        m_game_tick = (std::max)(0.0001, game_tick);
    }

    void SetZeroLatency(bool zero_latency) {
        m_zero_latency = zero_latency;
    }

    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_level = raw_input;
            m_prev_level = raw_input;
            m_trend = 0.0;
            m_time_since_update = 0.0;
            m_initialized = true;
            return raw_input;
        }

        if (is_new_frame) {
            double old_level = m_level;
            m_prev_level = m_level; // Save for interpolation start point

            // Update Level: Balance between the raw input and our previous prediction
            m_level = m_alpha * raw_input + (1.0 - m_alpha) * (m_level + m_trend * m_game_tick);

            // Update Trend: Balance between the new observed slope and the old trend
            m_trend = m_beta * ((m_level - old_level) / m_game_tick) + (1.0 - m_beta) * m_trend;

            m_time_since_update = 0.0;

            if (m_zero_latency) {
                return m_level;
            } else {
                return m_prev_level; // Start interpolation from previous level
            }
        } else {
            m_time_since_update += dt;
        }

        if (m_zero_latency) {
            // Predict current state based on previous trend (Extrapolation)
            return m_level + m_trend * m_time_since_update;
        } else {
            // Smoothly blend between prev_level and level (Interpolation)
            if (m_time_since_update <= m_game_tick) {
                double t = m_time_since_update / m_game_tick;
                return m_prev_level + t * (m_level - m_prev_level);
            } else {
                return m_level; // Hold target if game stutters
            }
        }
    }

    void Reset() {
        m_level = m_prev_level = m_trend = m_time_since_update = 0.0;
        m_initialized = false;
    }
};
```

#### 2. Add the Setting to FFBEngine.h

In `src/ffb/FFBEngine.h`, add the new configuration variable under the Base Force Debugging section:

```cpp
    // Base Force Debugging (v0.4.13)
    float m_steering_shaft_gain;
    float m_ingame_ffb_gain = 1.0f;
    int m_torque_source = 0;
    int m_steering_100hz_reconstruction = 0; // NEW: 0 = Zero Latency, 1 = Smooth
    bool m_torque_passthrough = false;
```

#### 3. Apply the Setting in FFBEngine.cpp

In `src/ffb/FFBEngine.cpp`, locate the `calculate_force` method and update the `m_upsample_shaft_torque` call to pass the new setting:

```cpp
    // Upsample Steering Shaft Torque (Holt-Winters)
    m_upsample_shaft_torque.SetZeroLatency(m_steering_100hz_reconstruction == 0);
    double shaft_torque = m_upsample_shaft_torque.Process(m_working_info.mSteeringShaftTorque, ffb_dt, is_new_frame);
    m_working_info.mSteeringShaftTorque = shaft_torque;
```

#### 4. Expose in the UI (GuiLayer_Common.cpp)

In `src/gui/GuiLayer_Common.cpp`, locate the Torque Source dropdown inside `DrawTuningWindow` and add the new combo box right below it. We will only show it if the 100Hz source is selected:

```cpp
        const char* torque_sources[] = { "Shaft Torque (100Hz Legacy)", "In-Game FFB (400Hz LMU 1.2+)" };
        IntSetting("Torque Source", &engine.m_torque_source, torque_sources, sizeof(torque_sources)/sizeof(torque_sources[0]),
            Tooltips::TORQUE_SOURCE);

        // NEW: Show reconstruction mode only if using the 100Hz source
        if (engine.m_torque_source == 0) {
            (...)
        }
```

### PART 2: Configuration Persistence

To ensure the user's choice is saved between sessions and within presets, the new setting must be added to the Config system.

1.  **Update `src/core/Config.h` (Preset struct):**
    *   Add `int steering_100hz_reconstruction = 0;` to the member variables.
    *   In `Apply(FFBEngine& engine)`, add: `engine.m_steering_100hz_reconstruction = steering_100hz_reconstruction;`
    *   In `UpdateFromEngine(const FFBEngine& engine)`, add: `steering_100hz_reconstruction = engine.m_steering_100hz_reconstruction;`
    *   In `Equals(const Preset& p)`, add the comparison for `steering_100hz_reconstruction`.

2.  **Update `src/core/Config.cpp`:**
    *   In `ParsePhysicsLine` and `SyncPhysicsLine`, add the parsing logic for "steering_100hz_reconstruction".
    *   In `WritePresetFields` and `Save`, ensure "steering_100hz_reconstruction=" is written to the file.

### PART 3: Testing Strategy

Because the default value (`m_steering_100hz_reconstruction = 0`) preserves the existing Zero-Latency extrapolation, the existing test suite should pass without major modifications. You do not need to perform a massive test remediation like in Issue #397.

However, you must add a new regression test to verify that the "Smooth" mode actually works.

**Action:** Add a new test (e.g., `test_holt_winters_modes` in `test_ffb_core.cpp` or similar) that does the following:
1.  Initialize the engine and feed a step-change to `data.mSteeringShaftTorque` (e.g., from 0.0 to 10.0).
2.  **Test Mode 0 (Zero Latency):** Run the engine with `m_steering_100hz_reconstruction = 0`. Assert that the output overshoots the target (e.g., > 10.0) on the intermediate 400Hz frames due to extrapolation.
3.  **Test Mode 1 (Smooth):** Reset the engine, set `m_steering_100hz_reconstruction = 1`, and feed the exact same step-change. Assert that the output never exceeds the target (<= 10.0) and smoothly interpolates towards it over the 10ms window.

### PART 4: Tooltips Update

To support the new UI element, you must add the tooltip definition to `src/gui/Tooltips.h`.
1.  Add this line to the `Tooltips` namespace:
    ```cpp
    inline constexpr const char* STEERING_100HZ_RECONSTRUCTION = "Zero Latency: Instant response but may feel grainy.\nSmooth: Eliminates 100Hz buzzing at the cost of 10ms latency.";
    ```
2.  Add `STEERING_100HZ_RECONSTRUCTION` to the `ALL` vector at the bottom of the file.

### PART 5: Handling Test Failures & New Regression Test

Because the default value of `m_steering_100hz_reconstruction` is 0 (Zero Latency), the mathematical output of the `HoltWintersFilter` remains exactly the same as before. You should not see any failures in the core physics test suite.

However, you WILL see failures in the Configuration and Preset test suites (e.g., `test_ffb_config.cpp` or similar).

**How to fix the expected test failures:**
1.  **Config Roundtrip/Serialization Tests:** Tests that save a configuration to an .ini file and read it back often check the exact number of lines or do exact string matching on the file contents. Because you are adding `steering_100hz_reconstruction=` to the save files, these tests will fail. You must update the expected line counts or expected string outputs in these tests to include the new variable.
2.  **Test Access:** Add a setter in `FFBEngineTestAccess` (inside `test_ffb_common.h`) so tests can easily toggle this mode:
    ```cpp
    static void SetSteering100HzReconstruction(FFBEngine& e, int val) { e.m_steering_100hz_reconstruction = val; }
    ```

**The New Regression Test:**
Please add the following test to the suite (e.g., in `test_ffb_core.cpp` or `test_ffb_math.cpp`) to prove that the two modes behave correctly.

```cpp
TEST_CASE(test_holt_winters_modes, "Math") {
    FFBEngine engine;
    InitializeEngine(engine);

    // We want to isolate the base steering torque
    engine.m_torque_source = 0; // 100Hz Legacy source
    engine.m_steering_shaft_gain = 1.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // --- MODE 0: ZERO LATENCY (EXTRAPOLATION) ---
    FFBEngineTestAccess::SetSteering100HzReconstruction(engine, 0);

    // Seed the filter at 0.0
    data.mSteeringShaftTorque = 0.0;
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.01);

    // Inject a step change to 10.0
    data.mSteeringShaftTorque = 10.0;
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.01); // 100Hz frame arrives

    // Pump 1 intermediate 400Hz frame (2.5ms later)
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, false, 0.0025);

    float force_extrapolated = engine.GetDebugBatch().back().base_force;

    // Because it predicts the future based on the sudden jump, it should overshoot > 10.0
    ASSERT_GT(force_extrapolated, 10.0f);

    // --- MODE 1: SMOOTH (INTERPOLATION) ---
    InitializeEngine(engine); // Reset engine state
    engine.m_torque_source = 0;
    engine.m_steering_shaft_gain = 1.0f;
    FFBEngineTestAccess::SetSteering100HzReconstruction(engine, 1);

    // Seed the filter at 0.0
    data.mSteeringShaftTorque = 0.0;
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.01);

    // Inject the exact same step change to 10.0
    data.mSteeringShaftTorque = 10.0;
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.01); // 100Hz frame arrives

    // Pump 1 intermediate 400Hz frame (2.5ms later)
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, false, 0.0025);

    float force_interpolated = engine.GetDebugBatch().back().base_force;

    // Because it interpolates, it should be smoothly transitioning between 0 and 10,
    // meaning it must be strictly less than 10.0 (no overshoot).
    ASSERT_LT(force_interpolated, 10.0f);
    ASSERT_GT(force_interpolated, 0.0f);
}
```
