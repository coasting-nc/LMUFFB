#include "UpSampler.h"
#include <algorithm>

namespace LMUFFB {

PolyphaseResampler::PolyphaseResampler() {
    Reset();
}

void PolyphaseResampler::Reset() {
    m_phase = 0;
    m_needs_shift = false;
    m_pending_sample = 0.0;
    m_history.fill(0.0);
}

double PolyphaseResampler::Process(double latest_physics_sample, bool is_new_physics_tick) {
    if (is_new_physics_tick) {
        m_pending_sample = latest_physics_sample;
    }

    if (m_needs_shift) {
        // Shift history and add pending sample
        m_history[0] = m_history[1];
        m_history[1] = m_history[2];
        m_history[2] = m_pending_sample;
        m_needs_shift = false;
    }

    // Apply the 3-tap FIR filter for the current phase
    const double* c = COEFFS[m_phase];

    // FIX 1: Correct convolution order.
    // c[0] is the earliest tap, so it must multiply the newest sample (m_history[2]).
    // y[n] = c[0]*x[n] + c[1]*x[n-1] + c[2]*x[n-2]
    double output = c[0] * m_history[2] + c[1] * m_history[1] + c[2] * m_history[0];

    // Advance phase (1000Hz ticks)
    // The phase accumulator in main.cpp handles the 2/5 relationship,
    // but the resampler itself needs to know which branch of the polyphase filter to use.
    // Each call to Process is one 1000Hz tick.

    // FIX 2: For a 5/2 resampling ratio (400Hz to 1000Hz), the phase must advance by 2
    // modulo 5 for every output sample.
    m_phase += 2;
    if (m_phase >= 5) {
        m_phase -= 5;
        m_needs_shift = true;
    }

    return output;
}

} // namespace LMUFFB
