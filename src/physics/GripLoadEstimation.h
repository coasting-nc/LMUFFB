#ifndef GRIP_LOAD_ESTIMATION_H
#define GRIP_LOAD_ESTIMATION_H

#include "VehicleUtils.h"
#include "../io/lmu_sm_interface/InternalsPluginWrapper.h"
#include <string>

namespace LMUFFB {
namespace Physics {

/**
 * @brief Load transformation modes for non-linear FFB scaling
 */
enum class LoadTransform {
    LINEAR = 0,
    CUBIC = 1,
    QUADRATIC = 2,
    HERMITE = 3
};

/**
 * @brief Helper Result Struct for calculate_axle_grip
 */
struct GripResult {
    double value;           // Final grip value
    bool approximated;      // Was approximation used?
    double original;        // Original telemetry value
    double slip_angle;      // Calculated slip angle (if approximated)
};

// Default FFB calculation timestep. Used by FFBCalculationContext (defined before
// FFBEngine, so cannot reference FFBEngine::DEFAULT_CALC_DT directly).
// Note: FFBEngine also has a private member of the same name; this file-scope
// constant does NOT trigger GCC's -Wchanges-meaning because it is only looked up
// inside FFBCalculationContext, not inside FFBEngine's own class body.
static constexpr double PHYSICS_CALC_DT = 0.0025; // 400 Hz (1/400 s)

/**
 * @brief Context structure for FFB calculations in a single frame
 */
struct FFBCalculationContext {
    double dt = PHYSICS_CALC_DT;
    double car_speed = 0.0;       // Absolute m/s
    double car_speed_long = 0.0;  // Longitudinal m/s (Raw)
    double speed_gate = 1.0;
    double texture_load_factor = 1.0;
    double brake_load_factor = 1.0;
    double avg_front_load = 0.0;
    double avg_front_grip = 0.0;

    // Diagnostics
    bool frame_warn_load = false;
    bool frame_warn_grip = false;
    bool frame_warn_rear_grip = false;
    bool frame_warn_dt = false;

    // Intermediate results
    double grip_factor = 1.0;     // 1.0 = full grip, 0.0 = no grip
    double sop_base_force = 0.0;
    double sop_unboosted_force = 0.0; // For snapshot compatibility
    double lat_load_force = 0.0;  // New v0.7.154 (Issue #282)
    double rear_torque = 0.0;
    double yaw_force = 0.0;
    double scrub_drag_force = 0.0;
    double gyro_force = 0.0;
    double stationary_damping_force = 0.0; // New v0.7.206 (Issue #418)
    double avg_rear_grip = 0.0;
    double calc_rear_lat_force = 0.0;
    double avg_rear_load = 0.0;
    double long_load_force = 0.0; // New #301

    // Effect outputs
    double road_noise = 0.0;
    double slide_noise = 0.0;
    double lockup_rumble = 0.0;
    double spin_rumble = 0.0;
    double bottoming_crunch = 0.0;
    double abs_pulse_force = 0.0;
    double soft_lock_force = 0.0;
    double gain_reduction_factor = 1.0;
};

// --- Physics Logic Functions (Decoupled from FFBEngine) ---

/**
 * @brief Helper: Calculate Raw Slip Angle for a pair of wheels (v0.4.9 Refactor)
 * Returns the average slip angle of two wheels using atan2(lateral_vel, longitudinal_vel)
 */
double CalculateRawSlipAnglePair(const TelemWheelV01& w1, const TelemWheelV01& w2);

/**
 * @brief Helper: Calculate Slip Angle with LPF (v0.4.19/v0.4.37)
 * Preserve sign for directional counter-steering.
 */
double CalculateSlipAngle(const TelemWheelV01& w, double& prev_state, double dt, float slip_angle_smoothing);

/**
 * @brief Helper: Calculate Manual Slip Ratio (v0.4.6)
 */
double CalculateManualSlipRatio(const TelemWheelV01& w, double car_speed_ms);

/**
 * @brief Helper: Calculate Slip Ratio from wheel (v0.6.36)
 */
double CalculateWheelSlipRatio(const TelemWheelV01& w);

/**
 * @brief Helper: Approximate Tire Load from suspension force (v0.4.5/v0.7.175)
 * Corrects pushrod force to wheel load using Motion Ratio.
 */
double CalculateApproximateLoad(const TelemWheelV01& w, ParsedVehicleClass vclass, bool is_rear);

} // namespace Physics

// Bridge Aliases for backward compatibility during migration
using LoadTransform = Physics::LoadTransform;
using GripResult = Physics::GripResult;
using FFBCalculationContext = Physics::FFBCalculationContext;

} // namespace LMUFFB

#endif // GRIP_LOAD_ESTIMATION_H
