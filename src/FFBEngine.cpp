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

bool FFBEngine::IsFFBAllowed(const VehicleScoringInfoV01& scoring, unsigned char gamePhase) const {
    if (!scoring.mIsPlayer || scoring.mControl != 0) return false;
    if (gamePhase == 7 || gamePhase == 8) return false;
    if (scoring.mFinishStatus == 3) return false;
    return true;
}

double FFBEngine::ApplySafetySlew(double target_force, double dt, bool restricted) {
    if (!std::isfinite(target_force)) return 0.0;
    double max_slew = restricted ? 100.0 : 1000.0;
    double max_change = max_slew * dt;
    double delta = target_force - m_last_output_force;
    delta = std::clamp(delta, -max_change, max_change);
    m_last_output_force += delta;
    return m_last_output_force;
}

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

void FFBEngine::update_static_load_reference(double current_load, double speed, double dt) {
    if (speed > 2.0 && speed < 15.0) {
        if (m_static_front_load < 100.0) {
             m_static_front_load = current_load;
        } else {
             m_static_front_load += (dt / 5.0) * (current_load - m_static_front_load);
        }
    }
    if (m_static_front_load < 1000.0) m_static_front_load = 4000.0;
}

void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    ParsedVehicleClass vclass = ParseVehicleClass(className, vehicleName);
    m_auto_peak_load = GetDefaultLoadForClass(vclass);
    std::cout << "[FFB] Vehicle Identification -> Detected Class: " << VehicleClassToString(vclass)
              << " | Seed Load: " << m_auto_peak_load << "N" 
              << " (Raw -> Class: " << (className ? className : "Unknown") 
              << ", Name: " << (vehicleName ? vehicleName : "Unknown") << ")" << std::endl;
}

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
    double raw_angle = std::atan2(w.mLateralPatchVel, v_long);
    double tau = (double)m_slip_angle_smoothing;
    if (tau < 0.0001) tau = 0.0001;
    double alpha = dt / (tau + dt);
    alpha = (std::min)(1.0, (std::max)(0.001, alpha));
    prev_state = prev_state + alpha * (raw_angle - prev_state);
    return prev_state;
}

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
        result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
    }

    result.value = result.original;
    result.approximated = false;
    result.slip_angle = 0.0;
    
    double slip1 = calculate_slip_angle(w1, prev_slip1, dt);
    double slip2 = calculate_slip_angle(w2, prev_slip2, dt);
    result.slip_angle = (slip1 + slip2) / 2.0;

    if (result.value < 0.0001 && avg_load > 100.0) {
        result.approximated = true;
        
        if (car_speed < 5.0) {
            result.value = 1.0; 
        } else {
            if (m_slope_detection_enabled && is_front && data) {
                result.value = calculate_slope_grip(
                    data->mLocalAccel.x / 9.81,
                    result.slip_angle,
                    dt,
                    data
                );
            } else {
                double lat_metric = std::abs(result.slip_angle) / (double)m_optimal_slip_angle;
                double ratio1 = calculate_manual_slip_ratio(w1, car_speed);
                double ratio2 = calculate_manual_slip_ratio(w2, car_speed);
                double avg_ratio = (std::abs(ratio1) + std::abs(ratio2)) / 2.0;
                double long_metric = avg_ratio / (double)m_optimal_slip_ratio;
                double combined_slip = std::sqrt((lat_metric * lat_metric) + (long_metric * long_metric));

                if (combined_slip > 1.0) {
                    double excess = combined_slip - 1.0;
                    result.value = 1.0 / (1.0 + excess * 2.0);
                } else {
                    result.value = 1.0;
                }
            }
        }
        
        result.value = (std::max)(0.2, result.value);
        
        if (!warned_flag) {
            std::cout << "Warning: Data for mGripFract from the game seems to be missing for this car (" << vehicleName << "). (Likely Encrypted/DLC Content). A fallback estimation will be used." << std::endl;
            warned_flag = true;
        }
    }
    
    double& state = is_front ? m_front_grip_smoothed_state : m_rear_grip_smoothed_state;
    result.value = apply_adaptive_smoothing(result.value, state, dt,
                                            (double)m_grip_smoothing_steady,
                                            (double)m_grip_smoothing_fast,
                                            (double)m_grip_smoothing_sensitivity);

    result.value = (std::max)(0.0, (std::min)(1.0, result.value));
    return result;
}

double FFBEngine::approximate_load(const TelemWheelV01& w) {
    return w.mSuspForce + 300.0;
}

double FFBEngine::approximate_rear_load(const TelemWheelV01& w) {
    return w.mSuspForce + 300.0;
}

double FFBEngine::calculate_kinematic_load(const TelemInfoV01* data, int wheel_index) {
    bool is_rear = (wheel_index >= 2);
    double bias = is_rear ? m_approx_weight_bias : (1.0 - m_approx_weight_bias);
    double static_weight = (m_approx_mass_kg * 9.81 * bias) / 2.0;

    double speed = std::abs(data->mLocalVel.z);
    double aero_load = m_approx_aero_coeff * (speed * speed);
    double wheel_aero = aero_load / 4.0; 

    double long_transfer = (m_accel_z_smoothed / 9.81) * WEIGHT_TRANSFER_SCALE; 
    if (is_rear) long_transfer *= -1.0;

    double lat_transfer = (m_accel_x_smoothed / 9.81) * WEIGHT_TRANSFER_SCALE * m_approx_roll_stiffness;
    bool is_left = (wheel_index == 0 || wheel_index == 2);
    if (!is_left) lat_transfer *= -1.0;

    double total_load = static_weight + wheel_aero + long_transfer + lat_transfer;
    return (std::max)(0.0, total_load);
}

double FFBEngine::calculate_manual_slip_ratio(const TelemWheelV01& w, double car_speed_ms) {
    if (std::abs(car_speed_ms) < 2.0) return 0.0;
    double radius_m = (double)w.mStaticUndeflectedRadius / 100.0;
    if (radius_m < 0.1) radius_m = 0.33;
    
    double wheel_vel = w.mRotation * radius_m;
    double denom = std::abs(car_speed_ms);
    if (denom < 1.0) denom = 1.0;
    return (wheel_vel - car_speed_ms) / denom;
}

double FFBEngine::calculate_slope_grip(double lateral_g, double slip_angle, double dt, const TelemInfoV01* data) {
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

    m_slope_lat_g_buffer[m_slope_buffer_index] = m_slope_lat_g_smoothed;
    m_slope_slip_buffer[m_slope_buffer_index] = m_slope_slip_smoothed;
    if (data) {
        m_slope_torque_buffer[m_slope_buffer_index] = m_slope_torque_smoothed;
        m_slope_steer_buffer[m_slope_buffer_index] = m_slope_steer_smoothed;
    }

    m_slope_buffer_index = (m_slope_buffer_index + 1) % SLOPE_BUFFER_MAX;
    if (m_slope_buffer_count < SLOPE_BUFFER_MAX) m_slope_buffer_count++;

    double dG_dt = calculate_sg_derivative(m_slope_lat_g_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);
    double dAlpha_dt = calculate_sg_derivative(m_slope_slip_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);

    m_slope_dG_dt = dG_dt;
    m_slope_dAlpha_dt = dAlpha_dt;

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

    volatile bool can_calc_torque = (m_slope_use_torque && data != nullptr);
    if (can_calc_torque) {
        double dTorque_dt = calculate_sg_derivative(m_slope_torque_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);
        double dSteer_dt = calculate_sg_derivative(m_slope_steer_buffer, m_slope_buffer_count, m_slope_sg_window, dt, m_slope_buffer_index);

        if (std::abs(dSteer_dt) > (double)m_slope_alpha_threshold) {
            m_debug_slope_torque_num = dTorque_dt * dSteer_dt;
            m_debug_slope_torque_den = (dSteer_dt * dSteer_dt) + 0.000001;
            m_slope_torque_current = std::clamp(m_debug_slope_torque_num / m_debug_slope_torque_den, -50.0, 50.0);
        } else {
            m_slope_torque_current += (double)m_slope_decay_rate * dt * (0.0 - m_slope_torque_current);
        }
    } else {
        m_slope_torque_current = 20.0;
    }

    double current_grip_factor = 1.0;
    double confidence = calculate_slope_confidence(dAlpha_dt);

    double loss_percent_g = inverse_lerp((double)m_slope_min_threshold, (double)m_slope_max_threshold, m_slope_current);

    double loss_percent_torque = 0.0;
    volatile bool use_torque_fusion = (m_slope_use_torque && data != nullptr);
    if (use_torque_fusion) {
        if (m_slope_torque_current < 0.0) {
            loss_percent_torque = std::abs(m_slope_torque_current) * (double)m_slope_torque_sensitivity;
            loss_percent_torque = (std::max)(0.0, (std::min)(1.0, loss_percent_torque));
        }
    }

    double loss_percent = (std::max)(loss_percent_g, loss_percent_torque);
    current_grip_factor = 1.0 - (loss_percent * 0.8 * confidence);
    current_grip_factor = (std::max)(0.2, (std::min)(1.0, current_grip_factor));

    double alpha = dt / ((double)m_slope_smoothing_tau + dt);
    alpha = (std::max)(0.001, (std::min)(1.0, alpha));
    m_slope_smoothed_output += alpha * (current_grip_factor - m_slope_smoothed_output);

    return m_slope_smoothed_output;
}

double FFBEngine::calculate_slope_confidence(double dAlpha_dt) {
    if (!m_slope_confidence_enabled) return 1.0;
    return smoothstep((double)m_slope_alpha_threshold, (double)m_slope_confidence_max_rate, std::abs(dAlpha_dt));
}

double FFBEngine::calculate_wheel_slip_ratio(const TelemWheelV01& w) {
    double v_long = std::abs(w.mLongitudinalGroundVel);
    if (v_long < MIN_SLIP_ANGLE_VELOCITY) v_long = MIN_SLIP_ANGLE_VELOCITY;
    return w.mLongitudinalPatchVel / v_long;
}

double FFBEngine::apply_signal_conditioning(double raw_torque, const TelemInfoV01* data, FFBCalculationContext& ctx) {
    double game_force_proc = raw_torque;

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

double FFBEngine::calculate_force(const TelemInfoV01* data, const char* vehicleClass, const char* vehicleName, float genFFBTorque) {
    if (!data) return 0.0;

    double raw_torque_input = (m_torque_source == 1) ? (double)genFFBTorque : data->mSteeringShaftTorque;

    if (!std::isfinite(raw_torque_input)) return 0.0;

    bool seeded = false;
    if (vehicleClass && m_current_class_name != vehicleClass) {
        m_current_class_name = vehicleClass;
        InitializeLoadReference(vehicleClass, vehicleName);
        seeded = true;
    }
    
    FFBCalculationContext ctx;
    ctx.dt = data->mDeltaTime;

    if (ctx.dt <= 0.000001) {
        ctx.dt = 0.0025; 
        if (!m_warned_dt) {
            std::cout << "[WARNING] Invalid DeltaTime (<=0). Using default 0.0025s." << std::endl;
            m_warned_dt = true;
        }
        ctx.frame_warn_dt = true;
    }
    
    ctx.car_speed_long = data->mLocalVel.z;
    ctx.car_speed = std::abs(ctx.car_speed_long);
    
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

    double chassis_tau = (double)m_chassis_inertia_smoothing;
    if (chassis_tau < 0.0001) chassis_tau = 0.0001;
    double alpha_chassis = ctx.dt / (chassis_tau + ctx.dt);
    m_accel_x_smoothed += alpha_chassis * (data->mLocalAccel.x - m_accel_x_smoothed);
    m_accel_z_smoothed += alpha_chassis * (data->mLocalAccel.z - m_accel_z_smoothed);

    const TelemWheelV01& fl = data->mWheel[0];
    const TelemWheelV01& fr = data->mWheel[1];

    double raw_torque = raw_torque_input;
    double raw_load = (fl.mTireLoad + fr.mTireLoad) / 2.0;
    double raw_grip = (fl.mGripFract + fr.mGripFract) / 2.0;

    s_torque.Update(raw_torque);
    s_load.Update(raw_load);
    s_grip.Update(raw_grip);
    s_lat_g.Update(data->mLocalAccel.x);
    
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1) {
        s_torque.ResetInterval(); 
        s_load.ResetInterval(); 
        s_grip.ResetInterval(); 
        s_lat_g.ResetInterval();
        last_log_time = now;
    }

    ctx.avg_load = raw_load;

    if (ctx.avg_load < 1.0 && ctx.car_speed > 1.0) {
        m_missing_load_frames++;
    } else {
        m_missing_load_frames = (std::max)(0, m_missing_load_frames - 1);
    }

    if (m_missing_load_frames > 20) {
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
    
    if (m_auto_load_normalization_enabled && !seeded) {
        if (ctx.avg_load > m_auto_peak_load) {
            m_auto_peak_load = ctx.avg_load; 
        } else {
            m_auto_peak_load -= (100.0 * ctx.dt); 
        }
    }
    m_auto_peak_load = (std::max)(3000.0, m_auto_peak_load); 

    double raw_load_factor = ctx.avg_load / m_auto_peak_load;
    double texture_safe_max = (std::min)(10.0, (double)m_texture_load_cap);
    ctx.texture_load_factor = (std::min)(texture_safe_max, (std::max)(0.0, raw_load_factor));

    double brake_safe_max = (std::min)(10.0, (double)m_brake_load_cap);
    ctx.brake_load_factor = (std::min)(brake_safe_max, (std::max)(0.0, raw_load_factor));
    
    double max_torque_safe = (double)m_max_torque_ref;
    if (max_torque_safe < 1.0) max_torque_safe = 1.0;
    ctx.decoupling_scale = max_torque_safe / 20.0;
    if (ctx.decoupling_scale < 0.1) ctx.decoupling_scale = 0.1;

    ctx.speed_gate = smoothstep(
        (double)m_speed_gate_lower, 
        (double)m_speed_gate_upper, 
        ctx.car_speed
    );

    GripResult front_grip_res = calculate_grip(fl, fr, ctx.avg_load, m_warned_grip, 
                                                m_prev_slip_angle[0], m_prev_slip_angle[1],
                                                ctx.car_speed, ctx.dt, data->mVehicleName, data, true);
    ctx.avg_grip = front_grip_res.value;
    m_grip_diag.front_original = front_grip_res.original;
    m_grip_diag.front_approximated = front_grip_res.approximated;
    m_grip_diag.front_slip_angle = front_grip_res.slip_angle;
    if (front_grip_res.approximated) ctx.frame_warn_grip = true;

    double game_force_proc = apply_signal_conditioning(raw_torque_input, data, ctx);

    double base_input = 0.0;
    if (m_base_force_mode == 0) {
        base_input = game_force_proc;
    } else if (m_base_force_mode == 1) {
        if (std::abs(game_force_proc) > SYNTHETIC_MODE_DEADZONE_NM) {
            double sign = (game_force_proc > 0.0) ? 1.0 : -1.0;
            base_input = sign * (double)m_max_torque_ref;
        }
    }
    
    double grip_loss = (1.0 - ctx.avg_grip) * m_understeer_effect;
    ctx.grip_factor = (std::max)(0.0, 1.0 - grip_loss);

    update_static_load_reference(ctx.avg_load, ctx.car_speed, ctx.dt);
    double dynamic_weight_factor = 1.0;

    if (m_dynamic_weight_gain > 0.0 && !ctx.frame_warn_load) {
        double load_ratio = ctx.avg_load / m_static_front_load;
        dynamic_weight_factor = 1.0 + (load_ratio - 1.0) * (double)m_dynamic_weight_gain;
        dynamic_weight_factor = std::clamp(dynamic_weight_factor, 0.5, 2.0);
    }

    double dw_alpha = ctx.dt / ((double)m_dynamic_weight_smoothing + ctx.dt + 1e-9);
    dw_alpha = (std::max)(0.0, (std::min)(1.0, dw_alpha));
    m_dynamic_weight_smoothed += dw_alpha * (dynamic_weight_factor - m_dynamic_weight_smoothed);
    dynamic_weight_factor = m_dynamic_weight_smoothed;
    
    double output_force = (base_input * (double)m_steering_shaft_gain) * dynamic_weight_factor * ctx.grip_factor;
    output_force *= ctx.speed_gate;
    
    calculate_sop_lateral(data, ctx);
    
    calculate_gyro_damping(data, ctx);
    
    calculate_abs_pulse(data, ctx);
    calculate_lockup_vibration(data, ctx);
    calculate_wheel_spin(data, ctx);
    calculate_slide_texture(data, ctx);
    calculate_road_texture(data, ctx);
    calculate_suspension_bottoming(data, ctx);
    
    double structural_sum = output_force + ctx.sop_base_force + ctx.rear_torque + ctx.yaw_force + ctx.gyro_force +
                            ctx.abs_pulse_force + ctx.lockup_rumble + ctx.scrub_drag_force;

    structural_sum *= ctx.gain_reduction_factor;
    
    double texture_sum = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch;

    double total_sum = structural_sum + texture_sum;

    double norm_force = total_sum / max_torque_safe;
    norm_force *= m_gain;

    if (std::abs(norm_force) > 0.0001 && std::abs(norm_force) < m_min_force) {
        double sign = (norm_force > 0.0) ? 1.0 : -1.0;
        norm_force = sign * m_min_force;
    }

    if (m_invert_force) {
        norm_force *= -1.0;
    }

    for (int i = 0; i < 4; i++) {
        m_prev_vert_deflection[i] = data->mWheel[i].mVerticalTireDeflection;
        m_prev_rotation[i] = data->mWheel[i].mRotation;
        m_prev_brake_pressure[i] = data->mWheel[i].mBrakePressure;
    }
    m_prev_susp_force[0] = fl.mSuspForce;
    m_prev_susp_force[1] = fr.mSuspForce;
    
    m_prev_vert_accel = data->mLocalAccel.y;

    {
        std::lock_guard<std::mutex> lock(m_debug_mutex);
        if (m_debug_buffer.size() < 100) {
            FFBSnapshot snap;
            snap.total_output = (float)norm_force;
            snap.base_force = (float)base_input;
            snap.sop_force = (float)ctx.sop_unboosted_force; 
            snap.understeer_drop = (float)((base_input * m_steering_shaft_gain) * (1.0 - ctx.grip_factor));
            snap.oversteer_boost = (float)(ctx.sop_base_force - ctx.sop_unboosted_force); 

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
            snap.clipping = (std::abs(norm_force) > 0.99f) ? 1.0f : 0.0f;

            snap.calc_front_load = (float)ctx.avg_load;
            snap.calc_rear_load = (float)ctx.avg_rear_load;
            snap.calc_rear_lat_force = (float)ctx.calc_rear_lat_force;
            snap.calc_front_grip = (float)ctx.avg_grip;
            snap.calc_rear_grip = (float)ctx.avg_rear_grip;
            snap.calc_front_slip_angle_smoothed = (float)m_grip_diag.front_slip_angle;
            snap.calc_rear_slip_angle_smoothed = (float)m_grip_diag.rear_slip_angle;

            snap.raw_front_slip_angle = (float)calculate_raw_slip_angle_pair(fl, fr);
            snap.raw_rear_slip_angle = (float)calculate_raw_slip_angle_pair(data->mWheel[2], data->mWheel[3]);

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
            snap.slope_current = (float)m_slope_current; 

            snap.ffb_rate = (float)m_ffb_rate;
            snap.telemetry_rate = (float)m_telemetry_rate;
            snap.hw_rate = (float)m_hw_rate;
            snap.torque_rate = (float)m_torque_rate;
            snap.gen_torque_rate = (float)m_gen_torque_rate;

            m_debug_buffer.push_back(snap);
        }
    }
    
    if (AsyncLogger::Get().IsLogging()) {
        LogFrame frame;
        frame.timestamp = data->mElapsedTime;
        frame.delta_time = data->mDeltaTime;
        
        frame.steering = (float)data->mUnfilteredSteering;
        frame.throttle = (float)data->mUnfilteredThrottle;
        frame.brake = (float)data->mUnfilteredBrake;
        
        frame.speed = (float)ctx.car_speed;
        frame.lat_accel = (float)data->mLocalAccel.x;
        frame.long_accel = (float)data->mLocalAccel.z;
        frame.yaw_rate = (float)data->mLocalRot.y;
        
        frame.slip_angle_fl = (float)fl.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
        frame.slip_angle_fr = (float)fr.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
        frame.slip_ratio_fl = (float)calculate_wheel_slip_ratio(fl);
        frame.slip_ratio_fr = (float)calculate_wheel_slip_ratio(fr);
        frame.grip_fl = (float)fl.mGripFract;
        frame.grip_fr = (float)fr.mGripFract;
        frame.load_fl = (float)fl.mTireLoad;
        frame.load_fr = (float)fr.mTireLoad;
        
        frame.calc_slip_angle_front = (float)m_grip_diag.front_slip_angle;
        frame.calc_grip_front = (float)ctx.avg_grip;
        
        frame.dG_dt = (float)m_slope_dG_dt;
        frame.dAlpha_dt = (float)m_slope_dAlpha_dt;
        frame.slope_current = (float)m_slope_current;
        frame.slope_raw_unclamped = (float)m_debug_slope_raw;
        frame.slope_numerator = (float)m_debug_slope_num;
        frame.slope_denominator = (float)m_debug_slope_den;
        frame.hold_timer = (float)m_slope_hold_timer;
        frame.input_slip_smoothed = (float)m_slope_slip_smoothed;
        frame.slope_smoothed = (float)m_slope_smoothed_output;
        
        frame.confidence = (float)calculate_slope_confidence(m_slope_dAlpha_dt);
        
        frame.surface_type_fl = (float)fl.mSurfaceType;
        frame.surface_type_fr = (float)fr.mSurfaceType;

        frame.slope_torque = (float)m_slope_torque_current;
        frame.slew_limited_g = (float)m_debug_lat_g_slew;

        frame.calc_grip_rear = (float)ctx.avg_rear_grip;
        frame.grip_delta = (float)(ctx.avg_grip - ctx.avg_rear_grip);
        
        frame.ffb_total = (float)norm_force;
        frame.ffb_grip_factor = (float)ctx.grip_factor;
        frame.ffb_sop = (float)ctx.sop_base_force;
        frame.speed_gate = (float)ctx.speed_gate;
        frame.load_peak_ref = (float)m_auto_peak_load;
        frame.clipping = (std::abs(norm_force) > 0.99);
        frame.ffb_base = (float)base_input;
        
        AsyncLogger::Get().Log(frame);
    }
    
    return (std::max)(-1.0, (std::min)(1.0, norm_force));
}

void FFBEngine::calculate_sop_lateral(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    double raw_g = (std::max)(-49.05, (std::min)(49.05, data->mLocalAccel.x));
    double lat_g = (raw_g / 9.81);
    
    double smoothness = 1.0 - (double)m_sop_smoothing_factor;
    smoothness = (std::max)(0.0, (std::min)(0.999, smoothness));
    double tau = smoothness * 0.1;
    double alpha = ctx.dt / (tau + ctx.dt);
    alpha = (std::max)(0.001, (std::min)(1.0, alpha));
    m_sop_lat_g_smoothed += alpha * (lat_g - m_sop_lat_g_smoothed);
    
    double sop_base = m_sop_lat_g_smoothed * m_sop_effect * (double)m_sop_scale * ctx.decoupling_scale;
    ctx.sop_unboosted_force = sop_base; 
    
    GripResult rear_grip_res = calculate_grip(data->mWheel[2], data->mWheel[3], ctx.avg_load, m_warned_rear_grip,
                                                m_prev_slip_angle[2], m_prev_slip_angle[3],
                                                ctx.car_speed, ctx.dt, data->mVehicleName, data, false);
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
    
    double calc_load_rl = approximate_rear_load(data->mWheel[2]);
    double calc_load_rr = approximate_rear_load(data->mWheel[3]);
    ctx.avg_rear_load = (calc_load_rl + calc_load_rr) / 2.0;
    
    double rear_slip_angle = m_grip_diag.rear_slip_angle;
    ctx.calc_rear_lat_force = rear_slip_angle * ctx.avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;
    ctx.calc_rear_lat_force = (std::max)(-MAX_REAR_LATERAL_FORCE, (std::min)(MAX_REAR_LATERAL_FORCE, ctx.calc_rear_lat_force));
    
    ctx.rear_torque = -ctx.calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect * ctx.decoupling_scale;
    
    double raw_yaw_accel = data->mLocalRotAccel.y;
    if (ctx.car_speed < 5.0) raw_yaw_accel = 0.0;
    else if (std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) raw_yaw_accel = 0.0;
    
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < 0.0001) tau_yaw = 0.0001;
    double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
    m_yaw_accel_smoothed += alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
    
    ctx.yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK * ctx.decoupling_scale;
    
    ctx.sop_base_force *= ctx.speed_gate;
    ctx.rear_torque *= ctx.speed_gate;
    ctx.yaw_force *= ctx.speed_gate;
}

void FFBEngine::calculate_gyro_damping(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    float range = data->mPhysicalSteeringWheelRange;
    if (range <= 0.0f) range = (float)DEFAULT_STEERING_RANGE_RAD;
    double steer_angle = data->mUnfilteredSteering * (range / 2.0);
    double steer_vel = (steer_angle - m_prev_steering_angle) / ctx.dt;
    m_prev_steering_angle = steer_angle;
    
    double tau_gyro = (double)m_gyro_smoothing;
    if (tau_gyro < 0.0001) tau_gyro = 0.0001;
    double alpha_gyro = ctx.dt / (tau_gyro + ctx.dt);
    m_steering_velocity_smoothed += alpha_gyro * (steer_vel - m_steering_velocity_smoothed);
    
    ctx.gyro_force = -1.0 * m_steering_velocity_smoothed * m_gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE) * ctx.decoupling_scale;
}

void FFBEngine::calculate_abs_pulse(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_abs_pulse_enabled) return;
    
    bool abs_active = false;
    for (int i = 0; i < 4; i++) {
        double pressure_delta = (data->mWheel[i].mBrakePressure - m_prev_brake_pressure[i]) / ctx.dt;
        if (data->mUnfilteredBrake > ABS_PEDAL_THRESHOLD && std::abs(pressure_delta) > ABS_PRESSURE_RATE_THRESHOLD) {
            abs_active = true;
            break;
        }
    }
    
    if (abs_active) {
        m_abs_phase += (double)m_abs_freq_hz * ctx.dt * TWO_PI;
        m_abs_phase = std::fmod(m_abs_phase, TWO_PI);
        ctx.abs_pulse_force = (double)(std::sin(m_abs_phase) * m_abs_gain * 2.0 * ctx.decoupling_scale * ctx.speed_gate);
    }
}

void FFBEngine::calculate_lockup_vibration(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_lockup_enabled) return;
    
    double worst_severity = 0.0;
    double chosen_freq_multiplier = 1.0;
    double chosen_pressure_factor = 0.0;
    
    double slip_fl = calculate_wheel_slip_ratio(data->mWheel[0]);
    double slip_fr = calculate_wheel_slip_ratio(data->mWheel[1]);
    double worst_front = (std::min)(slip_fl, slip_fr);

    for (int i = 0; i < 4; i++) {
        const auto& w = data->mWheel[i];
        double slip = calculate_wheel_slip_ratio(w);
        double slip_abs = std::abs(slip);
        double wheel_accel = (w.mRotation - m_prev_rotation[i]) / ctx.dt;
        double radius = (double)w.mStaticUndeflectedRadius / 100.0;
        if (radius < 0.1) radius = 0.33;
        double car_dec_ang = -std::abs(data->mLocalAccel.z / radius);
        double susp_vel = std::abs(w.mVerticalTireDeflection - m_prev_vert_deflection[i]) / ctx.dt;
        bool is_bumpy = (susp_vel > (double)m_lockup_bump_reject);
        bool brake_active = (data->mUnfilteredBrake > PREDICTION_BRAKE_THRESHOLD);
        bool is_grounded = (w.mSuspForce > PREDICTION_LOAD_THRESHOLD);

        double start_threshold = (double)m_lockup_start_pct / 100.0;
        double full_threshold = (double)m_lockup_full_pct / 100.0;
        double trigger_threshold = full_threshold;

        if (brake_active && is_grounded && !is_bumpy) {
            double sensitivity_threshold = -1.0 * (double)m_lockup_prediction_sens;
            if (wheel_accel < car_dec_ang * 2.0 && wheel_accel < sensitivity_threshold) {
                trigger_threshold = start_threshold;
            }
        }

        if (slip_abs > trigger_threshold) {
            double window = full_threshold - start_threshold;
            if (window < 0.01) window = 0.01;
            double normalized = (slip_abs - start_threshold) / window;
            double severity = (std::min)(1.0, (std::max)(0.0, normalized));
            severity = std::pow(severity, (double)m_lockup_gamma);
            
            double freq_mult = 1.0;
            if (i >= 2) {
                if (slip < (worst_front - AXLE_DIFF_HYSTERESIS)) {
                    freq_mult = LOCKUP_FREQ_MULTIPLIER_REAR;
                }
            }
            double pressure_factor = w.mBrakePressure;
            if (pressure_factor < 0.1 && slip_abs > 0.5) pressure_factor = 0.5;

            if (severity > worst_severity) {
                worst_severity = severity;
                chosen_freq_multiplier = freq_mult;
                chosen_pressure_factor = pressure_factor;
            }
        }
    }

    if (worst_severity > 0.0) {
        double base_freq = 10.0 + (ctx.car_speed * 1.5);
        double final_freq = base_freq * chosen_freq_multiplier * (double)m_lockup_freq_scale;
        m_lockup_phase += final_freq * ctx.dt * TWO_PI;
        m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI);
        double amp = worst_severity * chosen_pressure_factor * m_lockup_gain * (double)BASE_NM_LOCKUP_VIBRATION * ctx.decoupling_scale * ctx.brake_load_factor;
        if (chosen_freq_multiplier < 1.0) amp *= (double)m_lockup_rear_boost;
        ctx.lockup_rumble = std::sin(m_lockup_phase) * amp * ctx.speed_gate;
    }
}

void FFBEngine::calculate_wheel_spin(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (m_spin_enabled && data->mUnfilteredThrottle > 0.05) {
        double slip_rl = calculate_wheel_slip_ratio(data->mWheel[2]);
        double slip_rr = calculate_wheel_slip_ratio(data->mWheel[3]);
        double max_slip = (std::max)(slip_rl, slip_rr);
        
        if (max_slip > 0.2) {
            double severity = (max_slip - 0.2) / 0.5;
            severity = (std::min)(1.0, severity);
            
            ctx.gain_reduction_factor = (1.0 - (severity * m_spin_gain * 0.6));
            
            double slip_speed_ms = ctx.car_speed * max_slip;
            double freq = (10.0 + (slip_speed_ms * 2.5)) * (double)m_spin_freq_scale;
            if (freq > 80.0) freq = 80.0;
            m_spin_phase += freq * ctx.dt * TWO_PI;
            m_spin_phase = std::fmod(m_spin_phase, TWO_PI);
            double amp = severity * m_spin_gain * (double)BASE_NM_SPIN_VIBRATION * ctx.decoupling_scale;
            ctx.spin_rumble = std::sin(m_spin_phase) * amp;
        }
    }
}

void FFBEngine::calculate_slide_texture(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_slide_texture_enabled) return;
    double lat_vel_fl = std::abs(data->mWheel[0].mLateralPatchVel);
    double lat_vel_fr = std::abs(data->mWheel[1].mLateralPatchVel);
    double front_slip_avg = (lat_vel_fl + lat_vel_fr) / 2.0;
    double lat_vel_rl = std::abs(data->mWheel[2].mLateralPatchVel);
    double lat_vel_rr = std::abs(data->mWheel[3].mLateralPatchVel);
    double rear_slip_avg = (lat_vel_rl + lat_vel_rr) / 2.0;
    double effective_slip_vel = (std::max)(front_slip_avg, rear_slip_avg);
    
    if (effective_slip_vel > 1.5) {
        double base_freq = 10.0 + (effective_slip_vel * 5.0);
        double freq = base_freq * (double)m_slide_freq_scale;
        if (freq > 250.0) freq = 250.0;
        m_slide_phase += freq * ctx.dt * TWO_PI;
        m_slide_phase = std::fmod(m_slide_phase, TWO_PI);
        double sawtooth = (m_slide_phase / TWO_PI) * 2.0 - 1.0;
        double grip_scale = (std::max)(0.0, 1.0 - ctx.avg_grip);
        ctx.slide_noise = sawtooth * m_slide_texture_gain * (double)BASE_NM_SLIDE_TEXTURE * ctx.texture_load_factor * grip_scale * ctx.decoupling_scale;
    }
}

void FFBEngine::calculate_road_texture(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (m_scrub_drag_gain > 0.0) {
        double avg_lat_vel = (data->mWheel[0].mLateralPatchVel + data->mWheel[1].mLateralPatchVel) / 2.0;
        double abs_lat_vel = std::abs(avg_lat_vel);
        if (abs_lat_vel > 0.001) {
            double fade = (std::min)(1.0, abs_lat_vel / 0.5);
            double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;
            ctx.scrub_drag_force = drag_dir * m_scrub_drag_gain * (double)BASE_NM_SCRUB_DRAG * fade * ctx.decoupling_scale;
        }
    }

    if (!m_road_texture_enabled) return;
    
    double delta_l = data->mWheel[0].mVerticalTireDeflection - m_prev_vert_deflection[0];
    double delta_r = data->mWheel[1].mVerticalTireDeflection - m_prev_vert_deflection[1];
    delta_l = (std::max)(-0.01, (std::min)(0.01, delta_l));
    delta_r = (std::max)(-0.01, (std::min)(0.01, delta_r));
    
    double road_noise_val = 0.0;
    bool deflection_active = (std::abs(delta_l) > 0.000001 || std::abs(delta_r) > 0.000001);
    
    if (deflection_active || ctx.car_speed < 5.0) {
        road_noise_val = (delta_l + delta_r) * 50.0;
    } else {
        double vert_accel = data->mLocalAccel.y;
        double delta_accel = vert_accel - m_prev_vert_accel;
        road_noise_val = delta_accel * 0.05 * 50.0;
    }
    
    ctx.road_noise = road_noise_val * m_road_texture_gain * ctx.decoupling_scale * ctx.texture_load_factor;
    ctx.road_noise *= ctx.speed_gate;
}

void FFBEngine::calculate_suspension_bottoming(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // Explicit reset to prevent persistent forces if not triggered this frame
    ctx.bottoming_crunch = 0.0;

    if (!m_bottoming_enabled) return;
    bool triggered = false;
    double intensity = 0.0;

    // Use best available load for each wheel (per-wheel fallback)
    double loadL = data->mWheel[0].mTireLoad;
    double loadR = data->mWheel[1].mTireLoad;

    // Reliability Fix: If mTireLoad is missing (0.0 for DLC cars), use the
    // kinematic/approximation fallback already calculated in calculate_force.
    if (loadL < 1.0 && ctx.avg_load > 1.0) loadL = ctx.avg_load;
    if (loadR < 1.0 && ctx.avg_load > 1.0) loadR = ctx.avg_load;
    
    if (m_bottoming_method == 0) {
        // Method A: Scraping (Ride Height based)
        double min_rh = (std::min)(data->mWheel[0].mRideHeight, data->mWheel[1].mRideHeight);
        // Improved Sensitivity: increased threshold from 2mm to 10mm (0.010m)
        if (min_rh < 0.010 && min_rh > -1.0) {
            triggered = true;
            intensity = (0.010 - min_rh) / 0.010;
            intensity = std::clamp(intensity, 0.0, 2.0); // Safety clamp
        }
    } else {
        // Method B: Susp. Spike (Force rate based)
        double suspFL = data->mWheel[0].mSuspForce;
        double suspFR = data->mWheel[1].mSuspForce;

        double dForceL = (suspFL - m_prev_susp_force[0]) / ctx.dt;
        double dForceR = (suspFR - m_prev_susp_force[1]) / ctx.dt;

        // Reliability Fix: If mSuspForce is missing (0.0 for DLC cars),
        // fallback to detecting vertical acceleration jolts.
        if (std::abs(suspFL) < 1.0 && std::abs(suspFR) < 1.0) {
             double dAccelY = (data->mLocalAccel.y - m_prev_vert_accel) / ctx.dt;
             if (std::abs(dAccelY) > 500.0) { // arbitrary threshold for jolt (~50G/s change)
                 triggered = true;
                 intensity = (std::abs(dAccelY) - 500.0) / 1000.0;
                 intensity = std::clamp(intensity, 0.0, 2.0); // Safety clamp
             }
        } else {
            double max_dForce = (std::max)(dForceL, dForceR);
            if (max_dForce > 100000.0) {
                triggered = true;
                intensity = (max_dForce - 100000.0) / 200000.0;
                intensity = std::clamp(intensity, 0.0, 2.0); // Safety clamp
            }
        }
    }

    if (!triggered) {
        // Reliability Fix: Uses per-wheel load (including DLC fallback)
        double max_load = (std::max)(loadL, loadR);
        if (max_load > 8000.0) {
            triggered = true;
            double excess = max_load - 8000.0;
            intensity = std::sqrt(excess) * 0.05;
            intensity = std::clamp(intensity, 0.0, 2.0); // Safety clamp
        }
    }

    if (triggered) {
        // increased BASE_NM_BOTTOMING (5.0) provides a more substantial "thud"
        double bump_magnitude = intensity * m_bottoming_gain * (double)BASE_NM_BOTTOMING * ctx.decoupling_scale;
        double freq = 50.0;
        m_bottoming_phase += freq * ctx.dt * TWO_PI;
        m_bottoming_phase = std::fmod(m_bottoming_phase, TWO_PI);
        ctx.bottoming_crunch = std::sin(m_bottoming_phase) * bump_magnitude * ctx.speed_gate;
    }
}
