#pragma once

namespace LMUFFB {

/**
 * @brief Standard wheel indices used for rFactor 2 / LMU telemetry arrays.
 * 
 * Mapping:
 * 0: Front Left
 * 1: Front Right
 * 2: Rear Left
 * 3: Rear Right
 */
enum WheelIndex : int {
    WHEEL_FL = 0,
    WHEEL_FR = 1,
    WHEEL_RL = 2,
    WHEEL_RR = 3
};

/**
 * @brief Total number of wheels in the simulation.
 */
static constexpr int NUM_WHEELS = 4;

/**
 * @brief Total number of axles in a standard 4-wheel vehicle.
 */
static constexpr int NUM_AXLES = 2;

} // namespace LMUFFB
