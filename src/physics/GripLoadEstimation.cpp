// GripLoadEstimation.cpp
// ---------------------------------------------------------------------------
// Grip and Load Estimation methods for FFBEngine.
//
// This file is a source-level split of FFBEngine.cpp (Approach A).
// All functions here are FFBEngine member methods; FFBEngine.h is unchanged.
//
// Rationale: These functions form a self-contained "data preparation" layer ---
// they take raw (possibly broken/encrypted) telemetry and produce the best
// available grip and load values. The FFB engine then consumes those values
// without needing to know how they were estimated.
//
// See docs/dev_docs/reports/FFBEngine_refactoring_analysis.md for full details.
// ---------------------------------------------------------------------------

#include "physics/GripLoadEstimation.h"
#include "ffb/FFBEngine.h"
#include "core/Config.h"
#include "logging/Logger.h"
#include <iostream>

#include <mutex>
#include <cmath>
#include "utils/StringUtils.h"

using namespace LMUFFB::Logging;
using namespace LMUFFB::Utils;
using namespace LMUFFB::Physics;

namespace LMUFFB {

extern std::recursive_mutex g_engine_mutex;

namespace Physics {

// --- Physics Logic Implementation (Decoupled) ---

// Helper: Calculate Raw Slip Angle for a pair of wheels (v0.4.9 Refactor)
// Returns the average slip angle of two wheels using atan2(lateral_vel, longitudinal_vel)
// v0.4.19: Removed abs() from lateral velocity to preserve sign for debug visualization
double CalculateRawSlipAnglePair(const TelemWheelV01& w1, const TelemWheelV01& w2) {
    double v_long_1 = std::abs(w1.mLongitudinalGroundVel);
    double v_long_2 = std::abs(w2.mLongitudinalGroundVel);
    if (v_long_1 < 0.5) v_long_1 = 0.5; // MIN_SLIP_ANGLE_VELOCITY
    if (v_long_2 < 0.5) v_long_2 = 0.5;
    double raw_angle_1 = std::atan2(w1.mLateralPatchVel, v_long_1);
    double raw_angle_2 = std::atan2(w2.mLateralPatchVel, v_long_2);
    return (raw_angle_1 + raw_angle_2) / 2.0;
}

double CalculateSlipAngle(const TelemWheelV01& w, double& prev_state, double dt, float slip_angle_smoothing) {
    double v_long = std::abs(w.mLongitudinalGroundVel);
    if (v_long < 0.5) v_long = 0.5; // MIN_SLIP_ANGLE_VELOCITY

    // v0.4.19: PRESERVE SIGN - Do NOT use abs() on lateral velocity
    // Positive lateral vel (+X = left) -> Positive slip angle
    // Negative lateral vel (-X = right) -> Negative slip angle
    // This sign is critical for directional counter-steering
    double raw_angle = std::atan2(w.mLateralPatchVel, v_long);  // SIGN PRESERVED

    // LPF: Time Corrected Alpha (v0.4.37)
    // Target: Alpha 0.1 at 400Hz (dt = 0.0025)
    // Formula: alpha = dt / (tau + dt) -> 0.1 = 0.0025 / (tau + 0.0025) -> tau approx 0.0225s
    // v0.4.40: Using configurable m_slip_angle_smoothing
    double tau = (double)slip_angle_smoothing;
    if (tau < 0.0001) tau = 0.0001; // Safety clamp

    double alpha = dt / (tau + dt);

    // Safety clamp
    alpha = (std::min)(1.0, (std::max)(0.001, alpha));
    prev_state = prev_state + alpha * (raw_angle - prev_state);
    return prev_state;
}

// Helper: Calculate Manual Slip Ratio (v0.4.6)
double CalculateManualSlipRatio(const TelemWheelV01& w, double car_speed_ms) {
    // Safety Trap: Force 0 slip at very low speeds (v0.4.6)
    if (std::abs(car_speed_ms) < 2.0) return 0.0;

    // Radius in meters (stored as cm unsigned char)
    // Explicit cast to double before division (v0.4.6)
    double radius_m = (double)w.mStaticUndeflectedRadius / 100.0;
    if (radius_m < 0.1) radius_m = 0.33; // Fallback if 0 or invalid

    double wheel_vel = w.mRotation * radius_m;

    // Avoid div-by-zero at standstill
    double denom = std::abs(car_speed_ms);
    if (denom < 1.0) denom = 1.0;

    // Ratio = (V_wheel - V_car) / V_car
    // Lockup: V_wheel < V_car -> Ratio < 0
    // Spin: V_wheel > V_car -> Ratio > 0
    return (wheel_vel - car_speed_ms) / denom;
}

// Helper: Calculate Slip Ratio from wheel (v0.6.36 - Extracted from lambdas)
// Unified slip ratio calculation for lockup and spin detection.
// Returns the ratio of longitudinal slip: (PatchVel - GroundVel) / GroundVel
double CalculateWheelSlipRatio(const TelemWheelV01& w) {
    double v_long = std::abs(w.mLongitudinalGroundVel);
    if (std::abs(v_long) < 0.5) v_long = 0.5; // MIN_SLIP_ANGLE_VELOCITY
    return w.mLongitudinalPatchVel / v_long;
}

// ========================================================================================
// CRITICAL VEHICLE DYNAMICS NOTE: mSuspForce vs Wheel Load
// ========================================================================================
// The LMU telemetry channel `mSuspForce` represents the load on the internal PUSHROD,
// NOT the load at the tire contact patch.
// Because race cars use bellcranks, the pushrod has a mechanical advantage (Motion Ratio).
// For example, a Hypercar with a Motion Ratio of 0.5 means the pushrod moves half as far
// as the wheel, and therefore carries TWICE the force of the wheel.
// To approximate the actual Tire Load, we MUST multiply mSuspForce by the Motion Ratio,
// and then add the unsprung mass (the weight of the wheel, tire, and brakes).
// ========================================================================================

// Helper: Approximate Load (v0.4.5 / Improved v0.7.171 / Refactored v0.7.175)
// Uses class-specific motion ratios to convert pushrod force (mSuspForce) to wheel load,
// and adds an estimate for unsprung mass.
double CalculateApproximateLoad(const TelemWheelV01& w, ParsedVehicleClass vclass, bool is_rear) {
    double motion_ratio = GetMotionRatioForClass(vclass);
    double unsprung_weight = GetUnsprungWeightForClass(vclass, is_rear);

    return (std::max)(0.0, (w.mSuspForce * motion_ratio) + unsprung_weight);
}

} // namespace Physics

// --- FFBEngine member implementations ---

// Helper: Learn static front and rear load reference (v0.7.46, expanded v0.7.164)
void FFBEngine::update_static_load_reference(double current_front_load, double current_rear_load, double speed, double dt) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    if (m_static_load_latched) return; // Do not update if latched

    if (speed > 2.0 && speed < 15.0) {
        // Front Load Learning
        if (m_static_front_load < 100.0) {
             m_static_front_load = current_front_load;
        } else {
             m_static_front_load += (dt / 5.0) * (current_front_load - m_static_front_load);
        }
        // Rear Load Learning
        if (m_static_rear_load < 100.0) {
             m_static_rear_load = current_rear_load;
        } else {
             m_static_rear_load += (dt / 5.0) * (current_rear_load - m_static_rear_load);
        }
    } else if (speed >= 15.0 && m_static_front_load > 1000.0 && m_static_rear_load > 1000.0) {
        // Latch the value once we exceed 15 m/s (aero begins to take over)
        m_static_load_latched = true;

        // Save to config map (v0.7.70)
        std::string vName = m_metadata.GetVehicleName();
        if (vName != "Unknown" && vName != "") {
            Config::SetSavedStaticLoad(vName, m_static_front_load);
            Config::SetSavedStaticLoad(vName + "_rear", m_static_rear_load);
            Config::m_needs_save = true; // Flag main thread to write to disk
            Logger::Get().LogFile("[FFB] Latched and saved static loads for %s: F=%.2fN, R=%.2fN", vName.c_str(), m_static_front_load, m_static_rear_load);
        }
    }

    // Safety fallback: Ensure we have a sane baseline if learning failed
    if (m_static_front_load < 1000.0) {
        m_static_front_load = m_auto_peak_front_load * 0.5;
    }
    if (m_static_rear_load < 1000.0) {
        m_static_rear_load = m_auto_peak_front_load * 0.5;
    }
}

// Initialize the load reference based on vehicle class and name seeding
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    // v0.7.109: Perform a full normalization reset on car change
    // This ensures that session-learned peaks from a previous car don't pollute the new session.
    ResetNormalization();

    // --- FIX #374 & #379: Reset all historical data across car change ---
    m_metadata.ResetWarnings();
    m_missing_load_frames = 0;
    m_missing_lat_force_front_frames = 0;
    m_missing_lat_force_rear_frames = 0;
    m_missing_susp_force_frames = 0;
    m_missing_susp_deflection_frames = 0;
    m_missing_vert_deflection_frames = 0;

    m_warned_load = false;
    m_warned_grip = false;
    m_warned_rear_grip = false;
    m_warned_lat_force_front = false;
    m_warned_lat_force_rear = false;
    m_warned_susp_force = false;
    m_warned_susp_deflection = false;
    m_warned_vert_deflection = false;

    // Reset seeding flags to trigger fresh capture on first frame of new car
    m_yaw_rate_seeded = false;
    m_yaw_accel_seeded = false;
    m_local_vel_seeded = false;
    m_yaw_rate_log_seeded = false;
    m_derivatives_seeded = false;

    // Clear slope detection buffers
    m_slope_buffer_count = 0;
    m_slope_buffer_index = 0;
    m_slope_lat_g_buffer.fill(0.0);
    m_slope_slip_buffer.fill(0.0);
    m_slope_torque_buffer.fill(0.0);
    m_slope_steer_buffer.fill(0.0);
    m_slope_current = 0.0;
    m_slope_smoothed_output = 1.0;
    // -----------------------------------------------------------------------

    ParsedVehicleClass vclass = Physics::ParseVehicleClass(className, vehicleName);

    // Stage 3 Reset: Ensure peak load starts at class baseline
    m_auto_peak_front_load = Physics::GetDefaultLoadForClass(vclass);

    std::string vName = vehicleName ? vehicleName : "Unknown";

    // Check if we already have a saved static load for this specific car (v0.7.70)
    double saved_front_load = 0.0;
    double saved_rear_load = 0.0;
    if (Config::GetSavedStaticLoad(vName, saved_front_load)) {
        m_static_front_load = saved_front_load;

        if (Config::GetSavedStaticLoad(vName + "_rear", saved_rear_load)) {
            m_static_rear_load = saved_rear_load;
        } else {
            // Migration: If we have front but no rear, estimate rear based on class default
            m_static_rear_load = m_auto_peak_front_load * 0.5;
        }

        m_static_load_latched = true; // Skip the 2-15 m/s learning phase
        Logger::Get().LogFile("[FFB] Loaded persistent static loads for %s: F=%.2fN, R=%.2fN", vName.c_str(), m_static_front_load, m_static_rear_load);
    } else {
        // Reset static load reference for new car class
        m_static_front_load = m_auto_peak_front_load * 0.5;
        m_static_rear_load = m_auto_peak_front_load * 0.5;
        m_static_load_latched = false;
        Logger::Get().LogFile("[FFB] No saved load for %s. Learning required.", vName.c_str());
    }

    m_smoothed_vibration_mult = 1.0;

    // v0.7.119: Update engine's car name immediately to prevent seeding loop (Issue #238)
    m_metadata.m_last_handled_vehicle_name = vName;

    // Sync metadata manager state
    m_metadata.UpdateInternal(className, vehicleName, nullptr);

    // Issue #368: Log strings with quotes to reveal hidden spaces if detection fails
    Logger::Get().LogFile("[FFB] Vehicle Identification -> Detected Class: %s | Seed Load: %.2fN (Raw -> Class: '%s', Name: '%s')",
        Physics::VehicleClassToString(vclass), m_auto_peak_front_load, (className ? className : "Unknown"), vName.c_str());
}

double FFBEngine::calculate_raw_slip_angle_pair(const TelemWheelV01& w1, const TelemWheelV01& w2) {
    return Physics::CalculateRawSlipAnglePair(w1, w2);
}

double FFBEngine::calculate_slip_angle(const TelemWheelV01& w, double& prev_state, double dt) {
    return Physics::CalculateSlipAngle(w, prev_state, dt, m_grip_estimation.slip_angle_smoothing);
}

// Helper: Calculate Axle Grip with Fallback (v0.4.6 Hardening)
// This function calculates the average grip for a pair of wheels (axle).
// If the primary telemetry grip is missing, it reconstructs it from slip angle and ratio.
// The avg_axle_load parameter is used as a threshold for triggering the reconstruction fallback.
Physics::GripResult FFBEngine::calculate_axle_grip(const TelemWheelV01& w1,
                          const TelemWheelV01& w2,
                          double avg_axle_load,
                          bool& warned_flag,
                          double& prev_slip1,
                          double& prev_slip2,
                          double& prev_load1, // NEW: State for load smoothing
                          double& prev_load2, // NEW: State for load smoothing
                          double car_speed,
                          double dt,
                          const char* vehicleName,
                          const TelemInfoV01* data,
                          bool is_front) {
    // Note on mGripFract: The LMU InternalsPlugin.hpp comments state this is the
    // "fraction of the contact patch that is sliding". This is poorly phrased.
    // In actual telemetry output, 1.0 = Full Adhesion (Gripping) and 0.0 = Fully Sliding.
    Physics::GripResult result;
    double total_load = w1.mTireLoad + w2.mTireLoad;
    if (total_load > 1.0) {
        result.original = (w1.mGripFract * w1.mTireLoad + w2.mGripFract * w2.mTireLoad) / total_load;
    } else {
        // Fallback for zero load (e.g. jump/missing data)
        result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
    }

    result.value = result.original;
    result.approximated = false;
    result.slip_angle = 0.0;
    
    // ==================================================================================
    // CRITICAL LOGIC FIX (v0.4.14) - DO NOT MOVE INSIDE CONDITIONAL BLOCK
    // ==================================================================================
    // We MUST calculate slip angle every single frame, regardless of whether the
    // grip fallback is triggered or not.
    //
    // Reason 1 (Physics State): The Low Pass Filter (LPF) inside calculate_slip_angle
    //           relies on continuous execution. If we skip frames (because telemetry
    //           is good), the 'prev_slip' state becomes stale. When telemetry eventually
    //           fails, the LPF will smooth against ancient history, causing a math spike.
    //
    // Reason 2 (Dependency): The 'Rear Aligning Torque' effect (calculated later)
    //           reads 'result.slip_angle'. If we only calculate this when grip is
    //           missing, the Rear Torque effect will toggle ON/OFF randomly based on
    //           telemetry health, causing violent kicks and "reverse FFB" sensations.
    // ==================================================================================
    double slip1 = calculate_slip_angle(w1, prev_slip1, dt);
    double slip2 = calculate_slip_angle(w2, prev_slip2, dt);
    result.slip_angle = (slip1 + slip2) / 2.0;

    // ==================================================================================
    // SHADOW MODE: Always calculate slope grip if enabled (for diagnostics and logging)
    // This allows us to compare our math against unencrypted cars to tune the algorithm.
    // ==================================================================================
    double slope_grip_estimate = 1.0;
    if (is_front && data && car_speed >= 5.0) {
        slope_grip_estimate = calculate_slope_grip(
            data->mLocalAccel.x / 9.81,
            result.slip_angle,
            dt,
            data
        );
    }

    // Fallback condition: Grip is essentially zero BUT car has significant load
    if (result.value < 0.0001 && avg_axle_load > 100.0) {
        result.approximated = true;

        if (car_speed < 5.0) {
            // Note: We still keep the calculated slip_angle in result.slip_angle
            // for visualization/rear torque, even if we force grip to 1.0 here.
            result.value = 1.0; 
        } else {
            if (m_slope_detection.enabled && is_front && data) {
                result.value = slope_grip_estimate;
            } else {
                // --- REFINED: Load-Sensitive Continuous Friction Circle ---

                auto calc_wheel_grip = [&](const TelemWheelV01& w, double slip_angle, double& prev_load) {
                    // 1. Dynamic Load Sensitivity with Slew/Smoothing
                    double raw_load = warned_flag ? approximate_load(w) : w.mTireLoad;

                    // Smooth the load to prevent curb strikes from causing threshold jitter (50ms tau)
                    double load_alpha = dt / (0.050 + dt);
                    prev_load += load_alpha * (raw_load - prev_load);
                    double current_load = prev_load;

                    // Calculate how loaded the tire is compared to the car's static weight
                    double static_ref = is_front ? m_static_front_load : m_static_rear_load;
                    double load_ratio = current_load / (static_ref + 1.0);
                    load_ratio = std::clamp(load_ratio, 0.25, 4.0); // Safety bounds

                    // Tire physics: Optimal slip angle increases with load (Hertzian cube root)
                    // Note: Future thermal/pressure multipliers would be applied here
                    double dynamic_slip_angle = m_grip_estimation.optimal_slip_angle;
                    if (m_grip_estimation.load_sensitivity_enabled) {
                        dynamic_slip_angle *= std::pow(load_ratio, 0.333);
                    }

                    // 2. Lateral Component
                    double lat_metric = std::abs(slip_angle) / dynamic_slip_angle;

                    // 3. Longitudinal Component
                    double long_ratio = calculate_manual_slip_ratio(w, car_speed);
                    double long_metric = std::abs(long_ratio) / (double)m_grip_estimation.optimal_slip_ratio;

                    // 4. Combined Vector (Friction Circle)
                    double combined_slip = std::sqrt((lat_metric * lat_metric) + (long_metric * long_metric));

                    // 5. Continuous Falloff Curve with Sliding Friction Asymptote
                    double cs2 = combined_slip * combined_slip;
                    double cs4 = cs2 * cs2;

                    const double MIN_SLIDING_GRIP = 0.05;
                    return MIN_SLIDING_GRIP + ((1.0 - MIN_SLIDING_GRIP) / (1.0 + cs4));
                };

                // Calculate grip for each wheel independently, passing the state reference
                double grip1 = calc_wheel_grip(w1, slip1, prev_load1);
                double grip2 = calc_wheel_grip(w2, slip2, prev_load2);

                // Average the resulting grip fractions
                result.value = (grip1 + grip2) / 2.0;
            }
        }

        // Clamp to standard 0.0 - 1.0 bounds
        result.value = std::clamp(result.value, 0.0, 1.0);

        if (!warned_flag) {
            Logger::Get().LogFile("Warning: Data for mGripFract from the game seems to be missing for this car (%s). (Likely Encrypted/DLC Content). A fallback estimation will be used.", vehicleName);
            warned_flag = true;
        }
    }    

    // Apply Adaptive Smoothing (v0.7.47)
    double& state = is_front ? m_front_grip_smoothed_state : m_rear_grip_smoothed_state;
    result.value = apply_adaptive_smoothing(result.value, state, dt,
                                            (double)m_grip_estimation.grip_smoothing_steady,
                                            (double)m_grip_estimation.grip_smoothing_fast,
                                            (double)m_grip_estimation.grip_smoothing_sensitivity);

    result.value = (std::max)(0.0, (std::min)(1.0, result.value));
    return result;
}

double FFBEngine::approximate_load(const TelemWheelV01& w) {
    return Physics::CalculateApproximateLoad(w, m_metadata.GetCurrentClass(), false);
}

double FFBEngine::approximate_rear_load(const TelemWheelV01& w) {
    return Physics::CalculateApproximateLoad(w, m_metadata.GetCurrentClass(), true);
}

double FFBEngine::calculate_manual_slip_ratio(const TelemWheelV01& w, double car_speed_ms) {
    return Physics::CalculateManualSlipRatio(w, car_speed_ms);
}

// Helper: Calculate Grip Factor from Slope - v0.7.40 REWRITE
// Robust projected slope estimation with hold-and-decay logic and torque-based anticipation.
double FFBEngine::calculate_slope_grip(double lateral_g, double slip_angle, double dt, const TelemInfoV01* data) {
    // 1. Signal Hygiene (Slew Limiter & Pre-Smoothing)

    // Initialize slew limiter and smoothing state on first sample to avoid ramp-up transients
    if (m_slope_buffer_count == 0) {
        m_slope_lat_g_prev = std::abs(lateral_g);
        m_slope_lat_g_smoothed = std::abs(lateral_g);
        m_slope_slip_smoothed = std::abs(slip_angle);
        if (data) {
            m_slope_torque_smoothed = std::abs(data->mSteeringShaftTorque);
            m_slope_steer_smoothed = std::abs(data->mUnfilteredSteering);
        }
    }

    // v0.7.198: We use the physical 'dt' for time-dependent filters (slew, LPF, decay)
    // to maintain correctness in tests and variable-rate scenarios.
    // However, we use a fixed 400Hz 'internal_dt' for Savitzky-Golay derivatives
    // because the internal buffers are populated at the 400Hz engine tick rate.
    const double internal_dt = PHYSICS_CALC_DT;

    double lat_g_slew = apply_slew_limiter(std::abs(lateral_g), m_slope_lat_g_prev, (double)m_slope_detection.g_slew_limit, dt);
    // v0.7.198 FIX: Must update m_slope_lat_g_prev immediately after computing lat_g_slew
    // so the slew limiter always receives its own output as the next call's starting point.
    // Previously this assignment was either absent or placed later in the function (after the
    // buffer update), which broke the slew limiter's feedback loop on the first call.
    m_slope_lat_g_prev = lat_g_slew;
    m_debug_lat_g_slew = lat_g_slew;

    double alpha_smooth = dt / (0.01 + dt);

    if (m_slope_buffer_count > 0) {
        m_slope_lat_g_smoothed += alpha_smooth * (lat_g_slew - m_slope_lat_g_smoothed);
        m_slope_slip_smoothed += alpha_smooth * (std::abs(slip_angle) - m_slope_slip_smoothed);
        if (data) {
            m_slope_torque_smoothed += alpha_smooth * (std::abs(data->mSteeringShaftTorque) - m_slope_torque_smoothed);
            m_slope_steer_smoothed += alpha_smooth * (std::abs(data->mUnfilteredSteering) - m_slope_steer_smoothed);
        }
    }

    // 2. Update Buffers with smoothed values
    m_slope_lat_g_buffer[m_slope_buffer_index] = m_slope_lat_g_smoothed;
    m_slope_slip_buffer[m_slope_buffer_index] = m_slope_slip_smoothed;
    if (data) {
        m_slope_torque_buffer[m_slope_buffer_index] = m_slope_torque_smoothed;
        m_slope_steer_buffer[m_slope_buffer_index] = m_slope_steer_smoothed;
    }

    m_slope_buffer_index = (m_slope_buffer_index + 1) % SLOPE_BUFFER_MAX;
    if (m_slope_buffer_count < SLOPE_BUFFER_MAX) m_slope_buffer_count++;

    // 3. Calculate G-based Derivatives (Savitzky-Golay)
    // v0.7.198: Always use fixed 400Hz dt for derivatives to ensure stable units (u/s).
    double dG_dt = calculate_sg_derivative(m_slope_lat_g_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);
    double dAlpha_dt = calculate_sg_derivative(m_slope_slip_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);

    m_slope_dG_dt = dG_dt;
    m_slope_dAlpha_dt = dAlpha_dt;

    // 4. Projected Slope Logic (G-based)
    // Update slope ONLY when steering is actively moving (abs(dAlpha_dt) > threshold).
    // This correctly implements "Hold and Decay" by preserving the peak during
    // steady states while allowing the signal to recover when the slide is caught.
    // v0.7.198: Added a check to only update G-Slope if the primary axle grip is
    // being approximated (encrypted DLC) OR if we are in Shadow Mode (always calc).
    // shadow_mode is true when m_slope_detection_enabled == false and we still reach
    // this function --- that can only happen when calculate_axle_grip calls us explicitly
    // to run the SG filter in "shadow" mode for diagnostics (Issue #348).
    // It is preserved here for readability and future guard logic; suppressing it would
    // require restructuring the call site instead.
    bool shadow_mode = (m_slope_detection.enabled == false);
    (void)shadow_mode; // Retained for documentation; see call site in calculate_axle_grip
    if (std::abs(dAlpha_dt) > (double)m_slope_detection.alpha_threshold) {
        m_slope_hold_timer = 0.25; // SLOPE_HOLD_TIME
        m_debug_slope_num = dG_dt * dAlpha_dt;
        m_debug_slope_den = (dAlpha_dt * dAlpha_dt) + 0.000001;
        m_debug_slope_raw = m_debug_slope_num / m_debug_slope_den;
        m_slope_current = std::clamp(m_debug_slope_raw, -20.0, 20.0);
    } else {
        // Decay phase: use physical 'dt' to ensure correct time advancement
        m_slope_hold_timer -= dt;
        if (m_slope_hold_timer <= 0.0) {
            m_slope_hold_timer = 0.0;
            m_slope_current += (double)m_slope_detection.decay_rate * dt * (0.0 - m_slope_current);
            if (std::abs(m_slope_current) < 0.0001) m_slope_current = 0.0;
        }
    }

    // 5. Calculate Torque-based Slope (Pneumatic Trail Anticipation)
    volatile bool can_calc_torque = (m_slope_detection.use_torque && data != nullptr);
    if (can_calc_torque) {
        double dTorque_dt = calculate_sg_derivative(m_slope_torque_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);
        double dSteer_dt = calculate_sg_derivative(m_slope_steer_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);

        if (std::abs(dSteer_dt) > (double)m_slope_detection.alpha_threshold) { // Unified threshold for steering movement
            m_debug_slope_torque_num = dTorque_dt * dSteer_dt;
            m_debug_slope_torque_den = (dSteer_dt * dSteer_dt) + 0.000001;
            m_slope_torque_current = std::clamp(m_debug_slope_torque_num / m_debug_slope_torque_den, -50.0, 50.0);
        } else {
            m_slope_torque_current += (double)m_slope_detection.decay_rate * dt * (0.0 - m_slope_torque_current);
        }
    } else {
        m_slope_torque_current = 20.0; // Positive value means no grip loss detected
    }

    double current_grip_factor = 1.0;
    double confidence = calculate_slope_confidence(dAlpha_dt);

    // 1. Calculate Grip Loss from G-Slope (Lateral Saturation)
    double loss_percent_g = inverse_lerp((double)m_slope_detection.min_threshold, (double)m_slope_detection.max_threshold, m_slope_current);

    // 2. Calculate Grip Loss from Torque-Slope (Pneumatic Trail Drop)
    double loss_percent_torque = 0.0;
    volatile bool use_torque_fusion = (m_slope_detection.use_torque && data != nullptr);
    if (use_torque_fusion) {
        if (m_slope_torque_current < 0.0) {
            loss_percent_torque = std::abs(m_slope_torque_current) * (double)m_slope_detection.torque_sensitivity;
            loss_percent_torque = (std::max)(0.0, (std::min)(1.0, loss_percent_torque));
        }
    }

    // 3. Fusion Logic (Max of both estimators)
    double loss_percent = (std::max)(loss_percent_g, loss_percent_torque);

    // Scale loss by confidence and apply floor (0.2)
    // 0% loss (loss_percent=0) -> 1.0 factor
    // 100% loss (loss_percent=1) -> 0.2 factor
    current_grip_factor = 1.0 - (loss_percent * 0.8 * confidence);

    // Apply Floor (Safety)
    current_grip_factor = (std::max)(0.2, (std::min)(1.0, current_grip_factor));

    // 5. Smoothing (v0.7.0)
    double alpha = dt / ((double)m_slope_detection.smoothing_tau + dt);
    alpha = (std::max)(0.001, (std::min)(1.0, alpha));
    m_slope_smoothed_output += alpha * (current_grip_factor - m_slope_smoothed_output);

    return m_slope_smoothed_output;
}

// Helper: Calculate confidence factor for slope detection
// Extracted to avoid code duplication between slope detection and logging
double FFBEngine::calculate_slope_confidence(double dAlpha_dt) {
    if (!m_slope_detection.confidence_enabled) return 1.0;

    // v0.7.21 FIX: Use smoothstep confidence ramp [m_slope_alpha_threshold, m_slope_confidence_max_rate] rad/s
    // to reject singularity artifacts near zero.
    return smoothstep((double)m_slope_detection.alpha_threshold, (double)m_slope_detection.confidence_max_rate, std::abs(dAlpha_dt));
}

double FFBEngine::calculate_wheel_slip_ratio(const TelemWheelV01& w) {
    return Physics::CalculateWheelSlipRatio(w);
}

} // namespace LMUFFB
