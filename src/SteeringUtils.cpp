#include "FFBEngine.h"
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Steering & Wheel Mechanics methods have been moved from FFBEngine.cpp.
// This includes the Soft Lock logic which prevents the wheel from rotating
// beyond the car's physical steering rack limits.
// ---------------------------------------------------------------------------

// Helper: Calculate Soft Lock (v0.7.61 - Issue #117)
// Provides a progressive spring-damping force when the wheel exceeds 100% lock.
void FFBEngine::calculate_soft_lock(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    ctx.soft_lock_force = 0.0;
    if (!m_soft_lock_enabled) return;

    double steer = data->mUnfilteredSteering;
    if (!std::isfinite(steer)) return;

    double abs_steer = std::abs(steer);
    if (abs_steer > 1.0) {
        double excess = abs_steer - 1.0;
        double sign = (steer > 0.0) ? 1.0 : -1.0;

        // Spring Force: pushes back to 1.0
        double spring = excess * m_soft_lock_stiffness * (double)BASE_NM_SOFT_LOCK;

        // Damping Force: opposes movement to prevent bouncing
        // Uses m_steering_velocity_smoothed which is in rad/s
        double damping = m_steering_velocity_smoothed * m_soft_lock_damping * (double)BASE_NM_SOFT_LOCK;

        // Total Soft Lock force (opposing the steering direction)
        // Note: damping already has a sign from m_steering_velocity_smoothed.
        // If moving further away from limit, damping should oppose it.
        // If returning to center, damping should also oppose it (slowing down the return).
        ctx.soft_lock_force = -(spring * sign + damping);
    }
}
