#include "SteeringUtils.h"
#include "../logging/Logger.h"
#include <cmath>
#include <algorithm>

using namespace LMUFFB::Logging;

namespace LMUFFB {
namespace SteeringUtils {

// ---------------------------------------------------------------------------
// Steering & Wheel Mechanics methods
// This includes the Soft Lock logic which prevents the wheel from rotating
// beyond the car's physical steering rack limits.
// ---------------------------------------------------------------------------

// Helper: Calculate Soft Lock (v0.7.61 - Issue #117)
// Provides a progressive spring-damping force when the wheel exceeds 100% lock.
void CalculateSoftLock(const TelemInfoV01* data, 
                       FFBCalculationContext& ctx, 
                       const AdvancedConfig& advanced_cfg, 
                       const GeneralConfig& general_cfg, 
                       FFBSafetyMonitor& safety, 
                       double steering_velocity_smoothed) {
    ctx.soft_lock_force = 0.0;
    if (!advanced_cfg.soft_lock_enabled) return;

    double steer = data->mUnfilteredSteering;
    if (!std::isfinite(steer)) return;

    double abs_steer = std::abs(steer);
    if (abs_steer > 1.0) {
        if (!safety.was_soft_locked) {
            Logger::Get().LogFile("[Safety] Soft Lock Engaged: Steering %.1f%%", steer * 100.0);
            safety.was_soft_locked = true;
        }

        double excess = abs_steer - 1.0;
        double sign = (steer > 0.0) ? 1.0 : -1.0;

        // NEW: Steep Spring Ramp reaching hardware max torque quickly.
        // This ensures the soft lock feels like a solid wall that doesn't "give"
        // under pressure, even at zero velocity.
        // At stiffness 20 (default), reaches 100% force at 0.25% excess.
        // At stiffness 100 (max), reaches 100% force at 0.05% excess.
        double stiffness = (double)(std::max)(1.0f, advanced_cfg.soft_lock_stiffness);
        double excess_for_max = 5.0 / (stiffness * 100.0);
        double spring_nm = (std::min)(1.0, excess / excess_for_max) * (double)general_cfg.wheelbase_max_nm * 2.0;

        // Damping Force: opposes movement to prevent bouncing.
        // Scaled by hardware torque to remain relevant across all wheelbases.
        double damping_nm = steering_velocity_smoothed * advanced_cfg.soft_lock_damping * (double)general_cfg.wheelbase_max_nm * 0.1;
        damping_nm = std::clamp(damping_nm, -(double)general_cfg.wheelbase_max_nm * 0.5, (double)general_cfg.wheelbase_max_nm * 0.5);

        // Total Soft Lock force (opposing the steering direction)
        ctx.soft_lock_force = -(spring_nm * sign + damping_nm);

        if (std::abs(ctx.soft_lock_force) > 5.0 && !safety.soft_lock_significant) {
            Logger::Get().LogFile("[Safety] Soft Lock Significant Influence: %.1f Nm", ctx.soft_lock_force);
            safety.soft_lock_significant = true;
        } else if (std::abs(ctx.soft_lock_force) < 4.0) {
            safety.soft_lock_significant = false;
        }
    } else {
        if (safety.was_soft_locked) {
            Logger::Get().LogFile("[Safety] Soft Lock Disengaged");
            safety.was_soft_locked = false;
        }
        safety.soft_lock_significant = false;
    }
}

} // namespace SteeringUtils
} // namespace LMUFFB
