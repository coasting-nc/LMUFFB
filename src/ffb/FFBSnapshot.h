#ifndef FFBSNAPSHOT_H
#define FFBSNAPSHOT_H

#include <stdint.h>

// 1. Define the Snapshot Struct (Unified FFB + Telemetry)
struct FFBSnapshot {
    // --- Header A: FFB Components (Outputs) ---
    float total_output;
    float base_force;
    float sop_force;
    float lat_load_force;   // New v0.7.154 (Issue #282)
    float understeer_drop;
    float oversteer_boost;
    float ffb_rear_torque;  // New v0.4.7
    float ffb_scrub_drag;   // New v0.4.7
    float ffb_yaw_kick;     // New v0.4.16
    float ffb_gyro_damping; // New v0.4.17
    float texture_road;
    float texture_slide;
    float texture_lockup;
    float texture_spin;
    float texture_bottoming;
    float ffb_abs_pulse;    // New v0.7.53
    float long_load_force;  // New #301
    float ffb_soft_lock;    // New v0.7.61 (Issue #117)
    float session_peak_torque; // New v0.7.67 (Issue #152)
    float clipping;

    // --- Header B: Internal Physics (Calculated) ---
    float calc_front_load;       // New v0.4.7
    float calc_rear_load;        // New v0.4.10
    float calc_rear_lat_force;   // New v0.4.10
    float calc_front_grip;       // New v0.4.7
    float calc_rear_grip;        // New v0.4.7 (Refined)
    float calc_front_slip_ratio; // New v0.4.7 (Manual Calc)
    float calc_front_slip_angle_smoothed; // Renamed from slip_angle
    float raw_front_slip_angle;  // New v0.4.7 (Raw atan2)
    float calc_rear_slip_angle_smoothed; // New v0.4.9
    float raw_rear_slip_angle;   // New v0.4.9 (Raw atan2)

    // --- Header C: Raw Game Telemetry (Inputs) ---
    float steer_force;
    float raw_shaft_torque;      // New v0.7.62 (Issue #138)
    float raw_gen_torque;        // New v0.7.62 (Issue #138)
    float raw_input_steering;    // New v0.4.7 (Unfiltered -1 to 1)
    float raw_front_tire_load;   // New v0.4.7
    float raw_front_grip_fract;  // New v0.4.7
    float raw_rear_grip;         // New v0.4.7
    float raw_front_susp_force;  // New v0.4.7
    float raw_front_ride_height; // New v0.4.7
    float raw_rear_lat_force;    // New v0.4.7
    float raw_car_speed;         // New v0.4.7
    float raw_front_slip_ratio;  // New v0.4.7 (Game API)
    float raw_input_throttle;    // New v0.4.7
    float raw_input_brake;       // New v0.4.7
    float accel_x;
    float raw_front_lat_patch_vel; // Renamed from patch_vel
    float raw_front_deflection;    // Renamed from deflection
    float raw_front_long_patch_vel; // New v0.4.9
    float raw_rear_lat_patch_vel;   // New v0.4.9
    float raw_rear_long_patch_vel;  // New v0.4.9
    float steering_angle_deg;       // New v0.7.112 (Issue #218)
    float steering_range_deg;       // New v0.7.112 (Issue #218)

    // Telemetry Health Flags
    bool warn_load;
    bool warn_grip;
    bool warn_dt;

    float debug_freq; // New v0.4.41: Frequency for diagnostics
    float tire_radius; // New v0.4.41: Tire radius in meters for theoretical freq calculation
    float slope_current; // New v0.7.1: Slope detection derivative value
    float slope_dG_dt; // New v0.7.198 (Issue #397): Expose for regression tests
    float slope_dAlpha_dt; // New v0.7.198 (Issue #397): Expose for regression tests

    // Rate Monitoring (Issue #129)
    float ffb_rate;
    float telemetry_rate;
    float hw_rate;
    float torque_rate;
    float gen_torque_rate;
    float physics_rate; // New v0.7.117 (Issue #217)
};

#endif // FFBSNAPSHOT_H
