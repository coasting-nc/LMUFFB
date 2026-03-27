#pragma once

#include "../ffb/FFBSnapshot.h"
#include "../ffb/FFBSafetyMonitor.h"
#include "../core/Config.h"
#include "../io/lmu_sm_interface/InternalsPlugin.hpp"

namespace LMUFFB {
namespace Physics {
namespace SteeringUtils {

/**
 * @brief Calculate Soft Lock force
 * Provides a progressive spring-damping force when the wheel exceeds 100% lock.
 */
void CalculateSoftLock(const TelemInfoV01* data, 
                       FFBCalculationContext& ctx, 
                       const AdvancedConfig& advanced_cfg, 
                       const GeneralConfig& general_cfg, 
                       FFBSafetyMonitor& safety, 
                       double steering_velocity_smoothed);

} // namespace SteeringUtils
} // namespace Physics
} // namespace LMUFFB
