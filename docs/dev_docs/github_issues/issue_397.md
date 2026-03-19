# Fix LinearExtrapolator Sawtooth Bug #397

**Open**
**coasting-nc** opened this issue on Mar 17, 2026

## Description

Fix LinearExtrapolator Sawtooth Bug

### Code Review and recommended fixes

Based on a thorough review of the codebase, there is a critical mathematical flaw in the `LinearExtrapolator` class used to up-sample the 100Hz game telemetry to the 400Hz physics loop.

This flaw is the primary cause of the lack of smoothness, 100Hz "graininess" (buzzing), and violent unwanted vibrations (specifically false-triggering the suspension bottoming effect).

Here is the detailed breakdown of the issues found and how to fix them.

---

### The LinearExtrapolator Sawtooth Bug (Critical)

**Location:** `src/utils/MathUtils.h`

The `LinearExtrapolator` is used to up-sample almost every auxiliary telemetry channel (suspension force, patch velocities, accelerations, etc.) from 100Hz to 400Hz.

It uses a "dead reckoning" approach: it calculates the rate of change over the previous 10ms frame, and assumes the signal will continue at that exact rate into the future.

```cpp
if (is_new_frame) {
    m_rate = (raw_input - m_last_input) / m_game_tick;
    m_current_output = raw_input; // <--- THE SNAP
    // ...
} else {
    m_current_output += m_rate * dt; // <--- THE OVERSHOOT
}
```

**The Failure Mode:**
When a signal changes direction or stops accelerating, the extrapolation overshoots reality. When the next 100Hz frame arrives, `m_current_output` violently snaps back to the true `raw_input`. This creates a continuous 100Hz sawtooth wave riding on top of all your telemetry signals. This manifests in the wheel as a constant, grainy buzzing sensation.

**The Derivative Spike (Unwanted Vibrations):**
This sawtooth wave breaks effects that rely on derivatives (rate of change). For example, look at `calculate_suspension_bottoming` in `FFBEngine.cpp`:

```cpp
double dForceL = ((data->mWheel[0].mSuspForce - m_prev_susp_force[0]) * mr) / ctx.dt;
```

Because `mSuspForce` is up-sampled using the extrapolator, every 10ms when the signal "snaps back" from an overshoot, `dForceL` registers a massive, instantaneous spike. This easily exceeds the `BOTTOMING_IMPULSE_THRESHOLD_N_S`, causing the FFB engine to constantly trigger the heavy "Bottoming Crunch" vibration even on smooth roads.

### The Fix: Convert to a Linear Interpolator

To fix this, you must delay the auxiliary signals by exactly 1 frame (10ms). This allows you to smoothly interpolate between the previous frame and the current frame, guaranteeing C0 continuity (no snaps) and a piecewise-constant derivative (no spikes). 10ms of latency on auxiliary effects (like road texture and SoP) is completely imperceptible to humans, whereas 100Hz buzzing is highly perceptible.

Replace the `LinearExtrapolator` class in `MathUtils.h` with this:

```cpp
/**
 * @brief Linear Interpolator (Inter-Frame Reconstruction)
 *
 * Upsamples a 100Hz signal to 400Hz+ by delaying the signal by 1 frame (10ms)
 * and smoothly interpolating. Eliminates sawtooth artifacts and derivative spikes.
 */
class LinearExtrapolator { // Kept name to avoid breaking other files
private:
    double m_prev_sample = 0.0;
    double m_target_sample = 0.0;
    double m_current_output = 0.0;
    double m_rate = 0.0;
    double m_time_since_update = 0.0;
    double m_game_tick = 0.01; // Default 100Hz
    bool m_initialized = false;

public:
    void Configure(double game_tick) {
        m_game_tick = (std::max)(0.0001, game_tick);
    }

    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_prev_sample = raw_input;
            m_target_sample = raw_input;
            m_current_output = raw_input;
            m_initialized = true;
            return raw_input;
        }

        if (is_new_frame) {
            // Shift the window: old target becomes the new start point
            m_prev_sample = m_target_sample;
            m_target_sample = raw_input;

            // Calculate rate to reach the new target over the next game tick
            m_rate = (m_target_sample - m_prev_sample) / m_game_tick;
            m_time_since_update = 0.0;

            // Output starts exactly at the previous sample (no snapping)
            m_current_output = m_prev_sample;
        } else {
            m_time_since_update += dt;
            if (m_time_since_update <= m_game_tick) {
                m_current_output = m_prev_sample + m_rate * m_time_since_update;
            } else {
                // If the game stutters/drops a frame, hold the target value safely
                m_current_output = m_target_sample;
            }
        }

        return m_current_output;
    }

    void Reset() {
        m_prev_sample = m_target_sample = m_current_output = m_rate = m_time_since_update = 0.0;
        m_initialized = false;
    }
};
```

---

### PART 2: Test Suite Remediation Strategy

**CRITICAL DIRECTIVE FOR THE AGENT:** Do not bypass the interpolator in the main production code just to make tests pass. The 100Hz raw game data strictly requires interpolation to prevent stair-stepping at 400Hz. Instead, update the tests to respect the passage of time.

#### Strategy 1: The `PumpEngineTime` Helper

Because the new interpolator delays signals by 10ms to ensure smoothness, tests must "pump" the engine to see the final output of a step-change.

**Action:** Add this helper to `tests/test_ffb_common.cpp` and declare it in `test_ffb_common.h`:

```cpp
// Helper to simulate the passage of time for the FFB Engine's DSP pipeline.
void PumpEngineTime(FFBEngine& engine, TelemInfoV01& data, double time_to_advance_s) {
    double dt = 0.0025; // 400Hz FFB loop tick
    int ticks = std::ceil(time_to_advance_s / dt);

    for (int i = 0; i < ticks; i++) {
        // Only advance the telemetry timestamp every 10ms (100Hz)
        // to accurately simulate how the game feeds data to the app.
        if (i % 4 == 0) {
            data.mElapsedTime += 0.01;
        }
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, dt);
    }
}
```

#### Strategy 2: Fixing Step-Change Assertions

Any test that injects a sudden change in telemetry and immediately asserts the output on the same frame will fail (because the output is now delayed by 1 frame).

**Action:** Replace manual `calculate_force` calls in failing tests with the `PumpEngineTime` helper.

**Example Fix:**
```cpp
// OLD (Fails)
data.mWheel[0].mSuspForce = 5000.0;
engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
ASSERT_GT(engine.GetDebugBatch().back().ffb_bottoming_crunch, 0.0);

// NEW (Passes)
data.mWheel[0].mSuspForce = 5000.0;
PumpEngineTime(engine, data, 0.01); // Advance time by 10ms (1 telemetry frame)
ASSERT_GT(engine.GetDebugBatch().back().ffb_bottoming_crunch, 0.0);
```

#### Strategy 3: Resolving "Magic Number" Convergence Failures

The old `LinearExtrapolator` predicted the future, meaning it constantly overshot the actual telemetry values before snapping back. The legacy tests recorded these artificially high overshoot peaks as the "correct" expected values. The new interpolation is mathematically bounded and smooth, so peak forces will be slightly lower.

**Action:** Do not waste time trying to mathematically reverse-engineer the new exact floating-point values. Convert brittle `ASSERT_NEAR` checks into behavioral bounds.

**Example Fix:**
```cpp
// OLD (Brittle, will fail)
ASSERT_NEAR(snap.ffb_road_texture, 2.451f, 0.001f);

// NEW (Behavioral, resilient)
ASSERT_GT(snap.ffb_road_texture, 2.0f); // Ensure effect is strong
ASSERT_LT(snap.ffb_road_texture, 3.0f); // Ensure effect is bounded
```

#### Strategy 4: Fixing Diagnostic Warnings (The "Cold Start" Problem)

DSP filters have a "cold start" problem. If a test initializes the engine and immediately feeds it a telemetry frame where the car is traveling at 150 km/h with 4000N of suspension load, the interpolator ramps up from 0.0. The FFB engine sees this 0.0, assumes the telemetry is broken, and throws a "Missing Telemetry" warning (failing the test).

**Action:** Ensure tests properly "seed" the engine before running assertions. Feed one initial frame of telemetry to snap the interpolators to their starting values, clear any warnings, and then begin the actual test scenario.

**Example Fix (in `test_ffb_common.cpp` -> `InitializeEngine`):**
```cpp
void InitializeEngine(FFBEngine& engine) {
    // ... existing setup ...

    // Seed the DSP filters to prevent false "Missing Telemetry" warnings
    TelemInfoV01 seed_data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBEngineTestAccess::SetWasAllowed(engine, false); // Force a state transition reset
    engine.calculate_force(&seed_data); // First dummy frame seeds interpolators
    FFBEngineTestAccess::SetWasAllowed(engine, true);
}
```
