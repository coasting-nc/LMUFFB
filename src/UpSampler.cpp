#include "UpSampler.h"
#include <algorithm>

namespace ffb_math {

PolyphaseResampler::PolyphaseResampler() {
    Reset();
}

void PolyphaseResampler::Reset() {
    m_phase = 0;
    m_history.fill(0.0);
}

double PolyphaseResampler::Process(double latest_physics_sample, bool is_new_physics_tick) {
    if (is_new_physics_tick) {
        // Shift history and add new sample
        m_history[0] = m_history[1];
        m_history[1] = m_history[2];
        m_history[2] = latest_physics_sample;
    }

    // Apply the 3-tap FIR filter for the current phase
    const double* c = COEFFS[m_phase];
    double output = c[0] * m_history[0] + c[1] * m_history[1] + c[2] * m_history[2];

    // Advance phase (1000Hz ticks)
    // The phase accumulator in main.cpp handles the 2/5 relationship,
    // but the resampler itself needs to know which branch of the polyphase filter to use.
    // Each call to Process is one 1000Hz tick.
    m_phase = (m_phase + 1) % 5;

    return output;
}

} // namespace ffb_math
