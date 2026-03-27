#ifndef UPSAMPLER_H
#define UPSAMPLER_H

#include <array>

namespace LMUFFB::FFB {

/**
 * @brief Polyphase Resampler for 5/2 ratio (400Hz -> 1000Hz)
 *
 * Upsamples by 5, then downsamples by 2.
 * Uses a 15-tap FIR filter (3 taps per phase) with 200Hz cutoff.
 */
class PolyphaseResampler {
public:
    PolyphaseResampler();

    /**
     * @brief Process a single 1000Hz tick.
     *
     * @param latest_physics_sample The most recent output from the 400Hz physics loop.
     * @param is_new_physics_tick True if the physics loop just ran and produced a new sample.
     * @return The upsampled 1000Hz force value.
     */
    double Process(double latest_physics_sample, bool is_new_physics_tick);

    void Reset();

private:
    int m_phase; // Current phase (0-4)
    bool m_needs_shift;
    double m_pending_sample;
    std::array<double, 3> m_history; // History of physics samples (400Hz)

    // Polyphase coefficients (5 phases, 3 taps each)
    // Designed for 200Hz cutoff @ 2000Hz sample rate (upsampled domain)
    // Normalized so each phase sums exactly to 1.0 to preserve DC gain.
    static constexpr double COEFFS[5][3] = {
        {-0.01855,  0.67102,  0.34753}, // Phase 0
        {-0.02009,  0.91514,  0.10495}, // Phase 1
        { 0.00000,  1.00000,  0.00000}, // Phase 2 (Identity phase)
        { 0.10495,  0.91514, -0.02009}, // Phase 3
        { 0.34753,  0.67102, -0.01855}  // Phase 4
    };
};

} // namespace LMUFFB::FFB

// Temporary bridge alias to keep existing call sites compiling
namespace LMUFFB {
    using PolyphaseResampler = LMUFFB::FFB::PolyphaseResampler;
}

#endif // UPSAMPLER_H
