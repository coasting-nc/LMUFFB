// GripLoadEstimation.cpp
// ---------------------------------------------------------------------------
// Grip and Load Estimation methods for FFBEngine.
//
// This file is a source-level split of FFBEngine.cpp (Approach A).
// All functions here are FFBEngine member methods; FFBEngine.h is unchanged.
//
// Rationale: These functions form a self-contained "data preparation" layer â€”
// they take raw (possibly broken/encrypted) telemetry and produce the best
// available grip and load values. The FFB engine then consumes those values
// without needing to know how they were estimated.
//
// See docs/dev_docs/reports/FFBEngine_refactoring_analysis.md for full details.
// ---------------------------------------------------------------------------

#include "FFBEngine.h"
#include "Config.h"
#include <iostream>
#include <mutex>

extern std::recursive_mutex g_engine_mutex;
#include <cmath>

using namespace ffb_math;

// Helper: Learn static front load reference (v0.7.46)
void FFBEngine::update_static_load_reference(double current_load, double speed, double dt) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    if (m_static_load_latched) return; // Do not update if latched

    if (speed > 2.0 && speed < 15.0) {
        if (m_static_front_load < 100.0) {
             m_static_front_load = current_load;
        } else {
             m_static_front_load += (dt / 5.0) * (current_load - m_static_front_load);
        }
    } else if (speed >= 15.0 && m_static_front_load > 1000.0) {
        // Latch the value once we exceed 15 m/s (aero begins to take over)
        m_static_load_latched = true;

        // Save to config map (v0.7.70)
        std::string vName = m_vehicle_name;
        if (vName != "Unknown" && vName != "") {
            Config::SetSavedStaticLoad(vName, m_static_front_load);
            Config::m_needs_save = true; // Flag main thread to write to disk
            std::cout << "[FFB] Latched and saved static load for " << vName << ": " << m_static_front_load << "N" << std::endl;
        }
    }

    // Safety fallback: Ensure we have a sane baseline if learning failed
    if (m_static_front_load < 1000.0) {
        m_static_front_load = m_auto_peak_load * 0.5;
    }
}

// Initialize the load reference based on vehicle class and name seeding
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    ParsedVehicleClass vclass = ParseVehicleClass(className, vehicleName);
    m_auto_peak_load = GetDefaultLoadForClass(vclass);

    std::string vName = vehicleName ? vehicleName : "Unknown";

    // Check if we already have a saved static load for this specific car (v0.7.70)
    double saved_load = 0.0;
    if (Config::GetSavedStaticLoad(vName, saved_load)) {
        m_static_front_load = saved_load;
        m_static_load_latched = true; // Skip the 2-15 m/s learning phase
        std::cout << "[FFB] Loaded persistent static load for " << vName << ": " << m_static_front_load << "N" << std::endl;
    } else {
        // Reset static load reference for new car class
        m_static_front_load = m_auto_peak_load * 0.5;
        m_static_load_latched = false;
        std::cout << "[FFB] No saved load for " << vName << ". Learning required." << std::endl;
    }

    m_smoothed_tactile_mult = 1.0;

    std::cout << "[FFB] Vehicle Identification -> Detected Class: " << VehicleClassToString(vclass)
              << " | Seed Load: " << m_auto_peak_load << "N" 
              << " (Raw -> Class: " << (className ? className : "Unknown") 
              << ", Name: " << vName << ")" << std::endl;
}

// Helper: Calculate Raw Slip Angle for a pair of wheels (v0.4.9 Refactor)
// Returns the average slip angle of two wheels using atan2(lateral_vel, longitudinal_vel)
// v0.4.19: Removed abs() from lateral velocity to preserve sign for debug visualization
double FFBEngine::calculate_raw_slip_angle_pair(const TelemWheelV01& w1, const TelemWheelV01& w2) {
    double v_long_1 = std::abs(w1.mLongitudinalGroundVel);
    double v_long_2 = std::abs(w2.mLongitudinalGroundVel);
    if (v_long_1 < MIN_SLIP_ANGLE_VELOCITY) v_long_1 = MIN_SLIP_ANGLE_VELOCITY;
    if (v_long_2 < MIN_SLIP_ANGLE_VELOCITY) v_long_2 = MIN_SLIP_ANGLE_VELOCITY;
    double raw_angle_1 = std::atan2(w1.mLateralPatchVel, v_long_1);
    double raw_angle_2 = std::atan2(w2.mLateralPatchVel, v_long_2);
    return (raw_angle_1 + raw_angle_2) / 2.0;
}

double FFBEngine::calculate_slip_angle(const TelemWheelV01& w, double& prev_state, double dt) {
    double v_long = std::abs(w.mLongitudinalGroundVel);
    if (v_long < MIN_SLIP_ANGLE_VELOCITY) v_long = MIN_SLIP_ANGLE_VELOCITY;
    
    // v0.4.19: PRESERVE SIGN - Do NOT use abs() on lateral velocity
    // Positive lateral vel (+X = left) â†’ Positive slip angle
    // Negative lateral vel (-X = right) â†’ Negative slip angle
    // This sign is critical for directional counter-steering
    double raw_angle = std::atan2(w.mLateralPatchVel, v_long);  // SIGN PRESERVED
    
    // LPF: Time Corrected Alpha (v0.4.37)
    // Target: Alpha 0.1 at 400Hz (dt = 0.0025)
    // Formula: alpha = dt / (tau + dt) -> 0.1 = 0.0025 / (tau + 0.0025) -> tau approx 0.0225s
    // v0.4.40: Using configurable m_slip_angle_smoothing
    double tau = (double)m_slip_angle_smoothing;
    if (tau < 0.0001) tau = 0.0001; // Safety clamp 
    
    double alpha = dt / (tau + dt);
    
    // Safety clamp
    alpha = (std::min)(1.0, (std::max)(0.001, alpha));
    prev_state = prev_state + alpha * (raw_angle - prev_state);
    return prev_state;
}

// Helper: Calculate Grip with Fallback (v0.4.6 Hardening)
GripResult FFBEngine::calculate_grip(const TelemWheelV01& w1, 
                          const TelemWheelV01& w2,
                          double avg_load,
                          bool& warned_flag,
                          double& prev_slip1,
                          double& prev_slip2,
                          double car_speed,
                          double dt,
                          const char* vehicleName,
                          const TelemInfoV01* data,
                          bool is_front) {
    GripResult result;
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

    // Fallback condition: Grip is essentially zero BUT car has significant load
    if (result.value < 0.0001 && avg_load > 100.0) {
        result.approximated = true;
        
        if (car_speed < 5.0) {
            // Note: We still keep the calculated slip_angle in result.slip_angle
            // for visualization/rear torque, even if we force grip to 1.0 here.
            result.value = 1.0; 
        } else {
            if (m_slope_detection_enabled && is_front && data) {
                // Dynamic grip estimation via derivative monitoring
                result.value = calculate_slope_grip(
                    data->mLocalAccel.x / 9.81,
                    result.slip_angle,
                    dt,
                    data
                );
            } else {
                // v0.4.38: Combined Friction Circle (Advanced Reconstruction)
                
                // 1. Lateral Component (Alpha)
                // USE CONFIGURABLE THRESHOLD (v0.5.7)
                double lat_metric = std::abs(result.slip_angle) / (double)m_optimal_slip_angle;

                // 2. Longitudinal Component (Kappa)
                // Calculate manual slip for both wheels and average the magnitude
                double ratio1 = calculate_manual_slip_ratio(w1, car_speed);
                double ratio2 = calculate_manual_slip_ratio(w2, car_speed);
                double avg_ratio = (std::abs(ratio1) + std::abs(ratio2)) / 2.0;

                // USE CONFIGURABLE THRESHOLD (v0.5.7)
                double long_metric = avg_ratio / (double)m_optimal_slip_ratio;

                // 3. Combined Vector (Friction Circle)
                double combined_slip = std::sqrt((lat_metric * lat_metric) + (long_metric * long_metric));

                // 4. Map to Grip Fraction

                if (combined_slip > 1.0) {
                    double excess = combined_slip - 1.0;
                    result.value = 1.0 / (1.0 + excess * 2.0);
                } else {
                    result.value = 1.0;
                }
            }
        }
        
        
        // Safety Clamp (v0.4.6): Never drop below 0.2 in approximation
        result.value = (std::max)(0.2, result.value);
        
        if (!warned_flag) {
            std::cout << "Warning: Data for mGripFract from the game seems to be missing for this car (" << vehicleName << "). (Likely Encrypted/DLC Content). A fallback estimation will be used." << std::endl;
            warned_flag = true;
        }
    }
    
    // Apply Adaptive Smoothing (v0.7.47)
    double& state = is_front ? m_front_grip_smoothed_state : m_rear_grip_smoothed_state;
    result.value = apply_adaptive_smoothing(result.value, state, dt,
                                            (double)m_grip_smoothing_steady,
                                            (double)m_grip_smoothing_fast,
                                            (double)m_grip_smoothing_sensitivity);

    result.value = (std::max)(0.0, (std::min)(1.0, result.value));
    return result;
}

// Helper: Approximate Load (v0.4.5)
double FFBEngine::approximate_load(const TelemWheelV01& w) {
    // Base: Suspension Force + Est. Unsprung Mass (300N)
    // Note: mSuspForce captures weight transfer and aero
    return w.mSuspForce + 300.0;
}

// Helper: Approximate Rear Load (v0.4.10)
double FFBEngine::approximate_rear_load(const TelemWheelV01& w) {
    // Base: Suspension Force + Est. Unsprung Mass (300N)
    // This captures weight transfer (braking/accel) and aero downforce implicitly via suspension compression
    return w.mSuspForce + 300.0;
}

// Helper: Calculate Kinematic Load (v0.4.39)
// Estimates tire load from chassis physics when telemetry (mSuspForce) is missing.
// This is critical for encrypted DLC content where suspension sensors are blocked.
double FFBEngine::calculate_kinematic_load(const TelemInfoV01* data, int wheel_index) {
    // 1. Static Weight Distribution
    bool is_rear = (wheel_index >= 2);
    double bias = is_rear ? m_approx_weight_bias : (1.0 - m_approx_weight_bias);
    double static_weight = (m_approx_mass_kg * 9.81 * bias) / 2.0;

    // 2. Aerodynamic Load (Velocity Squared)
    double speed = std::abs(data->mLocalVel.z);
    double aero_load = m_approx_aero_coeff * (speed * speed);
    double wheel_aero = aero_load / 4.0; 

    // 3. Longitudinal Weight Transfer (Braking/Acceleration)
    // COORDINATE SYSTEM VERIFIED (v0.4.39):
    // - LMU: +Z axis points REARWARD (out the back of the car)
    // - Braking: Chassis decelerates â†’ Inertial force pushes rearward â†’ +Z acceleration
    // - Result: Front wheels GAIN load, Rear wheels LOSE load
    // - Source: docs/dev_docs/references/Reference - coordinate_system_reference.md
    // 
    // Formula: (Accel / g) * WEIGHT_TRANSFER_SCALE
    // We use SMOOTHED acceleration to simulate chassis pitch inertia (~35ms lag)
    double long_transfer = (m_accel_z_smoothed / 9.81) * WEIGHT_TRANSFER_SCALE; 
    if (is_rear) long_transfer *= -1.0; // Subtract from Rear during Braking

    // 4. Lateral Weight Transfer (Cornering)
    // COORDINATE SYSTEM VERIFIED (v0.4.39):
    // - LMU: +X axis points LEFT (out the left side of the car)
    // - Right Turn: Centrifugal force pushes LEFT â†’ +X acceleration
    // - Result: LEFT wheels (outside) GAIN load, RIGHT wheels (inside) LOSE load
    // - Source: docs/dev_docs/references/Reference - coordinate_system_reference.md
    // 
    // Formula: (Accel / g) * WEIGHT_TRANSFER_SCALE * Roll_Stiffness
    // We use SMOOTHED acceleration to simulate chassis roll inertia (~35ms lag)
    double lat_transfer = (m_accel_x_smoothed / 9.81) * WEIGHT_TRANSFER_SCALE * m_approx_roll_stiffness;
    bool is_left = (wheel_index == 0 || wheel_index == 2);
    if (!is_left) lat_transfer *= -1.0; // Subtract from Right wheels

    // Sum and Clamp
    double total_load = static_weight + wheel_aero + long_transfer + lat_transfer;
    return (std::max)(0.0, total_load);
}

// Helper: Calculate Manual Slip Ratio (v0.4.6)
double FFBEngine::calculate_manual_slip_ratio(const TelemWheelV01& w, double car_speed_ms) {
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

    double lat_g_slew = apply_slew_limiter(std::abs(lateral_g), m_slope_lat_g_prev, (double)m_slope_g_slew_limit, dt);
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
    double dG_dt = calculate_sg_derivative(m_slope_lat_g_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);
    double dAlpha_dt = calculate_sg_derivative(m_slope_slip_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);

    m_slope_dG_dt = dG_dt;
    m_slope_dAlpha_dt = dAlpha_dt;

    // 4. Projected Slope Logic (G-based)
    if (std::abs(dAlpha_dt) > (double)m_slope_alpha_threshold) {
        m_slope_hold_timer = SLOPE_HOLD_TIME;
        m_debug_slope_num = dG_dt * dAlpha_dt;
        m_debug_slope_den = (dAlpha_dt * dAlpha_dt) + 0.000001;
        m_debug_slope_raw = m_debug_slope_num / m_debug_slope_den;
        m_slope_current = std::clamp(m_debug_slope_raw, -20.0, 20.0);
    } else {
        m_slope_hold_timer -= dt;
        if (m_slope_hold_timer <= 0.0) {
            m_slope_hold_timer = 0.0;
            m_slope_current += (double)m_slope_decay_rate * dt * (0.0 - m_slope_current);
        }
    }

    // 5. Calculate Torque-based Slope (Pneumatic Trail Anticipation)
    volatile bool can_calc_torque = (m_slope_use_torque && data != nullptr);
    if (can_calc_torque) {
        double dTorque_dt = calculate_sg_derivative(m_slope_torque_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);
        double dSteer_dt = calculate_sg_derivative(m_slope_steer_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);

        if (std::abs(dSteer_dt) > (double)m_slope_alpha_threshold) { // Unified threshold for steering movement
            m_debug_slope_torque_num = dTorque_dt * dSteer_dt;
            m_debug_slope_torque_den = (dSteer_dt * dSteer_dt) + 0.000001;
            m_slope_torque_current = std::clamp(m_debug_slope_torque_num / m_debug_slope_torque_den, -50.0, 50.0);
        } else {
            m_slope_torque_current += (double)m_slope_decay_rate * dt * (0.0 - m_slope_torque_current);
        }
    } else {
        m_slope_torque_current = 20.0; // Positive value means no grip loss detected
    }

    double current_grip_factor = 1.0;
    double confidence = calculate_slope_confidence(dAlpha_dt);

    // 1. Calculate Grip Loss from G-Slope (Lateral Saturation)
    double loss_percent_g = inverse_lerp((double)m_slope_min_threshold, (double)m_slope_max_threshold, m_slope_current);

    // 2. Calculate Grip Loss from Torque-Slope (Pneumatic Trail Drop)
    double loss_percent_torque = 0.0;
    volatile bool use_torque_fusion = (m_slope_use_torque && data != nullptr);
    if (use_torque_fusion) {
        if (m_slope_torque_current < 0.0) {
            loss_percent_torque = std::abs(m_slope_torque_current) * (double)m_slope_torque_sensitivity;
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
    double alpha = dt / ((double)m_slope_smoothing_tau + dt);
    alpha = (std::max)(0.001, (std::min)(1.0, alpha));
    m_slope_smoothed_output += alpha * (current_grip_factor - m_slope_smoothed_output);

    return m_slope_smoothed_output;
}

// Helper: Calculate confidence factor for slope detection
// Extracted to avoid code duplication between slope detection and logging
double FFBEngine::calculate_slope_confidence(double dAlpha_dt) {
    if (!m_slope_confidence_enabled) return 1.0;

    // v0.7.21 FIX: Use smoothstep confidence ramp [m_slope_alpha_threshold, m_slope_confidence_max_rate] rad/s
    // to reject singularity artifacts near zero.
    return smoothstep((double)m_slope_alpha_threshold, (double)m_slope_confidence_max_rate, std::abs(dAlpha_dt));
}

// Helper: Calculate Slip Ratio from wheel (v0.6.36 - Extracted from lambdas)
// Unified slip ratio calculation for lockup and spin detection.
// Returns the ratio of longitudinal slip: (PatchVel - GroundVel) / GroundVel
double FFBEngine::calculate_wheel_slip_ratio(const TelemWheelV01& w) {
    double v_long = std::abs(w.mLongitudinalGroundVel);
    if (v_long < MIN_SLIP_ANGLE_VELOCITY) v_long = MIN_SLIP_ANGLE_VELOCITY;
    return w.mLongitudinalPatchVel / v_long;
}
