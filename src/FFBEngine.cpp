#include "FFBEngine.h"
#include "Config.h"
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace ffb_math;

FFBEngine::FFBEngine() {
    last_log_time = std::chrono::steady_clock::now();
    Preset::ApplyDefaultsToEngine(*this);
}

// v0.7.34: Safety Check for Issue #79
// Determines if FFB should be active based on vehicle scoring state.
// v0.7.49: Modified for #126 to allow cool-down laps after finish.
bool FFBEngine::IsFFBAllowed(const VehicleScoringInfoV01& scoring, unsigned char gamePhase) const {
    // mControl: 0 = local player, 1 = AI, 2 = Remote, -1 = None
    // mFinishStatus: 0 = none, 1 = finished, 2 = DNF, 3 = DQ

    // 1. Core control check
    if (!scoring.mIsPlayer || scoring.mControl != 0) return false;

    // 2. DO NOT use gamePhase to determine if IsFFBAllowed (eg. session over would cause no FFB after finish line, or time up)
    // (gamePhase, Game Phases: 7=Stopped, 8=Session Over)

    // 3. Individual status safety
    // Allow Finished (1) and DNF (2) as long as player is in control.
    // Mute for Disqualified (3).
    if (scoring.mFinishStatus == 3) return false;

    return true;
}

// v0.7.49: Slew rate limiter for safety (#79)
// Clamps the rate of change of the output force to prevent violent jolts.
// If restricted is true (e.g. after finish or lost control), limit is tighter.
double FFBEngine::ApplySafetySlew(double target_force, double dt, bool restricted) {
    if (!std::isfinite(target_force)) return 0.0;
    double max_slew = restricted ? 100.0 : 1000.0;
    double max_change = max_slew * dt;
    double delta = target_force - m_last_output_force;
    delta = std::clamp(delta, -max_change, max_change);
    m_last_output_force += delta;
    return m_last_output_force;
}

// Helper to retrieve data (Consumer)
std::vector<FFBSnapshot> FFBEngine::GetDebugBatch() {
    std::vector<FFBSnapshot> batch;
    {
        std::lock_guard<std::mutex> lock(m_debug_mutex);
        if (!m_debug_buffer.empty()) {
            batch.swap(m_debug_buffer);
        }
    }
    return batch;
}

// Helper: Learn static front load reference (v0.7.46)
void FFBEngine::update_static_load_reference(double current_load, double speed, double dt) {
    if (speed > 2.0 && speed < 15.0) {
        if (m_static_front_load < 100.0) {
             m_static_front_load = current_load;
        } else {
             m_static_front_load += (dt / 5.0) * (current_load - m_static_front_load);
        }
    }
    if (m_static_front_load < 1000.0) m_static_front_load = m_auto_peak_load * 0.5;
}

// Initialize the load reference based on vehicle class and name seeding
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    ParsedVehicleClass vclass = ParseVehicleClass(className, vehicleName);
    m_auto_peak_load = GetDefaultLoadForClass(vclass);

    // Reset static load reference for new car class
    m_static_front_load = m_auto_peak_load * 0.5;

    std::cout << "[FFB] Vehicle Identification -> Detected Class: " << VehicleClassToString(vclass)
              << " | Seed Load: " << m_auto_peak_load << "N" 
              << " (Raw -> Class: " << (className ? className : "Unknown") 
              << ", Name: " << (vehicleName ? vehicleName : "Unknown") << ")" << std::endl;
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
    // Positive lateral vel (+X = left) Æ Positive slip angle
    // Negative lateral vel (-X = right) Æ Negative slip angle
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
    // - Braking: Chassis decelerates ÔåÆ Inertial force pushes rearward ÔåÆ +Z acceleration
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
    // - Right Turn: Centrifugal force pushes LEFT ÔåÆ +X acceleration
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

// Signal Conditioning: Applies idle smoothing and notch filters to raw torque
// Returns the conditioned force value ready for effect processing
double FFBEngine::apply_signal_conditioning(double raw_torque, const TelemInfoV01* data, FFBCalculationContext& ctx) {
    double game_force_proc = raw_torque;

    // Idle Smoothing
    double effective_shaft_smoothing = (double)m_steering_shaft_smoothing;
    double idle_speed_threshold = (double)m_speed_gate_upper;
    if (idle_speed_threshold < 3.0) idle_speed_threshold = 3.0;
    if (ctx.car_speed < idle_speed_threshold) {
        double idle_blend = (idle_speed_threshold - ctx.car_speed) / idle_speed_threshold;
        double dynamic_smooth = 0.1 * idle_blend; 
        effective_shaft_smoothing = (std::max)(effective_shaft_smoothing, dynamic_smooth);
    }
    
    if (effective_shaft_smoothing > 0.0001) {
        double alpha_shaft = ctx.dt / (effective_shaft_smoothing + ctx.dt);
        alpha_shaft = (std::min)(1.0, (std::max)(0.001, alpha_shaft));
        m_steering_shaft_torque_smoothed += alpha_shaft * (game_force_proc - m_steering_shaft_torque_smoothed);
        game_force_proc = m_steering_shaft_torque_smoothed;
    } else {
        m_steering_shaft_torque_smoothed = game_force_proc;
    }

    // Frequency Estimator Logic
    double alpha_hpf = ctx.dt / (0.1 + ctx.dt);
    m_torque_ac_smoothed += alpha_hpf * (game_force_proc - m_torque_ac_smoothed);
    double ac_torque = game_force_proc - m_torque_ac_smoothed;

    if ((m_prev_ac_torque < -0.05 && ac_torque > 0.05) ||
        (m_prev_ac_torque > 0.05 && ac_torque < -0.05)) {

        double now = data->mElapsedTime;
        double period = now - m_last_crossing_time;

        if (period > 0.005 && period < 1.0) {
            double inst_freq = 1.0 / (period * 2.0);
            m_debug_freq = m_debug_freq * 0.9 + inst_freq * 0.1;
        }
        m_last_crossing_time = now;
    }
    m_prev_ac_torque = ac_torque;

    const TelemWheelV01& fl_ref = data->mWheel[0];
    double radius = (double)fl_ref.mStaticUndeflectedRadius / 100.0;
    if (radius < 0.1) radius = 0.33;
    double circumference = 2.0 * PI * radius;
    double wheel_freq = (circumference > 0.0) ? (ctx.car_speed / circumference) : 0.0;
    m_theoretical_freq = wheel_freq;
    
    // Dynamic Notch Filter
    if (m_flatspot_suppression) {
        if (wheel_freq > 1.0) {
            m_notch_filter.Update(wheel_freq, 1.0/ctx.dt, (double)m_notch_q);
            double input_force = game_force_proc;
            double filtered_force = m_notch_filter.Process(input_force);
            game_force_proc = input_force * (1.0f - m_flatspot_strength) + filtered_force * m_flatspot_strength;
        } else {
            m_notch_filter.Reset();
        }
    }
    
    // Static Notch Filter
    if (m_static_notch_enabled) {
         double bw = (double)m_static_notch_width;
         if (bw < 0.1) bw = 0.1;
         double q = (double)m_static_notch_freq / bw;
         m_static_notch_filter.Update((double)m_static_notch_freq, 1.0/ctx.dt, q);
         game_force_proc = m_static_notch_filter.Process(game_force_proc);
    } else {
         m_static_notch_filter.Reset();
    }

    return game_force_proc;
}

// Refactored calculate_force
double FFBEngine::calculate_force(const TelemInfoV01* data, const char* vehicleClass, const char* vehicleName, float genFFBTorque) {
    if (!data) return 0.0;

    // Select Torque Source
    double raw_torque_input = (m_torque_source == 1) ? (double)genFFBTorque : data->mSteeringShaftTorque;

    // RELIABILITY FIX: Sanitize input torque
    if (!std::isfinite(raw_torque_input)) return 0.0;

    // Class Seeding
    bool seeded = false;
    if (vehicleClass && m_current_class_name != vehicleClass) {
        m_current_class_name = vehicleClass;
        InitializeLoadReference(vehicleClass, vehicleName);
        seeded = true;
    }
    
    // --- 1. INITIALIZE CONTEXT ---
    FFBCalculationContext ctx;
    ctx.dt = data->mDeltaTime;

    // Sanity Check: Delta Time
    if (ctx.dt <= 0.000001) {
        ctx.dt = 0.0025; // Default to 400Hz
        if (!m_warned_dt) {
            std::cout << "[WARNING] Invalid DeltaTime (<=0). Using default 0.0025s." << std::endl;
            m_warned_dt = true;
        }
        ctx.frame_warn_dt = true;
    }
    
    ctx.car_speed_long = data->mLocalVel.z;
    ctx.car_speed = std::abs(ctx.car_speed_long);
    
    // Update Context strings (for UI/Logging)
    // Only update if first char differs to avoid redundant copies
    if (m_vehicle_name[0] != data->mVehicleName[0] || m_vehicle_name[10] != data->mVehicleName[10]) {
#ifdef _WIN32
         strncpy_s(m_vehicle_name, data->mVehicleName, 63);
         m_vehicle_name[63] = '\0';
         strncpy_s(m_track_name, data->mTrackName, 63);
         m_track_name[63] = '\0';
#else
         strncpy(m_vehicle_name, data->mVehicleName, 63);
         m_vehicle_name[63] = '\0';
         strncpy(m_track_name, data->mTrackName, 63);
         m_track_name[63] = '\0';
#endif
    }

    // --- 2. SIGNAL CONDITIONING (STATE UPDATES) ---
    
    // Chassis Inertia Simulation
    double chassis_tau = (double)m_chassis_inertia_smoothing;
    if (chassis_tau < 0.0001) chassis_tau = 0.0001;
    double alpha_chassis = ctx.dt / (chassis_tau + ctx.dt);
    m_accel_x_smoothed += alpha_chassis * (data->mLocalAccel.x - m_accel_x_smoothed);
    m_accel_z_smoothed += alpha_chassis * (data->mLocalAccel.z - m_accel_z_smoothed);

    // --- 3. TELEMETRY PROCESSING ---
    // Front Wheels
    const TelemWheelV01& fl = data->mWheel[0];
    const TelemWheelV01& fr = data->mWheel[1];

    // Raw Inputs
    double raw_torque = raw_torque_input;
    double raw_load = (fl.mTireLoad + fr.mTireLoad) / 2.0;
    double raw_grip = (fl.mGripFract + fr.mGripFract) / 2.0;

    // Update Stats
    s_torque.Update(raw_torque);
    s_load.Update(raw_load);
    s_grip.Update(raw_grip);
    s_lat_g.Update(data->mLocalAccel.x);
    
    // Stats Latching
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1) {
        s_torque.ResetInterval(); 
        s_load.ResetInterval(); 
        s_grip.ResetInterval(); 
        s_lat_g.ResetInterval();
        last_log_time = now;
    }

    // --- 4. PRE-CALCULATIONS ---

    // Average Load & Fallback Logic
    ctx.avg_load = raw_load;

    // Hysteresis for missing load
    if (ctx.avg_load < 1.0 && ctx.car_speed > 1.0) {
        m_missing_load_frames++;
    } else {
        m_missing_load_frames = (std::max)(0, m_missing_load_frames - 1);
    }

    if (m_missing_load_frames > 20) {
        // Fallback Logic
        if (fl.mSuspForce > MIN_VALID_SUSP_FORCE) {
            double calc_load_fl = approximate_load(fl);
            double calc_load_fr = approximate_load(fr);
            ctx.avg_load = (calc_load_fl + calc_load_fr) / 2.0;
        } else {
            double kin_load_fl = calculate_kinematic_load(data, 0);
            double kin_load_fr = calculate_kinematic_load(data, 1);
            ctx.avg_load = (kin_load_fl + kin_load_fr) / 2.0;
        }
        if (!m_warned_load) {
            std::cout << "Warning: Data for mTireLoad from the game seems to be missing for this car (" << data->mVehicleName << "). (Likely Encrypted/DLC Content). Using Kinematic Fallback." << std::endl;
            m_warned_load = true;
        }
        ctx.frame_warn_load = true;
    }

    // Sanity Checks (Missing Data)
    
    // 1. Suspension Force (mSuspForce)
    double avg_susp_f = (fl.mSuspForce + fr.mSuspForce) / 2.0;
    if (avg_susp_f < MIN_VALID_SUSP_FORCE && std::abs(data->mLocalVel.z) > 1.0) {
        m_missing_susp_force_frames++;
    } else {
         m_missing_susp_force_frames = (std::max)(0, m_missing_susp_force_frames - 1);
    }
    if (m_missing_susp_force_frames > 50 && !m_warned_susp_force) {
         std::cout << "Warning: Data for mSuspForce from the game seems to be missing for this car (" << data->mVehicleName << "). (Likely Encrypted/DLC Content). A fallback estimation will be used." << std::endl;
         m_warned_susp_force = true;
    }

    // 2. Suspension Deflection (mSuspensionDeflection)
    double avg_susp_def = (std::abs(fl.mSuspensionDeflection) + std::abs(fr.mSuspensionDeflection)) / 2.0;
    if (avg_susp_def < 0.000001 && std::abs(data->mLocalVel.z) > 10.0) {
        m_missing_susp_deflection_frames++;
    } else {
        m_missing_susp_deflection_frames = (std::max)(0, m_missing_susp_deflection_frames - 1);
    }
    if (m_missing_susp_deflection_frames > 50 && !m_warned_susp_deflection) {
        std::cout << "Warning: Data for mSuspensionDeflection from the game seems to be missing for this car (" << data->mVehicleName << "). (Likely Encrypted/DLC Content). A fallback estimation will be used." << std::endl;
        m_warned_susp_deflection = true;
    }

    // 3. Front Lateral Force (mLateralForce)
    double avg_lat_force_front = (std::abs(fl.mLateralForce) + std::abs(fr.mLateralForce)) / 2.0;
    if (avg_lat_force_front < 1.0 && std::abs(data->mLocalAccel.x) > 3.0) {
        m_missing_lat_force_front_frames++;
    } else {
        m_missing_lat_force_front_frames = (std::max)(0, m_missing_lat_force_front_frames - 1);
    }
    if (m_missing_lat_force_front_frames > 50 && !m_warned_lat_force_front) {
         std::cout << "Warning: Data for mLateralForce (Front) from the game seems to be missing for this car (" << data->mVehicleName << "). (Likely Encrypted/DLC Content). A fallback estimation will be used." << std::endl;
         m_warned_lat_force_front = true;
    }

    // 4. Rear Lateral Force (mLateralForce)
    double avg_lat_force_rear = (std::abs(data->mWheel[2].mLateralForce) + std::abs(data->mWheel[3].mLateralForce)) / 2.0;
    if (avg_lat_force_rear < 1.0 && std::abs(data->mLocalAccel.x) > 3.0) {
        m_missing_lat_force_rear_frames++;
    } else {
        m_missing_lat_force_rear_frames = (std::max)(0, m_missing_lat_force_rear_frames - 1);
    }
    if (m_missing_lat_force_rear_frames > 50 && !m_warned_lat_force_rear) {
         std::cout << "Warning: Data for mLateralForce (Rear) from the game seems to be missing for this car (" << data->mVehicleName << "). (Likely Encrypted/DLC Content). A fallback estimation will be used." << std::endl;
         m_warned_lat_force_rear = true;
    }

    // 5. Vertical Tire Deflection (mVerticalTireDeflection)
    double avg_vert_def = (std::abs(fl.mVerticalTireDeflection) + std::abs(fr.mVerticalTireDeflection)) / 2.0;
    if (avg_vert_def < 0.000001 && std::abs(data->mLocalVel.z) > 10.0) {
        m_missing_vert_deflection_frames++;
    } else {
        m_missing_vert_deflection_frames = (std::max)(0, m_missing_vert_deflection_frames - 1);
    }
    if (m_missing_vert_deflection_frames > 50 && !m_warned_vert_deflection) {
        std::cout << "[WARNING] mVerticalTireDeflection is missing for car: " << data->mVehicleName 
                  << ". (Likely Encrypted/DLC Content). Road Texture fallback active." << std::endl;
        m_warned_vert_deflection = true;
    }
    
    // Peak Hold Logic
    if (m_auto_load_normalization_enabled && !seeded) {
        if (ctx.avg_load > m_auto_peak_load) {
            m_auto_peak_load = ctx.avg_load; // Fast Attack
        } else {
            m_auto_peak_load -= (100.0 * ctx.dt); // Slow Decay (~100N/s)
        }
    }
    m_auto_peak_load = (std::max)(3000.0, m_auto_peak_load); // Safety Floor

    // Load Factors
    double raw_load_factor = ctx.avg_load / m_auto_peak_load;
    // v0.7.48: Increased texture load cap from 2.0 to 10.0 to match brake load cap
    double texture_safe_max = (std::min)(10.0, (double)m_texture_load_cap);
    ctx.texture_load_factor = (std::min)(texture_safe_max, (std::max)(0.0, raw_load_factor));

    double brake_safe_max = (std::min)(10.0, (double)m_brake_load_cap);
    ctx.brake_load_factor = (std::min)(brake_safe_max, (std::max)(0.0, raw_load_factor));
    
    // Decoupling Scale
    double max_torque_safe = (double)m_max_torque_ref;
    if (max_torque_safe < 1.0) max_torque_safe = 1.0;
    ctx.decoupling_scale = max_torque_safe / 20.0;
    if (ctx.decoupling_scale < 0.1) ctx.decoupling_scale = 0.1;

    // Speed Gate - v0.7.2 Smoothstep S-curve
    ctx.speed_gate = smoothstep(
        (double)m_speed_gate_lower, 
        (double)m_speed_gate_upper, 
        ctx.car_speed
    );

    // --- 5. EFFECT CALCULATIONS ---

    // A. Understeer (Base Torque + Grip Loss)

    // Grip Estimation (v0.4.5 FIX)
    GripResult front_grip_res = calculate_grip(fl, fr, ctx.avg_load, m_warned_grip, 
                                                m_prev_slip_angle[0], m_prev_slip_angle[1],
                                                ctx.car_speed, ctx.dt, data->mVehicleName, data, true /* is_front */);
    ctx.avg_grip = front_grip_res.value;
    m_grip_diag.front_original = front_grip_res.original;
    m_grip_diag.front_approximated = front_grip_res.approximated;
    m_grip_diag.front_slip_angle = front_grip_res.slip_angle;
    if (front_grip_res.approximated) ctx.frame_warn_grip = true;

    // 2. Signal Conditioning (Smoothing, Notch Filters)
    double game_force_proc = apply_signal_conditioning(raw_torque_input, data, ctx);

    // Base Force Mode
    double base_input = 0.0;
    if (m_base_force_mode == 0) {
        base_input = game_force_proc;
    } else if (m_base_force_mode == 1) {
        if (std::abs(game_force_proc) > SYNTHETIC_MODE_DEADZONE_NM) {
            double sign = (game_force_proc > 0.0) ? 1.0 : -1.0;
            base_input = sign * (double)m_max_torque_ref;
        }
    }
    
    // Apply Grip Modulation
    double grip_loss = (1.0 - ctx.avg_grip) * m_understeer_effect;
    ctx.grip_factor = (std::max)(0.0, 1.0 - grip_loss);

    // v0.7.46: Dynamic Weight logic
    update_static_load_reference(ctx.avg_load, ctx.car_speed, ctx.dt);
    double dynamic_weight_factor = 1.0;

    // Only apply if enabled AND we have real load data (no warnings)
    if (m_dynamic_weight_gain > 0.0 && !ctx.frame_warn_load) {
        double load_ratio = ctx.avg_load / m_static_front_load;
        // Blend: 1.0 + (Ratio - 1.0) * Gain
        dynamic_weight_factor = 1.0 + (load_ratio - 1.0) * (double)m_dynamic_weight_gain;
        dynamic_weight_factor = std::clamp(dynamic_weight_factor, 0.5, 2.0);
    }

    // Apply Smoothing to Dynamic Weight (v0.7.47)
    double dw_alpha = ctx.dt / ((double)m_dynamic_weight_smoothing + ctx.dt + 1e-9);
    dw_alpha = (std::max)(0.0, (std::min)(1.0, dw_alpha));
    m_dynamic_weight_smoothed += dw_alpha * (dynamic_weight_factor - m_dynamic_weight_smoothed);
    dynamic_weight_factor = m_dynamic_weight_smoothed;
    
    double output_force = (base_input * (double)m_steering_shaft_gain) * dynamic_weight_factor * ctx.grip_factor;
    output_force *= ctx.speed_gate;
    
    // B. SoP Lateral (Oversteer)
    calculate_sop_lateral(data, ctx);
    
    // C. Gyro Damping
    calculate_gyro_damping(data, ctx);
    
    // D. Effects
    calculate_abs_pulse(data, ctx);
    calculate_lockup_vibration(data, ctx);
    calculate_wheel_spin(data, ctx);
    calculate_slide_texture(data, ctx);
    calculate_road_texture(data, ctx);
    calculate_suspension_bottoming(data, ctx);
    calculate_soft_lock(data, ctx);
    
    // --- 6. SUMMATION ---
    // Split into Structural (Attenuated by Spin) and Texture (Raw) groups
    double structural_sum = output_force + ctx.sop_base_force + ctx.rear_torque + ctx.yaw_force + ctx.gyro_force +
                            ctx.abs_pulse_force + ctx.lockup_rumble + ctx.scrub_drag_force + ctx.soft_lock_force;

    // Apply Torque Drop (from Spin/Traction Loss) only to structural physics
    structural_sum *= ctx.gain_reduction_factor;
    
    double texture_sum = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch;

    double total_sum = structural_sum + texture_sum;

    // --- 7. OUTPUT SCALING ---
    double norm_force = total_sum / max_torque_safe;
    norm_force *= m_gain;

    // Min Force
    if (std::abs(norm_force) > 0.0001 && std::abs(norm_force) < m_min_force) {
        double sign = (norm_force > 0.0) ? 1.0 : -1.0;
        norm_force = sign * m_min_force;
    }

    if (m_invert_force) {
        norm_force *= -1.0;
    }

    // --- 8. STATE UPDATES (POST-CALC) ---
    // CRITICAL: These updates must run UNCONDITIONALLY every frame to prevent
    // stale state issues when effects are toggled on/off.
    for (int i = 0; i < 4; i++) {
        m_prev_vert_deflection[i] = data->mWheel[i].mVerticalTireDeflection;
        m_prev_rotation[i] = data->mWheel[i].mRotation;
        m_prev_brake_pressure[i] = data->mWheel[i].mBrakePressure;
    }
    m_prev_susp_force[0] = fl.mSuspForce;
    m_prev_susp_force[1] = fr.mSuspForce;
    
    // v0.6.36 FIX: Move m_prev_vert_accel to unconditional section
    // Previously only updated inside calculate_road_texture when enabled.
    // Now always updated to prevent stale data if other effects use it.
    m_prev_vert_accel = data->mLocalAccel.y;

    // --- 9. SNAPSHOT ---
    // This block captures the current state of the FFB Engine (inputs, outputs, intermediate calculations)
    // into a thread-safe buffer. These snapshots are retrieved by the GUI layer (or other consumers)
    // to visualize real-time telemetry graphs, FFB clipping, and effect contributions.
    // It uses a mutex to protect the shared circular buffer.
    {
        std::lock_guard<std::mutex> lock(m_debug_mutex);
        if (m_debug_buffer.size() < 100) {
            FFBSnapshot snap;
            snap.total_output = (float)norm_force;
            snap.base_force = (float)base_input;
            snap.sop_force = (float)ctx.sop_unboosted_force; // Use unboosted for snapshot
            snap.understeer_drop = (float)((base_input * m_steering_shaft_gain) * (1.0 - ctx.grip_factor));
            snap.oversteer_boost = (float)(ctx.sop_base_force - ctx.sop_unboosted_force); // Exact boost amount

            snap.ffb_rear_torque = (float)ctx.rear_torque;
            snap.ffb_scrub_drag = (float)ctx.scrub_drag_force;
            snap.ffb_yaw_kick = (float)ctx.yaw_force;
            snap.ffb_gyro_damping = (float)ctx.gyro_force;
            snap.texture_road = (float)ctx.road_noise;
            snap.texture_slide = (float)ctx.slide_noise;
            snap.texture_lockup = (float)ctx.lockup_rumble;
            snap.texture_spin = (float)ctx.spin_rumble;
            snap.texture_bottoming = (float)ctx.bottoming_crunch;
            snap.ffb_abs_pulse = (float)ctx.abs_pulse_force; 
            snap.ffb_soft_lock = (float)ctx.soft_lock_force;
            snap.clipping = (std::abs(norm_force) > 0.99f) ? 1.0f : 0.0f;

            // Physics
            snap.calc_front_load = (float)ctx.avg_load;
            snap.calc_rear_load = (float)ctx.avg_rear_load;
            snap.calc_rear_lat_force = (float)ctx.calc_rear_lat_force;
            snap.calc_front_grip = (float)ctx.avg_grip;
            snap.calc_rear_grip = (float)ctx.avg_rear_grip;
            snap.calc_front_slip_angle_smoothed = (float)m_grip_diag.front_slip_angle;
            snap.calc_rear_slip_angle_smoothed = (float)m_grip_diag.rear_slip_angle;

            snap.raw_front_slip_angle = (float)calculate_raw_slip_angle_pair(fl, fr);
            snap.raw_rear_slip_angle = (float)calculate_raw_slip_angle_pair(data->mWheel[2], data->mWheel[3]);

            // Telemetry
            snap.steer_force = (float)raw_torque;
            snap.raw_input_steering = (float)data->mUnfilteredSteering;
            snap.raw_front_tire_load = (float)raw_load;
            snap.raw_front_grip_fract = (float)raw_grip;
            snap.raw_rear_grip = (float)((data->mWheel[2].mGripFract + data->mWheel[3].mGripFract) / 2.0);
            snap.raw_front_susp_force = (float)((fl.mSuspForce + fr.mSuspForce) / 2.0);
            snap.raw_front_ride_height = (float)((std::min)(fl.mRideHeight, fr.mRideHeight));
            snap.raw_rear_lat_force = (float)((data->mWheel[2].mLateralForce + data->mWheel[3].mLateralForce) / 2.0);
            snap.raw_car_speed = (float)ctx.car_speed_long;
            snap.raw_input_throttle = (float)data->mUnfilteredThrottle;
            snap.raw_input_brake = (float)data->mUnfilteredBrake;
            snap.accel_x = (float)data->mLocalAccel.x;
            snap.raw_front_lat_patch_vel = (float)((std::abs(fl.mLateralPatchVel) + std::abs(fr.mLateralPatchVel)) / 2.0);
            snap.raw_front_deflection = (float)((fl.mVerticalTireDeflection + fr.mVerticalTireDeflection) / 2.0);
            snap.raw_front_long_patch_vel = (float)((fl.mLongitudinalPatchVel + fr.mLongitudinalPatchVel) / 2.0);
            snap.raw_rear_lat_patch_vel = (float)((std::abs(data->mWheel[2].mLateralPatchVel) + std::abs(data->mWheel[3].mLateralPatchVel)) / 2.0);
            snap.raw_rear_long_patch_vel = (float)((data->mWheel[2].mLongitudinalPatchVel + data->mWheel[3].mLongitudinalPatchVel) / 2.0);

            snap.warn_load = ctx.frame_warn_load;
            snap.warn_grip = ctx.frame_warn_grip || ctx.frame_warn_rear_grip;
            snap.warn_dt = ctx.frame_warn_dt;
            snap.debug_freq = (float)m_debug_freq;
            snap.tire_radius = (float)fl.mStaticUndeflectedRadius / 100.0f;
            snap.slope_current = (float)m_slope_current; // v0.7.1: Slope detection diagnostic

            snap.ffb_rate = (float)m_ffb_rate;
            snap.telemetry_rate = (float)m_telemetry_rate;
            snap.hw_rate = (float)m_hw_rate;
            snap.torque_rate = (float)m_torque_rate;
            snap.gen_torque_rate = (float)m_gen_torque_rate;

            m_debug_buffer.push_back(snap);
        }
    }
    
    // Telemetry Logging (v0.7.x)
    if (AsyncLogger::Get().IsLogging()) {
        LogFrame frame;
        frame.timestamp = data->mElapsedTime;
        frame.delta_time = data->mDeltaTime;
        
        // Inputs
        frame.steering = (float)data->mUnfilteredSteering;
        frame.throttle = (float)data->mUnfilteredThrottle;
        frame.brake = (float)data->mUnfilteredBrake;
        
        // Vehicle state
        frame.speed = (float)ctx.car_speed;
        frame.lat_accel = (float)data->mLocalAccel.x;
        frame.long_accel = (float)data->mLocalAccel.z;
        frame.yaw_rate = (float)data->mLocalRot.y;
        
        // Front Axle raw
        frame.slip_angle_fl = (float)fl.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
        frame.slip_angle_fr = (float)fr.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
        frame.slip_ratio_fl = (float)calculate_wheel_slip_ratio(fl);
        frame.slip_ratio_fr = (float)calculate_wheel_slip_ratio(fr);
        frame.grip_fl = (float)fl.mGripFract;
        frame.grip_fr = (float)fr.mGripFract;
        frame.load_fl = (float)fl.mTireLoad;
        frame.load_fr = (float)fr.mTireLoad;
        
        // Calculated values
        frame.calc_slip_angle_front = (float)m_grip_diag.front_slip_angle;
        frame.calc_grip_front = (float)ctx.avg_grip;
        
        // Slope detection
        frame.dG_dt = (float)m_slope_dG_dt;
        frame.dAlpha_dt = (float)m_slope_dAlpha_dt;
        frame.slope_current = (float)m_slope_current;
        frame.slope_raw_unclamped = (float)m_debug_slope_raw;
        frame.slope_numerator = (float)m_debug_slope_num;
        frame.slope_denominator = (float)m_debug_slope_den;
        frame.hold_timer = (float)m_slope_hold_timer;
        frame.input_slip_smoothed = (float)m_slope_slip_smoothed;
        frame.slope_smoothed = (float)m_slope_smoothed_output;
        
        // Use extracted confidence calculation
        frame.confidence = (float)calculate_slope_confidence(m_slope_dAlpha_dt);
        
        // Surface types (Accuracy Tools)
        frame.surface_type_fl = (float)fl.mSurfaceType;
        frame.surface_type_fr = (float)fr.mSurfaceType;

        // Advanced Slope Detection (v0.7.40)
        frame.slope_torque = (float)m_slope_torque_current;
        frame.slew_limited_g = (float)m_debug_lat_g_slew;

        // Rear axle
        frame.calc_grip_rear = (float)ctx.avg_rear_grip;
        frame.grip_delta = (float)(ctx.avg_grip - ctx.avg_rear_grip);
        
        // FFB output
        frame.ffb_total = (float)norm_force;
        frame.ffb_grip_factor = (float)ctx.grip_factor;
        frame.ffb_sop = (float)ctx.sop_base_force;
        frame.speed_gate = (float)ctx.speed_gate;
        frame.load_peak_ref = (float)m_auto_peak_load;
        frame.clipping = (std::abs(norm_force) > 0.99);
        frame.ffb_base = (float)base_input; // Plan included ffb_base
        
        AsyncLogger::Get().Log(frame);
    }
    
    return (std::max)(-1.0, (std::min)(1.0, norm_force));
}

// Helper: Calculate Seat-of-the-Pants (SoP) Lateral & Oversteer Boost
void FFBEngine::calculate_sop_lateral(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // 1. Raw Lateral G (Chassis-relative X)
    // Clamp to 5G to prevent numeric instability in crashes
    double raw_g = (std::max)(-49.05, (std::min)(49.05, data->mLocalAccel.x));
    double lat_g = (raw_g / 9.81);
    
    // Smoothing: Map 0.0-1.0 slider to 0.1-0.0001s tau
    double smoothness = 1.0 - (double)m_sop_smoothing_factor;
    smoothness = (std::max)(0.0, (std::min)(0.999, smoothness));
    double tau = smoothness * 0.1;
    double alpha = ctx.dt / (tau + ctx.dt);
    alpha = (std::max)(0.001, (std::min)(1.0, alpha));
    m_sop_lat_g_smoothed += alpha * (lat_g - m_sop_lat_g_smoothed);
    
    // Base SoP Force
    double sop_base = m_sop_lat_g_smoothed * m_sop_effect * (double)m_sop_scale * ctx.decoupling_scale;
    ctx.sop_unboosted_force = sop_base; // Store for snapshot
    
    // 2. Oversteer Boost (Grip Differential)
    // Calculate Rear Grip
    GripResult rear_grip_res = calculate_grip(data->mWheel[2], data->mWheel[3], ctx.avg_load, m_warned_rear_grip,
                                                m_prev_slip_angle[2], m_prev_slip_angle[3],
                                                ctx.car_speed, ctx.dt, data->mVehicleName, data, false /* is_front */);
    ctx.avg_rear_grip = rear_grip_res.value;
    m_grip_diag.rear_original = rear_grip_res.original;
    m_grip_diag.rear_approximated = rear_grip_res.approximated;
    m_grip_diag.rear_slip_angle = rear_grip_res.slip_angle;
    if (rear_grip_res.approximated) ctx.frame_warn_rear_grip = true;
    
    if (!m_slope_detection_enabled) {
        double grip_delta = ctx.avg_grip - ctx.avg_rear_grip;
        if (grip_delta > 0.0) {
            sop_base *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
        }
    }
    ctx.sop_base_force = sop_base;
    
    // 3. Rear Aligning Torque (v0.4.9)
    // Calculate load for rear wheels (for tire stiffness scaling)
    double calc_load_rl = approximate_rear_load(data->mWheel[2]);
    double calc_load_rr = approximate_rear_load(data->mWheel[3]);
    ctx.avg_rear_load = (calc_load_rl + calc_load_rr) / 2.0;
    
    // Rear lateral force estimation: F = Alpha * k * TireLoad
    double rear_slip_angle = m_grip_diag.rear_slip_angle;
    ctx.calc_rear_lat_force = rear_slip_angle * ctx.avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;
    ctx.calc_rear_lat_force = (std::max)(-MAX_REAR_LATERAL_FORCE, (std::min)(MAX_REAR_LATERAL_FORCE, ctx.calc_rear_lat_force));
    
    // Torque = Force * Aligning_Lever
    // Note negative sign: Oversteer (Rear Slide) pushes wheel TOWARDS slip direction
    ctx.rear_torque = -ctx.calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect * ctx.decoupling_scale;
    
    // 4. Yaw Kick (Inertial Oversteer)
    double raw_yaw_accel = data->mLocalRotAccel.y;
    // v0.4.16: Reject yaw at low speeds and below threshold
    if (ctx.car_speed < 5.0) raw_yaw_accel = 0.0;
    else if (std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) raw_yaw_accel = 0.0;
    
    // Alpha Smoothing (v0.4.16)
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < 0.0001) tau_yaw = 0.0001;
    double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
    m_yaw_accel_smoothed += alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
    
    ctx.yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK * ctx.decoupling_scale;
    
    // Apply speed gate to all lateral effects
    ctx.sop_base_force *= ctx.speed_gate;
    ctx.rear_torque *= ctx.speed_gate;
    ctx.yaw_force *= ctx.speed_gate;
}

// Helper: Calculate Gyroscopic Damping (v0.4.17)
void FFBEngine::calculate_gyro_damping(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // 1. Calculate Steering Velocity (rad/s)
    float range = data->mPhysicalSteeringWheelRange;
    if (range <= 0.0f) range = (float)DEFAULT_STEERING_RANGE_RAD;
    double steer_angle = data->mUnfilteredSteering * (range / 2.0);
    double steer_vel = (steer_angle - m_prev_steering_angle) / ctx.dt;
    m_prev_steering_angle = steer_angle;
    
    // 2. Alpha Smoothing
    double tau_gyro = (double)m_gyro_smoothing;
    if (tau_gyro < 0.0001) tau_gyro = 0.0001;
    double alpha_gyro = ctx.dt / (tau_gyro + ctx.dt);
    m_steering_velocity_smoothed += alpha_gyro * (steer_vel - m_steering_velocity_smoothed);
    
    // 3. Force = -Vel * Gain * Speed_Scaling
    // Speed scaling: Gyro effect increases with wheel RPM (car speed)
    ctx.gyro_force = -1.0 * m_steering_velocity_smoothed * m_gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE) * ctx.decoupling_scale;
}

// Helper: Calculate ABS Pulse (v0.7.53)
void FFBEngine::calculate_abs_pulse(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_abs_pulse_enabled) return;
    
    bool abs_active = false;
    for (int i = 0; i < 4; i++) {
        // Detection: Sudden pressure oscillation + high brake pedal
        double pressure_delta = (data->mWheel[i].mBrakePressure - m_prev_brake_pressure[i]) / ctx.dt;
        if (data->mUnfilteredBrake > ABS_PEDAL_THRESHOLD && std::abs(pressure_delta) > ABS_PRESSURE_RATE_THRESHOLD) {
            abs_active = true;
            break;
        }
    }
    
    if (abs_active) {
        // Generate sine pulse
        m_abs_phase += (double)m_abs_freq_hz * ctx.dt * TWO_PI;
        m_abs_phase = std::fmod(m_abs_phase, TWO_PI);
        ctx.abs_pulse_force = (double)(std::sin(m_abs_phase) * m_abs_gain * 2.0 * ctx.decoupling_scale * ctx.speed_gate);
    }
}

// Helper: Calculate Lockup Vibration (v0.4.36 - REWRITTEN as dedicated method)
void FFBEngine::calculate_lockup_vibration(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_lockup_enabled) return;
    
    double worst_severity = 0.0;
    double chosen_freq_multiplier = 1.0;
    double chosen_pressure_factor = 0.0;
    
    // Calculate reference slip for front wheels (v0.4.38)
    double slip_fl = calculate_wheel_slip_ratio(data->mWheel[0]);
    double slip_fr = calculate_wheel_slip_ratio(data->mWheel[1]);
    double worst_front = (std::min)(slip_fl, slip_fr);

    for (int i = 0; i < 4; i++) {
        const auto& w = data->mWheel[i];
        double slip = calculate_wheel_slip_ratio(w);
        double slip_abs = std::abs(slip);

        // 1. Predictive Lockup (v0.4.38)
        // Detects rapidly decelerating wheels BEFORE they reach full lock
        double wheel_accel = (w.mRotation - m_prev_rotation[i]) / ctx.dt;
        double radius = (double)w.mStaticUndeflectedRadius / 100.0;
        if (radius < 0.1) radius = 0.33;
        double car_dec_ang = -std::abs(data->mLocalAccel.z / radius);

        // Signal Quality Check (Reject surface bumps)
        double susp_vel = std::abs(w.mVerticalTireDeflection - m_prev_vert_deflection[i]) / ctx.dt;
        bool is_bumpy = (susp_vel > (double)m_lockup_bump_reject);

        // Pre-conditions
        bool brake_active = (data->mUnfilteredBrake > PREDICTION_BRAKE_THRESHOLD);
        bool is_grounded = (w.mSuspForce > PREDICTION_LOAD_THRESHOLD);

        double start_threshold = (double)m_lockup_start_pct / 100.0;
        double full_threshold = (double)m_lockup_full_pct / 100.0;
        double trigger_threshold = full_threshold;

        if (brake_active && is_grounded && !is_bumpy) {
            // Predictive Trigger: Wheel decelerating significantly faster than chassis
            double sensitivity_threshold = -1.0 * (double)m_lockup_prediction_sens;
            if (wheel_accel < car_dec_ang * 2.0 && wheel_accel < sensitivity_threshold) {
                trigger_threshold = start_threshold; // Ease into effect earlier
            }
        }

        // 2. Intensity Calculation
        if (slip_abs > trigger_threshold) {
            double window = full_threshold - start_threshold;
            if (window < 0.01) window = 0.01;

            double normalized = (slip_abs - start_threshold) / window;
            double severity = (std::min)(1.0, (std::max)(0.0, normalized));
            
            // Apply gamma for curve control
            severity = std::pow(severity, (double)m_lockup_gamma);
            
            // Frequency calculation
            double freq_mult = 1.0;
            if (i >= 2) {
                // v0.4.38: Rear wheels use a different frequency to distinguish front/rear lockup
                if (slip < (worst_front - AXLE_DIFF_HYSTERESIS)) {
                    freq_mult = LOCKUP_FREQ_MULTIPLIER_REAR;
                }
            }

            // Pressure weighting (v0.4.38)
            double pressure_factor = w.mBrakePressure;
            if (pressure_factor < 0.1 && slip_abs > 0.5) pressure_factor = 0.5; // Catch low-pressure lockups

            if (severity > worst_severity) {
                worst_severity = severity;
                chosen_freq_multiplier = freq_mult;
                chosen_pressure_factor = pressure_factor;
            }
        }
    }

    // 3. Vibration Synthesis
    if (worst_severity > 0.0) {
        double base_freq = 10.0 + (ctx.car_speed * 1.5);
        double final_freq = base_freq * chosen_freq_multiplier * (double)m_lockup_freq_scale;
        
        m_lockup_phase += final_freq * ctx.dt * TWO_PI;
        m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI);
        
        double amp = worst_severity * chosen_pressure_factor * m_lockup_gain * (double)BASE_NM_LOCKUP_VIBRATION * ctx.decoupling_scale * ctx.brake_load_factor;
        
        // v0.4.38: Boost rear lockup volume
        if (chosen_freq_multiplier < 1.0) amp *= (double)m_lockup_rear_boost;

        ctx.lockup_rumble = std::sin(m_lockup_phase) * amp * ctx.speed_gate;
    }
}

// Helper: Calculate Wheel Spin Vibration (v0.6.36)
void FFBEngine::calculate_wheel_spin(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (m_spin_enabled && data->mUnfilteredThrottle > 0.05) {
        double slip_rl = calculate_wheel_slip_ratio(data->mWheel[2]);
        double slip_rr = calculate_wheel_slip_ratio(data->mWheel[3]);
        double max_slip = (std::max)(slip_rl, slip_rr);
        
        if (max_slip > 0.2) {
            double severity = (max_slip - 0.2) / 0.5;
            severity = (std::min)(1.0, severity);
            
            // Attenuate primary torque when spinning (Torque Drop)
            // v0.6.43: Blunted effect (0.6 multiplier) to prevent complete loss of feel
            ctx.gain_reduction_factor = (1.0 - (severity * m_spin_gain * 0.6));
            
            // Generate vibration based on spin velocity (RPM delta)
            double slip_speed_ms = ctx.car_speed * max_slip;
            double freq = (10.0 + (slip_speed_ms * 2.5)) * (double)m_spin_freq_scale;
            if (freq > 80.0) freq = 80.0; // Human sensory limit for gross vibration
            
            m_spin_phase += freq * ctx.dt * TWO_PI;
            m_spin_phase = std::fmod(m_spin_phase, TWO_PI);
            
            double amp = severity * m_spin_gain * (double)BASE_NM_SPIN_VIBRATION * ctx.decoupling_scale;
            ctx.spin_rumble = std::sin(m_spin_phase) * amp;
        }
    }
}

// Helper: Calculate Slide Texture (Friction Vibration)
void FFBEngine::calculate_slide_texture(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_slide_texture_enabled) return;
    
    // Use average lateral patch velocity of front wheels
    double lat_vel_fl = std::abs(data->mWheel[0].mLateralPatchVel);
    double lat_vel_fr = std::abs(data->mWheel[1].mLateralPatchVel);
    double front_slip_avg = (lat_vel_fl + lat_vel_fr) / 2.0;

    // Use average lateral patch velocity of rear wheels
    double lat_vel_rl = std::abs(data->mWheel[2].mLateralPatchVel);
    double lat_vel_rr = std::abs(data->mWheel[3].mLateralPatchVel);
    double rear_slip_avg = (lat_vel_rl + lat_vel_rr) / 2.0;

    // Use the max slide velocity between axles
    double effective_slip_vel = (std::max)(front_slip_avg, rear_slip_avg);
    
    if (effective_slip_vel > 1.5) {
        // High-frequency sawtooth noise for localized friction feel
        double base_freq = 10.0 + (effective_slip_vel * 5.0);
        double freq = base_freq * (double)m_slide_freq_scale;
        
        if (freq > 250.0) freq = 250.0; // Hard clamp for hardware safety
        
        m_slide_phase += freq * ctx.dt * TWO_PI;
        m_slide_phase = std::fmod(m_slide_phase, TWO_PI);
        
        // Sawtooth generator (0 to 1 range across TWO_PI) -> (-1 to 1)
        double sawtooth = (m_slide_phase / TWO_PI) * 2.0 - 1.0;
        
        // Intensity scaling (Grip based)
        double grip_scale = (std::max)(0.0, 1.0 - ctx.avg_grip);
        
        ctx.slide_noise = sawtooth * m_slide_texture_gain * (double)BASE_NM_SLIDE_TEXTURE * ctx.texture_load_factor * grip_scale * ctx.decoupling_scale;
    }
}

// Helper: Calculate Road Texture & Scrub Drag
void FFBEngine::calculate_road_texture(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // 1. Scrub Drag (Longitudinal resistive force from lateral sliding)
    if (m_scrub_drag_gain > 0.0) {
        double avg_lat_vel = (data->mWheel[0].mLateralPatchVel + data->mWheel[1].mLateralPatchVel) / 2.0;
        double abs_lat_vel = std::abs(avg_lat_vel);
        
        if (abs_lat_vel > 0.001) {
            double fade = (std::min)(1.0, abs_lat_vel / 0.5); // Fade in over 0.5m/s
            double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;
            ctx.scrub_drag_force = drag_dir * m_scrub_drag_gain * (double)BASE_NM_SCRUB_DRAG * fade * ctx.decoupling_scale;
        }
    }

    if (!m_road_texture_enabled) return;
    
    // 2. Road Texture (Delta Deflection Method)
    // Measures the rate of change in tire vertical compression
    double delta_l = data->mWheel[0].mVerticalTireDeflection - m_prev_vert_deflection[0];
    double delta_r = data->mWheel[1].mVerticalTireDeflection - m_prev_vert_deflection[1];
    
    // Outlier rejection (crashes/jumps)
    delta_l = (std::max)(-0.01, (std::min)(0.01, delta_l));
    delta_r = (std::max)(-0.01, (std::min)(0.01, delta_r));
    
    double road_noise_val = 0.0;
    
    // FALLBACK (v0.6.36): If mVerticalTireDeflection is missing (Encrypted DLC),
    // use Chassis Vertical Acceleration delta as a secondary source.
    bool deflection_active = (std::abs(delta_l) > 0.000001 || std::abs(delta_r) > 0.000001);
    
    if (deflection_active || ctx.car_speed < 5.0) {
        road_noise_val = (delta_l + delta_r) * 50.0; // Scale to NM
    } else {
        // Fallback to vertical acceleration rate-of-change (jerk-like scaling)
        double vert_accel = data->mLocalAccel.y;
        double delta_accel = vert_accel - m_prev_vert_accel;
        road_noise_val = delta_accel * 0.05 * 50.0; // Blend into similar range
    }
    
    ctx.road_noise = road_noise_val * m_road_texture_gain * ctx.decoupling_scale * ctx.texture_load_factor;
    ctx.road_noise *= ctx.speed_gate;
}

// Helper: Calculate Suspension Bottoming (v0.6.22)
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
        ctx.soft_lock_force = -(spring * sign + damping) * ctx.decoupling_scale;
    }
}

void FFBEngine::calculate_suspension_bottoming(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_bottoming_enabled) return;
    bool triggered = false;
    double intensity = 0.0;
    
    // Method 0: Direct Ride Height Monitoring
    if (m_bottoming_method == 0) {
        double min_rh = (std::min)(data->mWheel[0].mRideHeight, data->mWheel[1].mRideHeight);
        if (min_rh < 0.002 && min_rh > -1.0) { // < 2mm
            triggered = true;
            intensity = (0.002 - min_rh) / 0.002; // Map 2mm->0mm to 0.0->1.0
        }
    } else {
        // Method 1: Suspension Force Impulse (Rate of Change)
        double dForceL = (data->mWheel[0].mSuspForce - m_prev_susp_force[0]) / ctx.dt;
        double dForceR = (data->mWheel[1].mSuspForce - m_prev_susp_force[1]) / ctx.dt;
        double max_dForce = (std::max)(dForceL, dForceR);
        
        if (max_dForce > 100000.0) { // 100kN/s impulse
            triggered = true;
            intensity = (max_dForce - 100000.0) / 200000.0;
        }
    }

    // Safety Trigger: Raw Load Peak (Catches Method 0/1 failures)
    if (!triggered) {
        double max_load = (std::max)(data->mWheel[0].mTireLoad, data->mWheel[1].mTireLoad);
        double bottoming_threshold = m_auto_peak_load * 1.6;
        if (max_load > bottoming_threshold) {
            triggered = true;
            double excess = max_load - bottoming_threshold;
            intensity = std::sqrt(excess) * 0.05; // Non-linear response
        }
    }

    if (triggered) {
        // Generate high-intensity low-frequency "thump"
        double bump_magnitude = intensity * m_bottoming_gain * (double)BASE_NM_BOTTOMING * ctx.decoupling_scale;
        double freq = 50.0;
        
        m_bottoming_phase += freq * ctx.dt * TWO_PI;
        m_bottoming_phase = std::fmod(m_bottoming_phase, TWO_PI);
        
        ctx.bottoming_crunch = std::sin(m_bottoming_phase) * bump_magnitude * ctx.speed_gate;
    }
}
