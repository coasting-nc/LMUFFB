# Fix additional issues related to the FIR filter #393

**Open**
**coasting-nc** opened this issue on Mar 16, 2026

## Description

Fix additional issues related to the FIR filter: "Polyphase Resampler Phase Misalignment (The "Stutter" Bug)" and "Holt-Winters Filter 100Hz Sawtooth (The "Buzz" Bug)".

Looking deeper into the up-sampling pipeline, there are two more critical flaws that directly contribute to the "cogwheel" and irregular vibration feelings.

While the previous fix (#385) corrected the FIR filter's shape, the way data is fed into the filter and how the primary torque signal is smoothed are both fundamentally broken.

Here are the two major issues and how to fix them:

### 1. Polyphase Resampler Phase Misalignment (The "Stutter" Bug)

In main.cpp, the physics loop runs at 400Hz inside a 1000Hz loop. This means a new physics sample is generated at uneven intervals (every 3 ticks, then 2 ticks, then 3 ticks, etc.).

Currently, PolyphaseResampler::Process shifts its history buffer the exact moment is_new_physics_tick is true. However, mathematically, the polyphase filter isn't ready for that new sample yet—it still needs the old sample to calculate the final fractional phase of the previous curve. By shifting the buffer one tick too early, the filter uses data from the future, causing a micro-stutter every 2.5ms.

**The Fix:** The resampler must decouple the arrival of the data from the shifting of its internal buffer. It should store the new sample in a "pending" state and only shift the history buffer when its internal phase accumulator wraps around.

Update src/ffb/UpSampler.h:
```cpp
class PolyphaseResampler {
public:
    PolyphaseResampler();
    double Process(double latest_physics_sample, bool is_new_physics_tick);
    void Reset();

private:
    int m_phase;
    bool m_needs_shift;          // NEW
    double m_pending_sample;     // NEW
    std::array<double, 3> m_history;
    // ... COEFFS array remains the same
};
```

Update src/ffb/UpSampler.cpp:
```cpp
void PolyphaseResampler::Reset() {
    m_phase = 0;
    m_needs_shift = false;
    m_pending_sample = 0.0;
    m_history.fill(0.0);
}

double PolyphaseResampler::Process(double latest_physics_sample, bool is_new_physics_tick) {
    // 1. Store the new sample, but DO NOT shift the buffer yet
    if (is_new_physics_tick) {
        m_pending_sample = latest_physics_sample;
    }

    // 2. Shift the buffer ONLY when the fractional phase wraps around
    if (m_needs_shift) {
        m_history[0] = m_history[1];
        m_history[1] = m_history[2];
        m_history[2] = m_pending_sample;
        m_needs_shift = false;
    }

    const double* c = COEFFS[m_phase];
    double output = c[0] * m_history[2] + c[1] * m_history[1] + c[2] * m_history[0];

    // 3. Advance phase by 2 (for 5/2 ratio). If it wraps >= 5, flag a shift for the NEXT tick.
    m_phase += 2;
    if (m_phase >= 5) {
        m_phase -= 5;
        m_needs_shift = true;
    }

    return output;
}
```

### 2. Holt-Winters Filter 100Hz Sawtooth (The "Buzz" Bug)

In src/utils/MathUtils.h, the HoltWintersFilter is used to upsample and smooth the raw mSteeringShaftTorque from the game.

Look at what happens when a new frame arrives:
```cpp
if (is_new_frame) {
    double prev_level = m_level;
    m_level = m_alpha * raw_input + (1.0 - m_alpha) * (m_level + m_trend * m_game_tick);
    m_trend = m_beta * ((m_level - prev_level) / m_game_tick) + (1.0 - m_beta) * m_trend;
    m_time_since_update = 0.0;

    // BUG: Returning raw_input bypasses the smoothing entirely!
    return raw_input;
}
```

The comment in the code says it returns raw_input to "avoid breaking existing tests". However, this completely destroys the FFB signal. Every 10ms (100Hz), the output spikes to the raw, noisy input, and then immediately drops back down to the smoothed m_level on the next 400Hz tick. This creates a massive 100Hz sawtooth wave that feels exactly like a buzzing cogwheel in the user's hands.

**The Fix:** You must return the smoothed m_level. (Note: You will likely need to update your unit tests to expect the smoothed value rather than the raw value).

Update src/utils/MathUtils.h inside HoltWintersFilter::Process:
```cpp
        if (is_new_frame) {
            double prev_level = m_level;
            m_level = m_alpha * raw_input + (1.0 - m_alpha) * (m_level + m_trend * m_game_tick);
            m_trend = m_beta * ((m_level - prev_level) / m_game_tick) + (1.0 - m_beta) * m_trend;
            m_time_since_update = 0.0;

            // FIX: Return the smoothed level to maintain a continuous signal
            return m_level;
        } else {
            m_time_since_update += dt;
        }

        return m_level + m_trend * m_time_since_update;
```

### Summary

By fixing the Convolution Order (from the previous prompt), the Phase Alignment (PolyphaseResampler), and the Sawtooth Bypass (HoltWintersFilter), the up-sampling pipeline will finally output a mathematically continuous, artifact-free signal. This will completely eliminate the artificial vibrations the user is reporting.
