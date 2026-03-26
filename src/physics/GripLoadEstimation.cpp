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

namespace LMUFFB {

extern std::recursive_mutex g_engine_mutex;

namespace Physics {

// --- Physics Logic Implementation (Decoupled) ---

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

    double raw_angle = std::atan2(w.mLateralPatchVel, v_long);

    double tau = (double)slip_angle_smoothing;
    if (tau < 0.0001) tau = 0.0001;

    double alpha = dt / (tau + dt);
    alpha = (std::min)(1.0, (std::max)(0.001, alpha));
    prev_state = prev_state + alpha * (raw_angle - prev_state);
    return prev_state;
}

double CalculateManualSlipRatio(const TelemWheelV01& w, double car_speed_ms) {
    if (std::abs(car_speed_ms) < 2.0) return 0.0;
    double radius_m = (double)w.mStaticUndeflectedRadius / 100.0;
    if (radius_m < 0.1) radius_m = 0.33;
    double wheel_vel = w.mRotation * radius_m;
    double denom = std::abs(car_speed_ms);
    if (denom < 1.0) denom = 1.0;
    return (wheel_vel - car_speed_ms) / denom;
}

double CalculateWheelSlipRatio(const TelemWheelV01& w) {
    double v_long = std::abs(w.mLongitudinalGroundVel);
    if (std::abs(v_long) < 0.5) v_long = 0.5;
    return w.mLongitudinalPatchVel / v_long;
}

double CalculateApproximateLoad(const TelemWheelV01& w, ParsedVehicleClass vclass, bool is_rear) {
    double motion_ratio = GetMotionRatioForClass(vclass);
    double unsprung_weight = GetUnsprungWeightForClass(vclass, is_rear);
    return (std::max)(0.0, (w.mSuspForce * motion_ratio) + unsprung_weight);
}

} // namespace Physics

// --- FFBEngine member implementations ---

void FFBEngine::update_static_load_reference(double current_front_load, double current_rear_load, double speed, double dt) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    if (m_static_load_latched) return;

    if (speed > 2.0 && speed < 15.0) {
        if (m_static_front_load < 100.0) {
             m_static_front_load = current_front_load;
        } else {
             m_static_front_load += (dt / 5.0) * (current_front_load - m_static_front_load);
        }
        if (m_static_rear_load < 100.0) {
             m_static_rear_load = current_rear_load;
        } else {
             m_static_rear_load += (dt / 5.0) * (current_rear_load - m_static_rear_load);
        }
    } else if (speed >= 15.0 && m_static_front_load > 1000.0 && m_static_rear_load > 1000.0) {
        m_static_load_latched = true;
        std::string vName = m_metadata.GetVehicleName();
        if (vName != "Unknown" && vName != "") {
            Config::SetSavedStaticLoad(vName, m_static_front_load);
            Config::SetSavedStaticLoad(vName + "_rear", m_static_rear_load);
            Config::m_needs_save = true;
            Logger::Get().LogFile("[FFB] Latched and saved static loads for %s: F=%.2fN, R=%.2fN", vName.c_str(), m_static_front_load, m_static_rear_load);
        }
    }

    if (m_static_front_load < 1000.0) {
        m_static_front_load = m_auto_peak_front_load * 0.5;
    }
    if (m_static_rear_load < 1000.0) {
        m_static_rear_load = m_auto_peak_front_load * 0.5;
    }
}

void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    ResetNormalization();
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
    m_yaw_rate_seeded = false;
    m_yaw_accel_seeded = false;
    m_local_vel_seeded = false;
    m_yaw_rate_log_seeded = false;
    m_derivatives_seeded = false;
    m_slope_buffer_count = 0;
    m_slope_buffer_index = 0;
    m_slope_lat_g_buffer.fill(0.0);
    m_slope_slip_buffer.fill(0.0);
    m_slope_torque_buffer.fill(0.0);
    m_slope_steer_buffer.fill(0.0);
    m_slope_current = 0.0;
    m_slope_smoothed_output = 1.0;

    ParsedVehicleClass vclass = Physics::ParseVehicleClass(className, vehicleName);
    m_auto_peak_front_load = Physics::GetDefaultLoadForClass(vclass);

    std::string vName = vehicleName ? vehicleName : "Unknown";
    double saved_front_load = 0.0;
    double saved_rear_load = 0.0;
    if (Config::GetSavedStaticLoad(vName, saved_front_load)) {
        m_static_front_load = saved_front_load;
        if (Config::GetSavedStaticLoad(vName + "_rear", saved_rear_load)) {
            m_static_rear_load = saved_rear_load;
        } else {
            m_static_rear_load = m_auto_peak_front_load * 0.5;
        }
        m_static_load_latched = true;
        Logger::Get().LogFile("[FFB] Loaded persistent static loads for %s: F=%.2fN, R=%.2fN", vName.c_str(), m_static_front_load, m_static_rear_load);
    } else {
        m_static_front_load = m_auto_peak_front_load * 0.5;
        m_static_rear_load = m_auto_peak_front_load * 0.5;
        m_static_load_latched = false;
        Logger::Get().LogFile("[FFB] No saved load for %s. Learning required.", vName.c_str());
    }
    m_smoothed_vibration_mult = 1.0;
    m_metadata.m_last_handled_vehicle_name = vName;
    m_metadata.UpdateInternal(className, vehicleName, nullptr);
    Logger::Get().LogFile("[FFB] Vehicle Identification -> Detected Class: %s | Seed Load: %.2fN (Raw -> Class: '%s', Name: '%s')",
        Physics::VehicleClassToString(vclass), m_auto_peak_front_load, (className ? className : "Unknown"), vName.c_str());
}

double FFBEngine::calculate_raw_slip_angle_pair(const TelemWheelV01& w1, const TelemWheelV01& w2) {
    return Physics::CalculateRawSlipAnglePair(w1, w2);
}

double FFBEngine::calculate_slip_angle(const TelemWheelV01& w, double& prev_state, double dt) {
    return Physics::CalculateSlipAngle(w, prev_state, dt, m_grip_estimation.slip_angle_smoothing);
}

Physics::GripResult FFBEngine::calculate_axle_grip(const TelemWheelV01& w1,
                          const TelemWheelV01& w2,
                          double avg_axle_load,
                          bool& warned_flag,
                          double& prev_slip1,
                          double& prev_slip2,
                          double& prev_load1,
                          double& prev_load2,
                          double car_speed,
                          double dt,
                          const char* vehicleName,
                          const TelemInfoV01* data,
                          bool is_front) {
    Physics::GripResult result;
    double total_load = w1.mTireLoad + w2.mTireLoad;
    if (total_load > 1.0) {
        result.original = (w1.mGripFract * w1.mTireLoad + w2.mGripFract * w2.mTireLoad) / total_load;
    } else {
        result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
    }
    result.value = result.original;
    result.approximated = false;
    
    double slip1 = calculate_slip_angle(w1, prev_slip1, dt);
    double slip2 = calculate_slip_angle(w2, prev_slip2, dt);
    result.slip_angle = (slip1 + slip2) / 2.0;

    double slope_grip_estimate = 1.0;
    if (is_front && data && car_speed >= 5.0) {
        slope_grip_estimate = calculate_slope_grip(data->mLocalAccel.x / 9.81, result.slip_angle, dt, data);
    }

    if (result.value < 0.0001 && avg_axle_load > 100.0) {
        result.approximated = true;
        if (car_speed < 5.0) {
            result.value = 1.0; 
        } else {
            if (m_slope_detection.enabled && is_front && data) {
                result.value = slope_grip_estimate;
            } else {
                auto calc_wheel_grip = [&](const TelemWheelV01& w, double slip_angle, double& prev_load) {
                    double raw_load = warned_flag ? approximate_load(w) : w.mTireLoad;
                    double load_alpha = dt / (0.050 + dt);
                    prev_load += load_alpha * (raw_load - prev_load);
                    double static_ref = is_front ? m_static_front_load : m_static_rear_load;
                    double load_ratio = std::clamp(prev_load / (static_ref + 1.0), 0.25, 4.0);
                    double dynamic_slip_angle = m_grip_estimation.optimal_slip_angle;
                    if (m_grip_estimation.load_sensitivity_enabled) {
                        dynamic_slip_angle *= std::pow(load_ratio, 0.333);
                    }
                    double lat_metric = std::abs(slip_angle) / dynamic_slip_angle;
                    double long_ratio = calculate_manual_slip_ratio(w, car_speed);
                    double long_metric = std::abs(long_ratio) / (double)m_grip_estimation.optimal_slip_ratio;
                    double combined_slip = std::sqrt((lat_metric * lat_metric) + (long_metric * long_metric));
                    double cs2 = combined_slip * combined_slip;
                    double cs4 = cs2 * cs2;
                    return 0.05 + (0.95 / (1.0 + cs4));
                };
                result.value = (calc_wheel_grip(w1, slip1, prev_load1) + calc_wheel_grip(w2, slip2, prev_load2)) / 2.0;
            }
        }
        result.value = std::clamp(result.value, 0.0, 1.0);
        if (!warned_flag) {
            Logger::Get().LogFile("Warning: Data for mGripFract from the game seems to be missing for this car (%s). (Likely Encrypted/DLC Content). A fallback estimation will be used.", vehicleName);
            warned_flag = true;
        }
    }    

    double& state = is_front ? m_front_grip_smoothed_state : m_rear_grip_smoothed_state;
    result.value = apply_adaptive_smoothing(result.value, state, dt, (double)m_grip_estimation.grip_smoothing_steady, (double)m_grip_estimation.grip_smoothing_fast, (double)m_grip_estimation.grip_smoothing_sensitivity);
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
    const double internal_dt = Physics::PHYSICS_CALC_DT;
    double lat_g_slew = apply_slew_limiter(std::abs(lateral_g), m_slope_lat_g_prev, (double)m_slope_detection.g_slew_limit, dt);
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
    m_slope_lat_g_buffer[m_slope_buffer_index] = m_slope_lat_g_smoothed;
    m_slope_slip_buffer[m_slope_buffer_index] = m_slope_slip_smoothed;
    if (data) {
        m_slope_torque_buffer[m_slope_buffer_index] = m_slope_torque_smoothed;
        m_slope_steer_buffer[m_slope_buffer_index] = m_slope_steer_smoothed;
    }
    m_slope_buffer_index = (m_slope_buffer_index + 1) % SLOPE_BUFFER_MAX;
    if (m_slope_buffer_count < SLOPE_BUFFER_MAX) m_slope_buffer_count++;
    double dG_dt = calculate_sg_derivative(m_slope_lat_g_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);
    double dAlpha_dt = calculate_sg_derivative(m_slope_slip_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);
    m_slope_dG_dt = dG_dt;
    m_slope_dAlpha_dt = dAlpha_dt;
    if (std::abs(dAlpha_dt) > (double)m_slope_detection.alpha_threshold) {
        m_slope_hold_timer = 0.25; // SLOPE_HOLD_TIME
        m_debug_slope_num = dG_dt * dAlpha_dt;
        m_debug_slope_den = (dAlpha_dt * dAlpha_dt) + 0.000001;
        m_debug_slope_raw = m_debug_slope_num / m_debug_slope_den;
        m_slope_current = std::clamp(m_debug_slope_raw, -20.0, 20.0);
    } else {
        m_slope_hold_timer -= dt;
        if (m_slope_hold_timer <= 0.0) {
            m_slope_hold_timer = 0.0;
            m_slope_current += (double)m_slope_detection.decay_rate * dt * (0.0 - m_slope_current);
            if (std::abs(m_slope_current) < 0.0001) m_slope_current = 0.0;
        }
    }
    if (m_slope_detection.use_torque && data != nullptr) {
        double dTorque_dt = calculate_sg_derivative(m_slope_torque_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);
        double dSteer_dt = calculate_sg_derivative(m_slope_steer_buffer, m_slope_buffer_count, m_slope_detection.sg_window, internal_dt, m_slope_buffer_index);
        if (std::abs(dSteer_dt) > (double)m_slope_detection.alpha_threshold) {
            m_debug_slope_torque_num = dTorque_dt * dSteer_dt;
            m_debug_slope_torque_den = (dSteer_dt * dSteer_dt) + 0.000001;
            m_slope_torque_current = std::clamp(m_debug_slope_torque_num / m_debug_slope_torque_den, -50.0, 50.0);
        } else {
            m_slope_torque_current += (double)m_slope_detection.decay_rate * dt * (0.0 - m_slope_torque_current);
        }
    } else {
        m_slope_torque_current = 20.0;
    }
    double confidence = calculate_slope_confidence(dAlpha_dt);
    double loss_percent_g = inverse_lerp((double)m_slope_detection.min_threshold, (double)m_slope_detection.max_threshold, m_slope_current);
    double loss_percent_torque = 0.0;
    if (m_slope_detection.use_torque && data != nullptr && m_slope_torque_current < 0.0) {
        loss_percent_torque = std::abs(m_slope_torque_current) * (double)m_slope_detection.torque_sensitivity;
        loss_percent_torque = (std::max)(0.0, (std::min)(1.0, loss_percent_torque));
    }
    double loss_percent = (std::max)(loss_percent_g, loss_percent_torque);
    double current_grip_factor = 1.0 - (loss_percent * 0.8 * confidence);
    current_grip_factor = (std::max)(0.2, (std::min)(1.0, current_grip_factor));
    double alpha = dt / ((double)m_slope_detection.smoothing_tau + dt);
    alpha = (std::max)(0.001, (std::min)(1.0, alpha));
    m_slope_smoothed_output += alpha * (current_grip_factor - m_slope_smoothed_output);
    return m_slope_smoothed_output;
}

double FFBEngine::calculate_slope_confidence(double dAlpha_dt) {
    if (!m_slope_detection.confidence_enabled) return 1.0;
    return smoothstep((double)m_slope_detection.alpha_threshold, (double)m_slope_detection.confidence_max_rate, std::abs(dAlpha_dt));
}

double FFBEngine::calculate_wheel_slip_ratio(const TelemWheelV01& w) {
    return Physics::CalculateWheelSlipRatio(w);
}

} // namespace LMUFFB
