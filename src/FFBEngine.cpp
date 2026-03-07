#include "FFBEngine.h"
#include "Config.h"
#include "RestApiProvider.h"
#include "Logger.h"
#include "lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include <iostream>
#include <mutex>

extern std::recursive_mutex g_engine_mutex;
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

    // 4. Garage Safety: Mute FFB when in garage stall
    if (scoring.mInGarageStall) return false;

    return true;
}

// v0.7.49: Slew rate limiter for safety (#79)
// Clamps the rate of change of the output force to prevent violent jolts.
// If restricted is true (e.g. after finish or lost control), limit is tighter.
double FFBEngine::ApplySafetySlew(double target_force, double dt, bool restricted) {
    if (!std::isfinite(target_force)) return 0.0;
    double max_slew = restricted ? (double)SAFETY_SLEW_RESTRICTED : (double)SAFETY_SLEW_NORMAL;
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

// ---------------------------------------------------------------------------
// Grip & Load Estimation methods have been moved to GripLoadEstimation.cpp.
// See docs/dev_docs/reports/FFBEngine_refactoring_analysis.md for rationale.
// Functions moved:
//   update_static_load_reference, InitializeLoadReference,
//   calculate_raw_slip_angle_pair, calculate_slip_angle,
//   calculate_grip, approximate_load, approximate_rear_load,
//   calculate_kinematic_load, calculate_manual_slip_ratio,
//   calculate_slope_grip, calculate_slope_confidence,
//   calculate_wheel_slip_ratio
// ---------------------------------------------------------------------------


// Signal Conditioning: Applies idle smoothing and notch filters to raw torque
// Returns the conditioned force value ready for effect processing
double FFBEngine::apply_signal_conditioning(double raw_torque, const TelemInfoV01* data, FFBCalculationContext& ctx) {
    double game_force_proc = raw_torque;

    // Idle Smoothing
    double effective_shaft_smoothing = (double)m_steering_shaft_smoothing;
    double idle_speed_threshold = (double)m_speed_gate_upper;
    if (idle_speed_threshold < (double)IDLE_SPEED_MIN_M_S) idle_speed_threshold = (double)IDLE_SPEED_MIN_M_S;
    if (ctx.car_speed < idle_speed_threshold) {
        double idle_blend = (idle_speed_threshold - ctx.car_speed) / idle_speed_threshold;
        double dynamic_smooth = (double)IDLE_BLEND_FACTOR * idle_blend; 
        effective_shaft_smoothing = (std::max)(effective_shaft_smoothing, dynamic_smooth);
    }
    
    if (effective_shaft_smoothing > MIN_TAU_S) {
        double alpha_shaft = ctx.dt / (effective_shaft_smoothing + ctx.dt);
        alpha_shaft = (std::min)(ALPHA_MAX, (std::max)(ALPHA_MIN, alpha_shaft));
        m_steering_shaft_torque_smoothed += alpha_shaft * (game_force_proc - m_steering_shaft_torque_smoothed);
        game_force_proc = m_steering_shaft_torque_smoothed;
    } else {
        m_steering_shaft_torque_smoothed = game_force_proc;
    }

    // Frequency Estimator Logic
    double alpha_hpf = ctx.dt / (HPF_TIME_CONSTANT_S + ctx.dt);
    m_torque_ac_smoothed += alpha_hpf * (game_force_proc - m_torque_ac_smoothed);
    double ac_torque = game_force_proc - m_torque_ac_smoothed;

    if ((m_prev_ac_torque < -ZERO_CROSSING_EPSILON && ac_torque > ZERO_CROSSING_EPSILON) ||
        (m_prev_ac_torque > ZERO_CROSSING_EPSILON && ac_torque < -ZERO_CROSSING_EPSILON)) {

        double now = data->mElapsedTime;
        double period = now - m_last_crossing_time;

        if (period > MIN_FREQ_PERIOD && period < MAX_FREQ_PERIOD) {
            double inst_freq = 1.0 / (period * DUAL_DIVISOR);
            m_debug_freq = m_debug_freq * DEBUG_FREQ_SMOOTHING + inst_freq * (1.0 - DEBUG_FREQ_SMOOTHING);
        }
        m_last_crossing_time = now;
    }
    m_prev_ac_torque = ac_torque;

    const TelemWheelV01& fl_ref = data->mWheel[0];
    double radius = (double)fl_ref.mStaticUndeflectedRadius / UNIT_CM_TO_M;
    if (radius < RADIUS_FALLBACK_MIN_M) radius = RADIUS_FALLBACK_DEFAULT_M;
    double circumference = TWO_PI * radius;
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
         if (bw < MIN_NOTCH_WIDTH_HZ) bw = MIN_NOTCH_WIDTH_HZ;
         double q = (double)m_static_notch_freq / bw;
         m_static_notch_filter.Update((double)m_static_notch_freq, 1.0/ctx.dt, q);
         game_force_proc = m_static_notch_filter.Process(game_force_proc);
    } else {
         m_static_notch_filter.Reset();
    }

    return game_force_proc;
}

// Refactored calculate_force
double FFBEngine::calculate_force(const TelemInfoV01* data, const char* vehicleClass, const char* vehicleName, float genFFBTorque, bool allowed, double override_dt) {
    if (!data) return 0.0;
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    // --- 0. UP-SAMPLING (Issue #216) ---
    // If override_dt is provided (e.g. from main.cpp), we are in 400Hz upsampling mode.
    // Otherwise (override_dt <= 0), we are in legacy/test mode: every call is a new frame.
    bool upsampling_active = (override_dt > 0.0);
    bool is_new_frame = !upsampling_active || (data->mElapsedTime != m_last_telemetry_time);

    if (is_new_frame) m_last_telemetry_time = data->mElapsedTime;

    double ffb_dt = upsampling_active ? override_dt : (double)data->mDeltaTime;
    if (ffb_dt < 0.0001) ffb_dt = 0.0025;

    // Synchronize persistent working copy
    m_working_info = *data;

    // Upsample Steering Shaft Torque (Holt-Winters)
    double shaft_torque = m_upsample_shaft_torque.Process(data->mSteeringShaftTorque, ffb_dt, is_new_frame);
    m_working_info.mSteeringShaftTorque = shaft_torque;

    // Update wheels in working_info (Channels used for derivatives)
    for (int i = 0; i < 4; i++) {
        m_working_info.mWheel[i].mLateralPatchVel = m_upsample_lat_patch_vel[i].Process(data->mWheel[i].mLateralPatchVel, ffb_dt, is_new_frame);
        m_working_info.mWheel[i].mLongitudinalPatchVel = m_upsample_long_patch_vel[i].Process(data->mWheel[i].mLongitudinalPatchVel, ffb_dt, is_new_frame);
        m_working_info.mWheel[i].mVerticalTireDeflection = m_upsample_vert_deflection[i].Process(data->mWheel[i].mVerticalTireDeflection, ffb_dt, is_new_frame);
        m_working_info.mWheel[i].mSuspForce = m_upsample_susp_force[i].Process(data->mWheel[i].mSuspForce, ffb_dt, is_new_frame);
        m_working_info.mWheel[i].mBrakePressure = m_upsample_brake_pressure[i].Process(data->mWheel[i].mBrakePressure, ffb_dt, is_new_frame);
        m_working_info.mWheel[i].mRotation = m_upsample_rotation[i].Process(data->mWheel[i].mRotation, ffb_dt, is_new_frame);
    }

    // Upsample other derivative sources
    m_working_info.mUnfilteredSteering = m_upsample_steering.Process(data->mUnfilteredSteering, ffb_dt, is_new_frame);
    m_working_info.mUnfilteredThrottle = m_upsample_throttle.Process(data->mUnfilteredThrottle, ffb_dt, is_new_frame);
    m_working_info.mUnfilteredBrake = m_upsample_brake.Process(data->mUnfilteredBrake, ffb_dt, is_new_frame);
    m_working_info.mLocalAccel.x = m_upsample_local_accel_x.Process(data->mLocalAccel.x, ffb_dt, is_new_frame);

    // --- DERIVED ACCELERATION (Issue #278) ---
    // Recalculate acceleration from velocity at 100Hz ticks to avoid raw sensor spikes.
    if (is_new_frame) {
        if (!m_local_vel_seeded) {
            m_prev_local_vel = data->mLocalVel;
            m_local_vel_seeded = true;
        }

        double game_dt = (data->mDeltaTime > 1e-6) ? data->mDeltaTime : 0.01;
        m_derived_accel_y_100hz = (data->mLocalVel.y - m_prev_local_vel.y) / game_dt;
        m_derived_accel_z_100hz = (data->mLocalVel.z - m_prev_local_vel.z) / game_dt;
        m_prev_local_vel = data->mLocalVel;
    }

    m_working_info.mLocalAccel.y = m_upsample_local_accel_y.Process(m_derived_accel_y_100hz, ffb_dt, is_new_frame);
    m_working_info.mLocalAccel.z = m_upsample_local_accel_z.Process(m_derived_accel_z_100hz, ffb_dt, is_new_frame);
    m_working_info.mLocalRotAccel.y = m_upsample_local_rot_accel_y.Process(data->mLocalRotAccel.y, ffb_dt, is_new_frame);
    m_working_info.mLocalRot.y = m_upsample_local_rot_y.Process(data->mLocalRot.y, ffb_dt, is_new_frame);

    // Use upsampled data pointer for all calculations
    const TelemInfoV01* upsampled_data = &m_working_info;

    // Transition Logic: Reset filters when entering "Muted" state (e.g. Garage/AI)
    // to clear out high-frequency residuals from the driving session.
    if (m_was_allowed && !allowed) {
        m_upsample_shaft_torque.Reset();
        m_upsample_steering.Reset();
        m_upsample_throttle.Reset();
        m_upsample_brake.Reset();
        m_upsample_local_accel_x.Reset();
        m_upsample_local_accel_y.Reset();
        m_upsample_local_accel_z.Reset();
        m_upsample_local_rot_accel_y.Reset();
        m_upsample_local_rot_y.Reset();
        for (int i = 0; i < 4; i++) {
            m_upsample_lat_patch_vel[i].Reset();
            m_upsample_long_patch_vel[i].Reset();
            m_upsample_vert_deflection[i].Reset();
            m_upsample_susp_force[i].Reset();
            m_upsample_brake_pressure[i].Reset();
            m_upsample_rotation[i].Reset();
        }
        m_steering_velocity_smoothed = 0.0;
        m_steering_shaft_torque_smoothed = 0.0;
        m_accel_x_smoothed = 0.0;
        m_accel_z_smoothed = 0.0;
        m_sop_lat_g_smoothed = 0.0;
        m_yaw_accel_smoothed = 0.0;
        m_prev_yaw_rate = 0.0;
        m_yaw_rate_seeded = false;
        m_prev_local_vel = {};
        m_local_vel_seeded = false;
    }
    m_was_allowed = allowed;

    // Select Torque Source
    // v0.7.63 Fix: genFFBTorque (Direct Torque 400Hz) is normalized [-1.0, 1.0].
    // It must be scaled by m_wheelbase_max_nm to match the engine's internal Nm-based pipeline.
    double raw_torque_input = (m_torque_source == 1) ? (double)genFFBTorque * (double)m_wheelbase_max_nm : shaft_torque;

    // RELIABILITY FIX: Sanitize input torque
    if (!std::isfinite(raw_torque_input)) return 0.0;

    // --- 0. DYNAMIC NORMALIZATION (Issue #152) ---
    // 1. Contextual Spike Rejection (Lightweight MAD alternative)
    double current_abs_torque = std::abs(raw_torque_input);
    double alpha_slow = ffb_dt / (TORQUE_ROLL_AVG_TAU + ffb_dt); // 1-second rolling average
    m_rolling_average_torque += alpha_slow * (current_abs_torque - m_rolling_average_torque);

    double lat_g_abs = std::abs(upsampled_data->mLocalAccel.x / (double)GRAVITY_MS2);
    double torque_slew = std::abs(raw_torque_input - m_last_raw_torque) / (ffb_dt + (double)EPSILON_DIV);
    m_last_raw_torque = raw_torque_input;

    // Flag as spike if torque jumps > 3x the rolling average (with a 15Nm floor to prevent low-speed false positives)
    bool is_contextual_spike = (current_abs_torque > (m_rolling_average_torque * TORQUE_SPIKE_RATIO)) && (current_abs_torque > TORQUE_SPIKE_MIN_NM);

    // Safety check for clean state
    bool is_clean_state = (lat_g_abs < LAT_G_CLEAN_LIMIT) && (torque_slew < TORQUE_SLEW_CLEAN_LIMIT) && !is_contextual_spike;

    // 2. Leaky Integrator (Exponential Decay + Floor)
    if (is_clean_state && m_torque_source == 0 && m_dynamic_normalization_enabled) {
        if (current_abs_torque > m_session_peak_torque) {
            m_session_peak_torque = current_abs_torque; // Fast attack
        } else {
            // Exponential decay (0.5% reduction per second)
            double decay_factor = 1.0 - ((double)SESSION_PEAK_DECAY_RATE * ffb_dt);
            m_session_peak_torque *= decay_factor;
        }
        // Absolute safety floor and ceiling
        m_session_peak_torque = std::clamp(m_session_peak_torque, (double)PEAK_TORQUE_FLOOR, (double)PEAK_TORQUE_CEILING);
    }

    // 3. EMA Filtering on the Gain Multiplier (Zero-latency physics)
    // v0.7.71: For In-Game FFB (1), we normalize against the wheelbase max since the signal is already normalized [-1, 1].
    double target_structural_mult;
    if (m_torque_source == 1) {
        target_structural_mult = 1.0 / ((double)m_wheelbase_max_nm + (double)EPSILON_DIV);
    } else if (m_dynamic_normalization_enabled) {
        target_structural_mult = 1.0 / (m_session_peak_torque + (double)EPSILON_DIV);
    } else {
        target_structural_mult = 1.0 / ((double)m_target_rim_nm + (double)EPSILON_DIV);
    }
    double alpha_gain = ffb_dt / ((double)STRUCT_MULT_SMOOTHING_TAU + ffb_dt); // 250ms smoothing
    m_smoothed_structural_mult += alpha_gain * (target_structural_mult - m_smoothed_structural_mult);

    // Class Seeding
    bool seeded = UpdateMetadataInternal(vehicleClass, vehicleName, data->mTrackName);

    // Trigger REST API Fallback if enabled and range is invalid (Issue #221)
    if (seeded && m_rest_api_enabled && data->mPhysicalSteeringWheelRange <= 0.0f) {
        RestApiProvider::Get().RequestSteeringRange(m_rest_api_port);
    }
    
    // --- 1. INITIALIZE CONTEXT ---
    FFBCalculationContext ctx;
    ctx.dt = ffb_dt; // Use our constant FFB loop time for all internal physics

    // Sanity Check: Delta Time (Keep legacy warning if raw dt is broken)
    if (data->mDeltaTime <= DT_EPSILON) {
        if (!m_warned_dt) {
            Logger::Get().LogFile("[WARNING] Invalid DeltaTime (<=0). Using default %.4fs.", DEFAULT_DT);
            m_warned_dt = true;
        }
        ctx.frame_warn_dt = true;
    }
    
    ctx.car_speed_long = upsampled_data->mLocalVel.z;
    ctx.car_speed = std::abs(ctx.car_speed_long);
    
    // Steering Range Diagnostic (Issue #218)
    if (upsampled_data->mPhysicalSteeringWheelRange <= 0.0f) {
        if (!m_warned_invalid_range) {
            float fallback = RestApiProvider::Get().GetFallbackRangeDeg();
            if (m_rest_api_enabled && fallback > 0.0f) {
                Logger::Get().LogFile("[FFB] Invalid Shared Memory Steering Range. Using REST API fallback: %.1f deg", fallback);
            } else {
                Logger::Get().LogFile("[WARNING] Invalid PhysicalSteeringWheelRange (<=0) for %s. Soft Lock and Steering UI may be incorrect.", upsampled_data->mVehicleName);
            }
            m_warned_invalid_range = true;
        }
    }

    // --- 2. SIGNAL CONDITIONING (STATE UPDATES) ---
    
    // Chassis Inertia Simulation
    double chassis_tau = (double)m_chassis_inertia_smoothing;
    if (chassis_tau < MIN_TAU_S) chassis_tau = MIN_TAU_S;
    double alpha_chassis = ctx.dt / (chassis_tau + ctx.dt);
    m_accel_x_smoothed += alpha_chassis * (upsampled_data->mLocalAccel.x - m_accel_x_smoothed);
    m_accel_z_smoothed += alpha_chassis * (upsampled_data->mLocalAccel.z - m_accel_z_smoothed);

    // --- 3. TELEMETRY PROCESSING ---
    // Front Wheels
    const TelemWheelV01& fl = upsampled_data->mWheel[0];
    const TelemWheelV01& fr = upsampled_data->mWheel[1];

    // Raw Inputs
    double raw_torque = raw_torque_input;
    double raw_load = (fl.mTireLoad + fr.mTireLoad) / DUAL_DIVISOR;
    double raw_grip = (fl.mGripFract + fr.mGripFract) / DUAL_DIVISOR;

    // Update Stats
    s_torque.Update(raw_torque);
    s_load.Update(raw_load);
    s_grip.Update(raw_grip);
    s_lat_g.Update(upsampled_data->mLocalAccel.x);
    
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

    // Average Front Axle Load & Fallback Logic
    ctx.avg_load_front = raw_load;

    // Hysteresis for missing front load
    // v0.7.150: Reduce threshold and add aggressive zero-check for high speed (Issue #282)
    bool is_load_missing = (ctx.avg_load_front < 1.0);
    if (is_load_missing && ctx.car_speed > SPEED_EPSILON) {
        m_missing_load_front_frames++;
        // Immediate pivot if we are moving fast and load is exactly 0
        if (ctx.car_speed > SPEED_HIGH_THRESHOLD && ctx.avg_load_front < 0.001) {
            m_missing_load_front_frames = MISSING_LOAD_WARN_THRESHOLD + 1;
        }
    } else {
        m_missing_load_front_frames = (std::max)(0, m_missing_load_front_frames - 1);
    }

    if (m_missing_load_front_frames > MISSING_LOAD_WARN_THRESHOLD) {
        // Fallback Logic
        if (fl.mSuspForce > MIN_VALID_SUSP_FORCE) {
            double calc_load_fl = approximate_load(fl);
            double calc_load_fr = approximate_load(fr);
            ctx.avg_load_front = (calc_load_fl + calc_load_fr) / DUAL_DIVISOR;
        } else {
            double kin_load_fl = calculate_kinematic_load(data, 0);
            double kin_load_fr = calculate_kinematic_load(data, 1);
            ctx.avg_load_front = (kin_load_fl + kin_load_fr) / DUAL_DIVISOR;
        }
        if (!m_warned_load) {
            Logger::Get().LogFile("Warning: Data for mTireLoad from the game seems to be missing for this car (%s). (Likely Encrypted/DLC Content). Using Kinematic Fallback.", data->mVehicleName);
            m_warned_load = true;
        }
        ctx.frame_warn_load = true;
    }

    // Sanity Checks (Missing Data)
    
    // 1. Suspension Force (mSuspForce)
    double avg_susp_f = (fl.mSuspForce + fr.mSuspForce) / DUAL_DIVISOR;
    if (avg_susp_f < MIN_VALID_SUSP_FORCE && std::abs(data->mLocalVel.z) > SPEED_EPSILON) {
        m_missing_susp_force_frames++;
    } else {
         m_missing_susp_force_frames = (std::max)(0, m_missing_susp_force_frames - 1);
    }
    if (m_missing_susp_force_frames > MISSING_TELEMETRY_WARN_THRESHOLD && !m_warned_susp_force) {
         Logger::Get().LogFile("Warning: Data for mSuspForce from the game seems to be missing for this car (%s). (Likely Encrypted/DLC Content). A fallback estimation will be used.", data->mVehicleName);
         m_warned_susp_force = true;
    }

    // 2. Suspension Deflection (mSuspensionDeflection)
    double avg_susp_def = (std::abs(fl.mSuspensionDeflection) + std::abs(fr.mSuspensionDeflection)) / DUAL_DIVISOR;
    if (avg_susp_def < DEFLECTION_NEAR_ZERO_M && std::abs(data->mLocalVel.z) > SPEED_HIGH_THRESHOLD) {
        m_missing_susp_deflection_frames++;
    } else {
        m_missing_susp_deflection_frames = (std::max)(0, m_missing_susp_deflection_frames - 1);
    }
    if (m_missing_susp_deflection_frames > MISSING_TELEMETRY_WARN_THRESHOLD && !m_warned_susp_deflection) {
        Logger::Get().LogFile("Warning: Data for mSuspensionDeflection from the game seems to be missing for this car (%s). (Likely Encrypted/DLC Content). A fallback estimation will be used.", data->mVehicleName);
        m_warned_susp_deflection = true;
    }

    // 3. Front Lateral Force (mLateralForce)
    double avg_lat_force_front = (std::abs(fl.mLateralForce) + std::abs(fr.mLateralForce)) / DUAL_DIVISOR;
    if (avg_lat_force_front < MIN_VALID_LAT_FORCE_N && std::abs(data->mLocalAccel.x) > G_FORCE_THRESHOLD) {
        m_missing_lat_force_front_frames++;
    } else {
        m_missing_lat_force_front_frames = (std::max)(0, m_missing_lat_force_front_frames - 1);
    }
    if (m_missing_lat_force_front_frames > MISSING_TELEMETRY_WARN_THRESHOLD && !m_warned_lat_force_front) {
         Logger::Get().LogFile("Warning: Data for mLateralForce (Front) from the game seems to be missing for this car (%s). (Likely Encrypted/DLC Content). A fallback estimation will be used.", data->mVehicleName);
         m_warned_lat_force_front = true;
    }

    // 4. Rear Lateral Force (mLateralForce)
    double avg_lat_force_rear = (std::abs(data->mWheel[2].mLateralForce) + std::abs(data->mWheel[3].mLateralForce)) / DUAL_DIVISOR;
    if (avg_lat_force_rear < MIN_VALID_LAT_FORCE_N && std::abs(data->mLocalAccel.x) > G_FORCE_THRESHOLD) {
        m_missing_lat_force_rear_frames++;
    } else {
        m_missing_lat_force_rear_frames = (std::max)(0, m_missing_lat_force_rear_frames - 1);
    }
    if (m_missing_lat_force_rear_frames > MISSING_TELEMETRY_WARN_THRESHOLD && !m_warned_lat_force_rear) {
         Logger::Get().LogFile("Warning: Data for mLateralForce (Rear) from the game seems to be missing for this car (%s). (Likely Encrypted/DLC Content). A fallback estimation will be used.", data->mVehicleName);
         m_warned_lat_force_rear = true;
    }

    // 5. Vertical Tire Deflection (mVerticalTireDeflection)
    double avg_vert_def = (std::abs(fl.mVerticalTireDeflection) + std::abs(fr.mVerticalTireDeflection)) / DUAL_DIVISOR;
    if (avg_vert_def < DEFLECTION_NEAR_ZERO_M && std::abs(data->mLocalVel.z) > SPEED_HIGH_THRESHOLD) {
        m_missing_vert_deflection_frames++;
    } else {
        m_missing_vert_deflection_frames = (std::max)(0, m_missing_vert_deflection_frames - 1);
    }
    if (m_missing_vert_deflection_frames > MISSING_TELEMETRY_WARN_THRESHOLD && !m_warned_vert_deflection) {
        Logger::Get().LogFile("[WARNING] mVerticalTireDeflection is missing for car: %s. (Likely Encrypted/DLC Content). Road Texture fallback active.", data->mVehicleName);
        m_warned_vert_deflection = true;
    }
    
    // Peak Hold Logic (Front Axle)
    if (m_auto_load_normalization_enabled && !seeded) {
        if (ctx.avg_load_front > m_auto_peak_load) {
            m_auto_peak_load = ctx.avg_load_front; // Fast Attack
        } else {
            m_auto_peak_load -= (LOAD_DECAY_RATE * ctx.dt); // Slow Decay (~100N/s)
        }
    }
    m_auto_peak_load = (std::max)(LOAD_SAFETY_FLOOR, m_auto_peak_load); // Safety Floor

    // Load Factors (Stage 3: Giannoulis Soft-Knee Compression)
    // 1. Calculate raw load multiplier based on static front weight
    // Safety clamp: Ensure load factor is non-negative even with telemetry noise
    double x = (std::max)(0.0, ctx.avg_load_front / m_static_front_load);

    // 2. Giannoulis Soft-Knee Parameters
    double T = COMPRESSION_KNEE_THRESHOLD;  // Threshold (Start compressing at 1.5x static weight)
    double W = COMPRESSION_KNEE_WIDTH;  // Knee Width (Transition from 1.25x to 1.75x)
    double R = COMPRESSION_RATIO;  // Compression Ratio (4:1 above the knee)

    double lower_bound = T - (W / DUAL_DIVISOR);
    double upper_bound = T + (W / DUAL_DIVISOR);
    double compressed_load_factor = x;

    // 3. Apply Compression
    if (x > upper_bound) {
        // Linear compressed region
        compressed_load_factor = T + ((x - T) / R);
    } else if (x > lower_bound) {
        // Quadratic soft-knee transition
        double diff = x - lower_bound;
        compressed_load_factor = x + (((1.0 / R) - 1.0) * (diff * diff)) / (DUAL_DIVISOR * W);
    }

    // 4. EMA Smoothing on the vibration multiplier (100ms time constant)
    double alpha_vibration = ctx.dt / (VIBRATION_EMA_TAU + ctx.dt);
    m_smoothed_vibration_mult += alpha_vibration * (compressed_load_factor - m_smoothed_vibration_mult);

    // 5. Apply to context with user caps
    double texture_safe_max = (std::min)(USER_CAP_MAX, (double)m_texture_load_cap);
    ctx.texture_load_factor = (std::min)(texture_safe_max, m_smoothed_vibration_mult);

    double brake_safe_max = (std::min)(USER_CAP_MAX, (double)m_brake_load_cap);
    ctx.brake_load_factor = (std::min)(brake_safe_max, m_smoothed_vibration_mult);
    
    // Hardware Scaling Safeties
    double wheelbase_max_safe = (double)m_wheelbase_max_nm;
    if (wheelbase_max_safe < 1.0) wheelbase_max_safe = 1.0;

    // Speed Gate - v0.7.2 Smoothstep S-curve
    ctx.speed_gate = smoothstep(
        (double)m_speed_gate_lower, 
        (double)m_speed_gate_upper, 
        ctx.car_speed
    );

    // --- 5. EFFECT CALCULATIONS ---

    // A. Understeer (Base Torque + Grip Loss)

    // Front Grip Estimation (v0.4.5 FIX)
    GripResult front_grip_res = calculate_grip(fl, fr, ctx.avg_load_front, m_warned_grip,
                                                m_prev_slip_angle[0], m_prev_slip_angle[1],
                                                ctx.car_speed, ctx.dt, data->mVehicleName, data, true /* is_front */);
    ctx.avg_grip_front = front_grip_res.value;
    m_grip_diag.front_original = front_grip_res.original;
    m_grip_diag.front_approximated = front_grip_res.approximated;
    m_grip_diag.front_slip_angle = front_grip_res.slip_angle;
    if (front_grip_res.approximated) ctx.frame_warn_grip = true;

    // 2. Signal Conditioning (Smoothing, Notch Filters)
    double game_force_proc = apply_signal_conditioning(raw_torque_input, upsampled_data, ctx);

    // Base Steering Force (Issue #178)
    double base_input = game_force_proc;
    
    // Apply Front Grip Modulation
    double grip_loss = (1.0 - ctx.avg_grip_front) * m_understeer_effect;
    ctx.grip_factor = (std::max)(0.0, 1.0 - grip_loss);

    // v0.7.63: Passthrough Logic for Direct Torque (TIC mode)
    double grip_factor_applied = m_torque_passthrough ? 1.0 : ctx.grip_factor;

    // v0.7.46: Dynamic Weight logic (Front Axle)
    if (m_auto_load_normalization_enabled) {
        update_static_load_reference(ctx.avg_load_front, ctx.car_speed, ctx.dt);
    }
    double dynamic_weight_factor = 1.0;

    // Only apply if enabled AND we have real front load data (no warnings)
    if (m_dynamic_weight_gain > 0.0 && !ctx.frame_warn_load) {
        double load_ratio = ctx.avg_load_front / m_static_front_load;
        // Blend: 1.0 + (Ratio - 1.0) * Gain
        dynamic_weight_factor = 1.0 + (load_ratio - 1.0) * (double)m_dynamic_weight_gain;
        dynamic_weight_factor = std::clamp(dynamic_weight_factor, DYNAMIC_WEIGHT_MIN, DYNAMIC_WEIGHT_MAX);
    }

    // Apply Smoothing to Dynamic Weight (v0.7.47)
    double dw_alpha = ctx.dt / ((double)m_dynamic_weight_smoothing + ctx.dt + EPSILON_DIV);
    dw_alpha = (std::max)(0.0, (std::min)(1.0, dw_alpha));
    m_dynamic_weight_smoothed += dw_alpha * (dynamic_weight_factor - m_dynamic_weight_smoothed);
    dynamic_weight_factor = m_dynamic_weight_smoothed;

    // v0.7.63: Final factor application
    double dw_factor_applied = m_torque_passthrough ? 1.0 : dynamic_weight_factor;
    
    double gain_to_apply = (m_torque_source == 1) ? (double)m_ingame_ffb_gain : (double)m_steering_shaft_gain;
    double output_force = (base_input * gain_to_apply) * dw_factor_applied * grip_factor_applied;
    output_force *= ctx.speed_gate;
    
    // B. SoP Lateral (Oversteer)
    calculate_sop_lateral(upsampled_data, ctx);
    
    // C. Gyro Damping
    calculate_gyro_damping(upsampled_data, ctx);
    
    // D. Effects
    calculate_abs_pulse(upsampled_data, ctx);
    calculate_lockup_vibration(upsampled_data, ctx);
    calculate_wheel_spin(upsampled_data, ctx);
    calculate_slide_texture(upsampled_data, ctx);
    calculate_road_texture(upsampled_data, ctx);
    calculate_suspension_bottoming(upsampled_data, ctx);
    calculate_soft_lock(upsampled_data, ctx);

    // v0.7.78 FIX: Support stationary/garage soft lock (Issue #184)
    // If not allowed (e.g. in garage or AI driving), mute all forces EXCEPT Soft Lock.
    if (!allowed) {
        output_force = 0.0;
        ctx.sop_base_force = 0.0;
        ctx.rear_torque = 0.0;
        ctx.yaw_force = 0.0;
        ctx.gyro_force = 0.0;
        ctx.scrub_drag_force = 0.0;
        ctx.road_noise = 0.0;
        ctx.slide_noise = 0.0;
        ctx.spin_rumble = 0.0;
        ctx.bottoming_crunch = 0.0;
        ctx.abs_pulse_force = 0.0;
        ctx.lockup_rumble = 0.0;
        // NOTE: ctx.soft_lock_force is PRESERVED.

        // Also zero out base_input for snapshot clarity
        base_input = 0.0;
    }
    
    // --- 6. SUMMATION (Issue #152 & #153 Split Scaling) ---
    // Split into Structural (Dynamic Normalization) and Texture (Absolute Nm) groups
    // v0.7.77 FIX: Soft Lock moved to Texture group to maintain absolute Nm scaling (Issue #181)
    double structural_sum = output_force + ctx.sop_base_force + ctx.rear_torque + ctx.yaw_force + ctx.gyro_force +
                            ctx.scrub_drag_force;

    // Apply Torque Drop (from Spin/Traction Loss) only to structural physics
    structural_sum *= ctx.gain_reduction_factor;
    
    // Apply Dynamic Normalization to structural forces
    double norm_structural = structural_sum * m_smoothed_structural_mult;

    // Vibration Effects are calculated in absolute Nm
    // v0.7.110: Apply m_vibration_gain to textures, but NOT to Soft Lock (Issue #206)
    double vibration_sum_nm = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch + ctx.abs_pulse_force + ctx.lockup_rumble;
    double final_texture_nm = (vibration_sum_nm * (double)m_vibration_gain) + ctx.soft_lock_force;

    // --- 7. OUTPUT SCALING (Physical Target Model) ---
    // Map structural to the target rim torque, then divide by wheelbase max to get DirectInput %
    double di_structural = norm_structural * ((double)m_target_rim_nm / wheelbase_max_safe);

    // Map absolute texture Nm directly to the wheelbase max
    double di_texture = final_texture_nm / wheelbase_max_safe;

    double norm_force = (di_structural + di_texture) * m_gain;

    // Min Force
    // v0.7.85 FIX: Bypass min_force if NOT allowed (e.g. in garage) unless soft lock is significant.
    // This prevents the "grinding" feel from tiny residuals when FFB should be muted.
    // v0.7.118: Tighten gate to require BOTH steering beyond limit AND significant force.
    bool significant_soft_lock = (std::abs(upsampled_data->mUnfilteredSteering) > 1.0) &&
                                 (std::abs(ctx.soft_lock_force) > 0.5); // > 0.5 Nm

    if (allowed || significant_soft_lock) {
        if (std::abs(norm_force) > FFB_EPSILON && std::abs(norm_force) < m_min_force) {
            double sign = (norm_force > 0.0) ? 1.0 : -1.0;
            norm_force = sign * m_min_force;
        }
    }

    if (m_invert_force) {
        norm_force *= -1.0;
    }

    // --- 8. STATE UPDATES (POST-CALC) ---
    // CRITICAL: These updates must run UNCONDITIONALLY every frame to prevent
    // stale state issues when effects are toggled on/off.
    // v0.7.116: Use upsampled_data to ensure derivatives (current - prev) / dt
    // are calculated correctly over the 400Hz 2.5ms interval.
    for (int i = 0; i < 4; i++) {
        m_prev_vert_deflection[i] = upsampled_data->mWheel[i].mVerticalTireDeflection;
        m_prev_rotation[i] = upsampled_data->mWheel[i].mRotation;
        m_prev_brake_pressure[i] = upsampled_data->mWheel[i].mBrakePressure;
    }
    m_prev_susp_force[0] = upsampled_data->mWheel[0].mSuspForce;
    m_prev_susp_force[1] = upsampled_data->mWheel[1].mSuspForce;
    
    // v0.6.36 FIX: Move m_prev_vert_accel to unconditional section
    // Previously only updated inside calculate_road_texture when enabled.
    // Now always updated to prevent stale data if other effects use it.
    // v0.7.145 (Issue #278): Use upsampled derived acceleration for smoother Jerk calculation.
    m_prev_vert_accel = upsampled_data->mLocalAccel.y;

    // --- 9. DERIVE LOGGABLE DIAGNOSTICS ---
    float sm_range_rad = data->mPhysicalSteeringWheelRange;
    float range_deg = sm_range_rad * (180.0f / (float)PI);

    // Fallback to REST API if enabled and SM range is invalid (Issue #221)
    if (m_rest_api_enabled && sm_range_rad <= 0.0f) {
        float fallback = RestApiProvider::Get().GetFallbackRangeDeg();
        if (fallback > 0.0f) {
            range_deg = fallback;
        }
    }

    float steering_angle_deg = (float)data->mUnfilteredSteering * (range_deg / 2.0f);
    float understeer_drop = (float)((base_input * m_steering_shaft_gain) * (1.0 - grip_factor_applied));
    float oversteer_boost = (float)(ctx.sop_base_force - ctx.sop_unboosted_force); // Exact boost amount

    // --- 10. SNAPSHOT ---
    // This block captures the current state of the FFB Engine (inputs, outputs, intermediate calculations)
    // into a thread-safe buffer. These snapshots are retrieved by the GUI layer (or other consumers)
    // to visualize real-time telemetry graphs, FFB clipping, and effect contributions.
    // It uses a mutex to protect the shared circular buffer.
    {
        std::lock_guard<std::mutex> lock(m_debug_mutex);
        if (m_debug_buffer.size() < DEBUG_BUFFER_CAP) {
            FFBSnapshot snap;
            snap.total_output = (float)norm_force;
            snap.base_force = (float)base_input;
            snap.sop_force = (float)ctx.sop_unboosted_force; // Use unboosted for snapshot
            snap.understeer_drop = understeer_drop;
            snap.oversteer_boost = oversteer_boost;

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
            snap.session_peak_torque = (float)m_session_peak_torque;
            snap.clipping = (std::abs(norm_force) > (double)CLIPPING_THRESHOLD) ? 1.0f : 0.0f;

            // Physics
            snap.calc_front_load = (float)ctx.avg_load_front;
            snap.calc_rear_load = (float)ctx.avg_rear_load;
            snap.calc_rear_lat_force = (float)ctx.calc_rear_lat_force;
            snap.calc_front_grip = (float)ctx.avg_grip_front;
            snap.calc_rear_grip = (float)ctx.avg_rear_grip;
            snap.calc_front_slip_angle_smoothed = (float)m_grip_diag.front_slip_angle;
            snap.calc_rear_slip_angle_smoothed = (float)m_grip_diag.rear_slip_angle;

            snap.raw_front_slip_angle = (float)calculate_raw_slip_angle_pair(fl, fr);
            snap.raw_rear_slip_angle = (float)calculate_raw_slip_angle_pair(data->mWheel[2], data->mWheel[3]);

            // Telemetry
            snap.steer_force = (float)raw_torque;
            snap.raw_shaft_torque = (float)data->mSteeringShaftTorque;
            snap.raw_gen_torque = (float)genFFBTorque;
            snap.raw_input_steering = (float)data->mUnfilteredSteering;
            snap.raw_front_tire_load = (float)raw_load;
            snap.raw_front_grip_fract = (float)raw_grip;
            snap.raw_rear_grip = (float)((data->mWheel[2].mGripFract + data->mWheel[3].mGripFract) / DUAL_DIVISOR);
            snap.raw_front_susp_force = (float)((fl.mSuspForce + fr.mSuspForce) / DUAL_DIVISOR);
            snap.raw_front_ride_height = (float)((std::min)(fl.mRideHeight, fr.mRideHeight));
            snap.raw_rear_lat_force = (float)((data->mWheel[2].mLateralForce + data->mWheel[3].mLateralForce) / DUAL_DIVISOR);
            snap.raw_car_speed = (float)ctx.car_speed_long;
            snap.raw_input_throttle = (float)data->mUnfilteredThrottle;
            snap.raw_input_brake = (float)data->mUnfilteredBrake;
            snap.accel_x = (float)data->mLocalAccel.x;
            snap.raw_front_lat_patch_vel = (float)((std::abs(fl.mLateralPatchVel) + std::abs(fr.mLateralPatchVel)) / DUAL_DIVISOR);
            snap.raw_front_deflection = (float)((fl.mVerticalTireDeflection + fr.mVerticalTireDeflection) / DUAL_DIVISOR);
            snap.raw_front_long_patch_vel = (float)((fl.mLongitudinalPatchVel + fr.mLongitudinalPatchVel) / DUAL_DIVISOR);
            snap.raw_rear_lat_patch_vel = (float)((std::abs(data->mWheel[2].mLateralPatchVel) + std::abs(data->mWheel[3].mLateralPatchVel)) / DUAL_DIVISOR);
            snap.raw_rear_long_patch_vel = (float)((data->mWheel[2].mLongitudinalPatchVel + data->mWheel[3].mLongitudinalPatchVel) / DUAL_DIVISOR);

            snap.steering_range_deg = range_deg;
            snap.steering_angle_deg = steering_angle_deg;

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
            snap.physics_rate = (float)m_physics_rate;

            m_debug_buffer.push_back(snap);
        }
    }
    
    // Telemetry Logging (v0.7.129: Augmented Binary & 400Hz)
    if (AsyncLogger::Get().IsLogging()) {
        LogFrame frame;
        frame.timestamp = upsampled_data->mElapsedTime;
        frame.delta_time = ctx.dt;
        
        // --- PROCESSED 400Hz DATA (Smooth) ---
        frame.speed = (float)ctx.car_speed;
        frame.lat_accel = (float)upsampled_data->mLocalAccel.x;
        frame.long_accel = (float)upsampled_data->mLocalAccel.z;
        frame.yaw_rate = (float)upsampled_data->mLocalRot.y;

        frame.steering = (float)upsampled_data->mUnfilteredSteering;
        frame.throttle = (float)upsampled_data->mUnfilteredThrottle;
        frame.brake = (float)upsampled_data->mUnfilteredBrake;

        // --- RAW 100Hz GAME DATA (Step-function) ---
        frame.raw_steering = (float)data->mUnfilteredSteering;
        frame.raw_throttle = (float)data->mUnfilteredThrottle;
        frame.raw_brake = (float)data->mUnfilteredBrake;
        frame.raw_lat_accel = (float)data->mLocalAccel.x;
        frame.raw_long_accel = (float)data->mLocalAccel.z;
        frame.raw_game_yaw_accel = (float)data->mLocalRotAccel.y;
        frame.raw_game_shaft_torque = (float)data->mSteeringShaftTorque;
        frame.raw_game_gen_torque = (float)genFFBTorque;

        frame.raw_load_fl = (float)data->mWheel[0].mTireLoad;
        frame.raw_load_fr = (float)data->mWheel[1].mTireLoad;
        frame.raw_load_rl = (float)data->mWheel[2].mTireLoad;
        frame.raw_load_rr = (float)data->mWheel[3].mTireLoad;

        frame.raw_slip_vel_lat_fl = (float)data->mWheel[0].mLateralPatchVel;
        frame.raw_slip_vel_lat_fr = (float)data->mWheel[1].mLateralPatchVel;
        frame.raw_slip_vel_lat_rl = (float)data->mWheel[2].mLateralPatchVel;
        frame.raw_slip_vel_lat_rr = (float)data->mWheel[3].mLateralPatchVel;

        frame.raw_slip_vel_long_fl = (float)data->mWheel[0].mLongitudinalPatchVel;
        frame.raw_slip_vel_long_fr = (float)data->mWheel[1].mLongitudinalPatchVel;
        frame.raw_slip_vel_long_rl = (float)data->mWheel[2].mLongitudinalPatchVel;
        frame.raw_slip_vel_long_rr = (float)data->mWheel[3].mLongitudinalPatchVel;

        frame.raw_ride_height_fl = (float)data->mWheel[0].mRideHeight;
        frame.raw_ride_height_fr = (float)data->mWheel[1].mRideHeight;
        frame.raw_ride_height_rl = (float)data->mWheel[2].mRideHeight;
        frame.raw_ride_height_rr = (float)data->mWheel[3].mRideHeight;

        frame.raw_susp_deflection_fl = (float)data->mWheel[0].mSuspensionDeflection;
        frame.raw_susp_deflection_fr = (float)data->mWheel[1].mSuspensionDeflection;
        frame.raw_susp_deflection_rl = (float)data->mWheel[2].mSuspensionDeflection;
        frame.raw_susp_deflection_rr = (float)data->mWheel[3].mSuspensionDeflection;

        frame.raw_susp_force_fl = (float)data->mWheel[0].mSuspForce;
        frame.raw_susp_force_fr = (float)data->mWheel[1].mSuspForce;
        frame.raw_susp_force_rl = (float)data->mWheel[2].mSuspForce;
        frame.raw_susp_force_rr = (float)data->mWheel[3].mSuspForce;

        frame.raw_brake_pressure_fl = (float)data->mWheel[0].mBrakePressure;
        frame.raw_brake_pressure_fr = (float)data->mWheel[1].mBrakePressure;
        frame.raw_brake_pressure_rl = (float)data->mWheel[2].mBrakePressure;
        frame.raw_brake_pressure_rr = (float)data->mWheel[3].mBrakePressure;

        frame.raw_rotation_fl = (float)data->mWheel[0].mRotation;
        frame.raw_rotation_fr = (float)data->mWheel[1].mRotation;
        frame.raw_rotation_rl = (float)data->mWheel[2].mRotation;
        frame.raw_rotation_rr = (float)data->mWheel[3].mRotation;

        // --- ALGORITHM STATE (400Hz) ---
        frame.slip_angle_fl = (float)fl.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
        frame.slip_angle_fr = (float)fr.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
        frame.slip_angle_rl = (float)upsampled_data->mWheel[2].mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
        frame.slip_angle_rr = (float)upsampled_data->mWheel[3].mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);

        frame.slip_ratio_fl = (float)calculate_wheel_slip_ratio(fl);
        frame.slip_ratio_fr = (float)calculate_wheel_slip_ratio(fr);
        frame.slip_ratio_rl = (float)calculate_wheel_slip_ratio(upsampled_data->mWheel[2]);
        frame.slip_ratio_rr = (float)calculate_wheel_slip_ratio(upsampled_data->mWheel[3]);

        frame.grip_fl = (float)fl.mGripFract;
        frame.grip_fr = (float)fr.mGripFract;
        frame.grip_rl = (float)upsampled_data->mWheel[2].mGripFract;
        frame.grip_rr = (float)upsampled_data->mWheel[3].mGripFract;

        frame.load_fl = (float)fl.mTireLoad;
        frame.load_fr = (float)fr.mTireLoad;
        frame.load_rl = (float)upsampled_data->mWheel[2].mTireLoad;
        frame.load_rr = (float)upsampled_data->mWheel[3].mTireLoad;

        frame.ride_height_fl = (float)fl.mRideHeight;
        frame.ride_height_fr = (float)fr.mRideHeight;
        frame.ride_height_rl = (float)upsampled_data->mWheel[2].mRideHeight;
        frame.ride_height_rr = (float)upsampled_data->mWheel[3].mRideHeight;

        frame.susp_deflection_fl = (float)fl.mSuspensionDeflection;
        frame.susp_deflection_fr = (float)fr.mSuspensionDeflection;
        frame.susp_deflection_rl = (float)upsampled_data->mWheel[2].mSuspensionDeflection;
        frame.susp_deflection_rr = (float)upsampled_data->mWheel[3].mSuspensionDeflection;
        
        frame.calc_slip_angle_front = (float)m_grip_diag.front_slip_angle;
        frame.calc_slip_angle_rear = (float)m_grip_diag.rear_slip_angle;
        frame.calc_grip_front = (float)ctx.avg_grip_front;
        frame.calc_grip_rear = (float)ctx.avg_rear_grip;
        frame.grip_delta = (float)(ctx.avg_grip_front - ctx.avg_rear_grip);
        frame.calc_rear_lat_force = (float)ctx.calc_rear_lat_force;

        frame.smoothed_yaw_accel = (float)m_yaw_accel_smoothed;
        frame.lat_load_norm = (float)m_sop_load_smoothed;
        
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

        frame.session_peak_torque = (float)m_session_peak_torque;
        frame.dynamic_weight_factor = (float)dynamic_weight_factor;
        frame.structural_mult = (float)m_smoothed_structural_mult;
        frame.vibration_mult = (float)m_smoothed_vibration_mult;
        frame.steering_angle_deg = steering_angle_deg;
        frame.steering_range_deg = range_deg;
        frame.debug_freq = (float)m_debug_freq;
        frame.tire_radius = (float)fl.mStaticUndeflectedRadius / 100.0f;
        
        // --- FFB COMPONENTS (400Hz) ---
        frame.ffb_total = (float)norm_force;
        frame.ffb_base = (float)base_input;
        frame.ffb_understeer_drop = understeer_drop;
        frame.ffb_oversteer_boost = oversteer_boost;
        frame.ffb_sop = (float)ctx.sop_base_force;
        frame.ffb_rear_torque = (float)ctx.rear_torque;
        frame.ffb_scrub_drag = (float)ctx.scrub_drag_force;
        frame.ffb_yaw_kick = (float)ctx.yaw_force;
        frame.ffb_gyro_damping = (float)ctx.gyro_force;
        frame.ffb_road_texture = (float)ctx.road_noise;
        frame.ffb_slide_texture = (float)ctx.slide_noise;
        frame.ffb_lockup_vibration = (float)ctx.lockup_rumble;
        frame.ffb_spin_vibration = (float)ctx.spin_rumble;
        frame.ffb_bottoming_crunch = (float)ctx.bottoming_crunch;
        frame.ffb_abs_pulse = (float)ctx.abs_pulse_force;
        frame.ffb_soft_lock = (float)ctx.soft_lock_force;

        frame.extrapolated_yaw_accel = (float)upsampled_data->mLocalRotAccel.y;

        // Passive test: calculate yaw accel from velocity derivative
        // Note: mLocalRot.y is angular velocity (rad/s), so its derivative is angular acceleration (rad/s^2).
        double current_yaw_rate = upsampled_data->mLocalRot.y;
        if (!m_yaw_rate_log_seeded) {
            m_prev_yaw_rate_log = current_yaw_rate;
            m_yaw_rate_log_seeded = true;
        }
        frame.derived_yaw_accel = (ctx.dt > 1e-6) ? (float)((current_yaw_rate - m_prev_yaw_rate_log) / ctx.dt) : 0.0f;
        m_prev_yaw_rate_log = current_yaw_rate;

        frame.ffb_shaft_torque = (float)upsampled_data->mSteeringShaftTorque;
        frame.ffb_gen_torque = (float)genFFBTorque;
        frame.ffb_grip_factor = (float)ctx.grip_factor;
        frame.speed_gate = (float)ctx.speed_gate;
        frame.load_peak_ref = (float)m_auto_peak_load;

        // --- SYSTEM (400Hz) ---
        frame.physics_rate = (float)m_physics_rate;
        frame.clipping = (uint8_t)(std::abs(norm_force) > CLIPPING_THRESHOLD);
        frame.warn_bits = 0;
        if (ctx.frame_warn_load) frame.warn_bits |= 0x01;
        if (ctx.frame_warn_grip || ctx.frame_warn_rear_grip) frame.warn_bits |= 0x02;
        if (ctx.frame_warn_dt) frame.warn_bits |= 0x04;
        frame.marker = 0; // Handled inside Log()
        
        AsyncLogger::Get().Log(frame);
    }
    
    return (std::max)(-1.0, (std::min)(1.0, norm_force));
}

// Helper: Calculate Seat-of-the-Pants (SoP) Lateral & Oversteer Boost
void FFBEngine::calculate_sop_lateral(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // 1. Raw Lateral G (Chassis-relative X)
    // Clamp to 5G to prevent numeric instability in crashes
    double raw_g = (std::max)(-G_LIMIT_5G * GRAVITY_MS2, (std::min)(G_LIMIT_5G * GRAVITY_MS2, data->mLocalAccel.x));
    double lat_g_accel = (raw_g / GRAVITY_MS2);

    // 2. Normalized Lateral Load Transfer (Issue #213) - Front Axle Only
    double fl_load_front = data->mWheel[0].mTireLoad;
    double fr_load_front = data->mWheel[1].mTireLoad;
    if (ctx.frame_warn_load) {
        fl_load_front = calculate_kinematic_load(data, 0);
        fr_load_front = calculate_kinematic_load(data, 1);
    }
    // v0.7.150: Normalize by FIXED static front weight to prevent aero-fade (Issue #282)
    // We use the class-based default load (axle total) as the fixed normalization basis.
    double lat_load_norm = (m_fixed_static_axle_load_front > 100.0) ? (fl_load_front - fr_load_front) / m_fixed_static_axle_load_front : 0.0;
    
    // Smoothing: Map 0.0-1.0 slider to 0.1-0.0001s tau
    double smoothness = (double)m_sop_smoothing_factor;
    smoothness = (std::max)(0.0, (std::min)(SMOOTHNESS_LIMIT_0999, smoothness));
    double tau = smoothness * SOP_SMOOTHING_MAX_TAU;
    double alpha = ctx.dt / (tau + ctx.dt);
    alpha = (std::max)(MIN_LFM_ALPHA, (std::min)(1.0, alpha));
    
    m_sop_lat_g_smoothed += alpha * (lat_g_accel - m_sop_lat_g_smoothed);
    m_sop_load_smoothed += alpha * (lat_load_norm - m_sop_load_smoothed);

    // Base SoP Force (Combined)
    // v0.7.150: Applied 2.0x boost to Lateral Load to match Lateral G magnitude (Issue #282)
    double sop_base = (m_sop_lat_g_smoothed * (double)m_sop_effect + m_sop_load_smoothed * (double)m_lat_load_effect * 2.0) * (double)m_sop_scale;
    ctx.sop_unboosted_force = sop_base; // Store for snapshot
    
    // 2. Oversteer Boost (Grip Differential)
    // Calculate Rear Grip
    // Note: Rear grip estimation still uses average FRONT load for capacity scaling consistency
    // in the simplified grip model.
    GripResult rear_grip_res = calculate_grip(data->mWheel[2], data->mWheel[3], ctx.avg_load_front, m_warned_rear_grip,
                                                m_prev_slip_angle[2], m_prev_slip_angle[3],
                                                ctx.car_speed, ctx.dt, data->mVehicleName, data, false /* is_front */);
    ctx.avg_rear_grip = rear_grip_res.value;
    m_grip_diag.rear_original = rear_grip_res.original;
    m_grip_diag.rear_approximated = rear_grip_res.approximated;
    m_grip_diag.rear_slip_angle = rear_grip_res.slip_angle;
    if (rear_grip_res.approximated) ctx.frame_warn_rear_grip = true;
    
    if (!m_slope_detection_enabled) {
        double grip_delta = ctx.avg_grip_front - ctx.avg_rear_grip;
        if (grip_delta > 0.0) {
            sop_base *= (1.0 + (grip_delta * m_oversteer_boost * OVERSTEER_BOOST_MULT));
        }
    }
    ctx.sop_base_force = sop_base;
    
    // 3. Rear Aligning Torque (v0.4.9)
    // Calculate load for rear wheels (for tire stiffness scaling)
    double calc_load_rl = approximate_rear_load(data->mWheel[2]);
    double calc_load_rr = approximate_rear_load(data->mWheel[3]);
    ctx.avg_rear_load = (calc_load_rl + calc_load_rr) / DUAL_DIVISOR;
    
    // Rear lateral force estimation: F = Alpha * k * TireLoad
    double rear_slip_angle = m_grip_diag.rear_slip_angle;
    ctx.calc_rear_lat_force = rear_slip_angle * ctx.avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;
    ctx.calc_rear_lat_force = (std::max)(-MAX_REAR_LATERAL_FORCE, (std::min)(MAX_REAR_LATERAL_FORCE, ctx.calc_rear_lat_force));
    
    // Torque = Force * Aligning_Lever
    // Note negative sign: Oversteer (Rear Slide) pushes wheel TOWARDS slip direction
    ctx.rear_torque = -ctx.calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;
    
    // 4. Yaw Kick (Inertial Oversteer)
    // v0.7.144 (Solution 2): Derive Yaw Acceleration from Yaw Rate (mLocalRot.y) 
    // instead of using noisy game-provided mLocalRotAccel.y.
    double current_yaw_rate = data->mLocalRot.y;
    if (!m_yaw_rate_seeded) {
        m_prev_yaw_rate = current_yaw_rate;
        m_yaw_rate_seeded = true;
    }
    double derived_yaw_accel = (ctx.dt > 1e-6) ? (current_yaw_rate - m_prev_yaw_rate) / ctx.dt : 0.0;
    m_prev_yaw_rate = current_yaw_rate;

    // v0.4.18: Apply Smoothing FIRST to extract macroscopic movement and cancel chatter.
    // This prevents "signal rectification" where a threshold applied to raw noise
    // creates an artificial DC offset (Issue #241).
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < MIN_TAU_S) tau_yaw = MIN_TAU_S;
    double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
    m_yaw_accel_smoothed += alpha_yaw * (derived_yaw_accel - m_yaw_accel_smoothed);

    double processed_yaw = 0.0;
    // Reject yaw at low speeds
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS) {
        // Continuous Deadzone (Issue #241):
        // Subtract the threshold from the smoothed signal to ensure force starts from 0.0.
        if (std::abs(m_yaw_accel_smoothed) > (double)m_yaw_kick_threshold) {
            processed_yaw = m_yaw_accel_smoothed - std::copysign((double)m_yaw_kick_threshold, m_yaw_accel_smoothed);
        }
    }
    
    ctx.yaw_force = -1.0 * processed_yaw * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK;
    
    // Apply speed gate to all lateral effects
    ctx.sop_base_force *= ctx.speed_gate;
    ctx.rear_torque *= ctx.speed_gate;
    ctx.yaw_force *= ctx.speed_gate;
}

// Helper: Calculate Gyroscopic Damping (v0.4.17)
void FFBEngine::calculate_gyro_damping(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // 1. Calculate Steering Velocity (rad/s)
    float range = data->mPhysicalSteeringWheelRange;

    // Fallback to REST API if enabled and SM range is invalid (Issue #221)
    if (m_rest_api_enabled && range <= 0.0f) {
        float fallback_deg = RestApiProvider::Get().GetFallbackRangeDeg();
        if (fallback_deg > 0.0f) {
            range = fallback_deg * ((float)PI / 180.0f);
        }
    }

    if (range <= 0.0f) range = (float)DEFAULT_STEERING_RANGE_RAD;
    double steer_angle = data->mUnfilteredSteering * (range / DUAL_DIVISOR);
    double steer_vel = (steer_angle - m_prev_steering_angle) / ctx.dt;
    m_prev_steering_angle = steer_angle;
    
    // 2. Alpha Smoothing
    double tau_gyro = (double)m_gyro_smoothing;
    if (tau_gyro < MIN_TAU_S) tau_gyro = MIN_TAU_S;
    double alpha_gyro = ctx.dt / (tau_gyro + ctx.dt);
    m_steering_velocity_smoothed += alpha_gyro * (steer_vel - m_steering_velocity_smoothed);
    
    // 3. Force = -Vel * Gain * Speed_Scaling
    // Speed scaling: Gyro effect increases with wheel RPM (car speed)
    ctx.gyro_force = -1.0 * m_steering_velocity_smoothed * m_gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE);
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
        ctx.abs_pulse_force = (double)(std::sin(m_abs_phase) * m_abs_gain * ABS_PULSE_MAGNITUDE_SCALER * ctx.speed_gate);
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
        double radius = (double)w.mStaticUndeflectedRadius / UNIT_CM_TO_M;
        if (radius < RADIUS_FALLBACK_MIN_M) radius = RADIUS_FALLBACK_DEFAULT_M;
        double car_dec_ang = -std::abs(data->mLocalAccel.z / radius);

        // Signal Quality Check (Reject surface bumps)
        double susp_vel = std::abs(w.mVerticalTireDeflection - m_prev_vert_deflection[i]) / ctx.dt;
        bool is_bumpy = (susp_vel > (double)m_lockup_bump_reject);

        // Pre-conditions
        bool brake_active = (data->mUnfilteredBrake > PREDICTION_BRAKE_THRESHOLD);
        bool is_grounded = (w.mSuspForce > PREDICTION_LOAD_THRESHOLD);

        double start_threshold = (double)m_lockup_start_pct / PERCENT_TO_DECIMAL;
        double full_threshold = (double)m_lockup_full_pct / PERCENT_TO_DECIMAL;
        double trigger_threshold = full_threshold;

        if (brake_active && is_grounded && !is_bumpy) {
            // Predictive Trigger: Wheel decelerating significantly faster than chassis
            double sensitivity_threshold = -1.0 * (double)m_lockup_prediction_sens;
            if (wheel_accel < car_dec_ang * LOCKUP_ACCEL_MARGIN && wheel_accel < sensitivity_threshold) {
                trigger_threshold = start_threshold; // Ease into effect earlier
            }
        }

        // 2. Intensity Calculation
        if (slip_abs > trigger_threshold) {
            double window = full_threshold - start_threshold;
            if (window < MIN_SLIP_WINDOW) window = MIN_SLIP_WINDOW;

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
            if (pressure_factor < LOW_PRESSURE_LOCKUP_THRESHOLD && slip_abs > LOW_PRESSURE_LOCKUP_FIX) pressure_factor = LOW_PRESSURE_LOCKUP_FIX; // Catch low-pressure lockups

            if (severity > worst_severity) {
                worst_severity = severity;
                chosen_freq_multiplier = freq_mult;
                chosen_pressure_factor = pressure_factor;
            }
        }
    }

    // 3. Vibration Synthesis
    if (worst_severity > 0.0) {
        double base_freq = LOCKUP_BASE_FREQ + (ctx.car_speed * LOCKUP_FREQ_SPEED_MULT);
        double final_freq = base_freq * chosen_freq_multiplier * (double)m_lockup_freq_scale;
        
        m_lockup_phase += final_freq * ctx.dt * TWO_PI;
        m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI);
        
        double amp = worst_severity * chosen_pressure_factor * m_lockup_gain * (double)BASE_NM_LOCKUP_VIBRATION * ctx.brake_load_factor;
        
        // v0.4.38: Boost rear lockup volume
        if (chosen_freq_multiplier < 1.0) amp *= (double)m_lockup_rear_boost;

        ctx.lockup_rumble = std::sin(m_lockup_phase) * amp * ctx.speed_gate;
    }
}

// Helper: Calculate Wheel Spin Vibration (v0.6.36)
void FFBEngine::calculate_wheel_spin(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (m_spin_enabled && data->mUnfilteredThrottle > SPIN_THROTTLE_THRESHOLD) {
        double slip_rl = calculate_wheel_slip_ratio(data->mWheel[2]);
        double slip_rr = calculate_wheel_slip_ratio(data->mWheel[3]);
        double max_slip = (std::max)(slip_rl, slip_rr);
        
        if (max_slip > SPIN_SLIP_THRESHOLD) {
            double severity = (max_slip - SPIN_SLIP_THRESHOLD) / SPIN_SEVERITY_RANGE;
            severity = (std::min)(1.0, severity);
            
            // Attenuate primary torque when spinning (Torque Drop)
            // v0.6.43: Blunted effect (0.6 multiplier) to prevent complete loss of feel
            ctx.gain_reduction_factor = (1.0 - (severity * m_spin_gain * SPIN_TORQUE_DROP_FACTOR));
            
            // Generate vibration based on spin velocity (RPM delta)
            double slip_speed_ms = ctx.car_speed * max_slip;
            double freq = (SPIN_BASE_FREQ + (slip_speed_ms * SPIN_FREQ_SLIP_MULT)) * (double)m_spin_freq_scale;
            if (freq > SPIN_MAX_FREQ) freq = SPIN_MAX_FREQ; // Human sensory limit for gross vibration
            
            m_spin_phase += freq * ctx.dt * TWO_PI;
            m_spin_phase = std::fmod(m_spin_phase, TWO_PI);
            
            double amp = severity * m_spin_gain * (double)BASE_NM_SPIN_VIBRATION;
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
    double front_slip_avg = (lat_vel_fl + lat_vel_fr) / DUAL_DIVISOR;

    // Use average lateral patch velocity of rear wheels
    double lat_vel_rl = std::abs(data->mWheel[2].mLateralPatchVel);
    double lat_vel_rr = std::abs(data->mWheel[3].mLateralPatchVel);
    double rear_slip_avg = (lat_vel_rl + lat_vel_rr) / DUAL_DIVISOR;

    // Use the max slide velocity between axles
    double effective_slip_vel = (std::max)(front_slip_avg, rear_slip_avg);
    
    if (effective_slip_vel > SLIDE_VEL_THRESHOLD) {
        // High-frequency sawtooth noise for localized friction feel
        double base_freq = SLIDE_BASE_FREQ + (effective_slip_vel * SLIDE_FREQ_VEL_MULT);
        double freq = base_freq * (double)m_slide_freq_scale;
        
        if (freq > SLIDE_MAX_FREQ) freq = SLIDE_MAX_FREQ; // Hard clamp for hardware safety
        
        m_slide_phase += freq * ctx.dt * TWO_PI;
        m_slide_phase = std::fmod(m_slide_phase, TWO_PI);
        
        // Sawtooth generator (0 to 1 range across TWO_PI) -> (-1 to 1)
        double sawtooth = (m_slide_phase / TWO_PI) * SAWTOOTH_SCALE - SAWTOOTH_OFFSET;
        
        // Intensity scaling (Grip based)
        double grip_scale = (std::max)(0.0, 1.0 - ctx.avg_grip_front);
        
        ctx.slide_noise = sawtooth * m_slide_texture_gain * (double)BASE_NM_SLIDE_TEXTURE * ctx.texture_load_factor * grip_scale;
    }
}

// Helper: Calculate Road Texture & Scrub Drag
void FFBEngine::calculate_road_texture(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // 1. Scrub Drag (Longitudinal resistive force from lateral sliding)
    if (m_scrub_drag_gain > 0.0) {
        double avg_lat_vel = (data->mWheel[0].mLateralPatchVel + data->mWheel[1].mLateralPatchVel) / DUAL_DIVISOR;
        double abs_lat_vel = std::abs(avg_lat_vel);
        
        if (abs_lat_vel > SCRUB_VEL_THRESHOLD) {
            double fade = (std::min)(1.0, abs_lat_vel / SCRUB_FADE_RANGE); // Fade in over 0.5m/s
            double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;
            ctx.scrub_drag_force = drag_dir * m_scrub_drag_gain * (double)BASE_NM_SCRUB_DRAG * fade;
        }
    }

    if (!m_road_texture_enabled) return;
    
    // 2. Road Texture (Delta Deflection Method)
    // Measures the rate of change in tire vertical compression
    double delta_l = data->mWheel[0].mVerticalTireDeflection - m_prev_vert_deflection[0];
    double delta_r = data->mWheel[1].mVerticalTireDeflection - m_prev_vert_deflection[1];
    
    // Outlier rejection (crashes/jumps)
    delta_l = (std::max)(-DEFLECTION_DELTA_LIMIT, (std::min)(DEFLECTION_DELTA_LIMIT, delta_l));
    delta_r = (std::max)(-DEFLECTION_DELTA_LIMIT, (std::min)(DEFLECTION_DELTA_LIMIT, delta_r));
    
    double road_noise_val = 0.0;
    
    // FALLBACK (v0.6.36): If mVerticalTireDeflection is missing (Encrypted DLC),
    // use Chassis Vertical Acceleration delta as a secondary source.
    bool deflection_active = (std::abs(delta_l) > DEFLECTION_ACTIVE_THRESHOLD || std::abs(delta_r) > DEFLECTION_ACTIVE_THRESHOLD);
    
    if (deflection_active || ctx.car_speed < ROAD_TEXTURE_SPEED_THRESHOLD) {
        road_noise_val = (delta_l + delta_r) * DEFLECTION_NM_SCALE; // Scale to NM
    } else {
        // Fallback to vertical acceleration rate-of-change (jerk-like scaling)
        double vert_accel = data->mLocalAccel.y;
        double delta_accel = vert_accel - m_prev_vert_accel;
        road_noise_val = delta_accel * ACCEL_ROAD_TEXTURE_SCALE * DEFLECTION_NM_SCALE; // Blend into similar range
    }
    
    ctx.road_noise = road_noise_val * m_road_texture_gain * ctx.texture_load_factor;
    ctx.road_noise *= ctx.speed_gate;
}

void FFBEngine::UpdateMetadata(const SharedMemoryObjectOut& data) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    const char* vClass = nullptr;
    const char* vName = nullptr;
    const char* tName = data.scoring.scoringInfo.mTrackName;

    if (data.telemetry.playerHasVehicle) {
        uint8_t idx = data.telemetry.playerVehicleIdx;
        if (idx < 104) {
            vClass = data.scoring.vehScoringInfo[idx].mVehicleClass;
            vName = data.scoring.vehScoringInfo[idx].mVehicleName;
        }
    }

    UpdateMetadataInternal(vClass, vName, tName);
}

bool FFBEngine::UpdateMetadataInternal(const char* vehicleClass, const char* vehicleName, const char* trackName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    bool seeded = false;

    // 1. Handle Vehicle/Class Change
    if (vehicleClass && (m_current_class_name != vehicleClass || (vehicleName && m_last_handled_vehicle_name != vehicleName))) {
        m_current_class_name = vehicleClass;
        InitializeLoadReference(vehicleClass, vehicleName);
        m_warned_invalid_range = false; // Reset warning on car change
        seeded = true;
    }

    // 2. Update Context strings (for UI/Logging)
    if (vehicleName && strcmp(m_vehicle_name, vehicleName) != 0) {
#ifdef _WIN32
        strncpy_s(m_vehicle_name, sizeof(m_vehicle_name), vehicleName, _TRUNCATE);
#else
        strncpy(m_vehicle_name, vehicleName, STR_MAX_64);
        m_vehicle_name[STR_MAX_64] = '\0';
#endif
    }

    if (trackName && strcmp(m_track_name, trackName) != 0) {
#ifdef _WIN32
        strncpy_s(m_track_name, sizeof(m_track_name), trackName, _TRUNCATE);
#else
        strncpy(m_track_name, trackName, STR_MAX_64);
        m_track_name[STR_MAX_64] = '\0';
#endif
    }

    return seeded;
}

void FFBEngine::ResetNormalization() {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    m_warned_invalid_range = false; // Issue #218: Reset warning on manual normalization reset

    // 1. Structural Normalization Reset (Stage 1)
    // If disabled, we return to the user's manual target.
    // If enabled, we reset to the target to restart the learning process.
    m_session_peak_torque = (std::max)(1.0, (double)m_target_rim_nm);
    m_smoothed_structural_mult = 1.0 / (m_session_peak_torque + EPSILON_DIV);
    m_rolling_average_torque = m_session_peak_torque;

    // 2. Vibration Normalization Reset (Stage 3)
    // Always return to the class-default seed load.
    ParsedVehicleClass vclass = ParseVehicleClass(m_current_class_name.c_str(), m_vehicle_name);
    m_auto_peak_load = GetDefaultLoadForClass(vclass);

    // Reset static load reference
    m_static_front_load = m_auto_peak_load * 0.5;
    m_static_load_latched = false;

    // If we have a saved static load, restore it (v0.7.70 logic)
    double saved_load = 0.0;
    if (Config::GetSavedStaticLoad(m_vehicle_name, saved_load)) {
        m_static_front_load = saved_load;
        m_static_load_latched = true;
    }

    m_smoothed_vibration_mult = 1.0;

    Logger::Get().LogFile("[FFB] Normalization state reset. Structural Peak: %.2f Nm | Load Peak: %.2f N",
        m_session_peak_torque, m_auto_peak_load);
}

// Helper: Calculate Suspension Bottoming (v0.6.22)
// NOTE: calculate_soft_lock has been moved to SteeringUtils.cpp.
void FFBEngine::calculate_suspension_bottoming(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    if (!m_bottoming_enabled) return;
    bool triggered = false;
    double intensity = 0.0;
    
    // Method 0: Direct Ride Height Monitoring
    if (m_bottoming_method == 0) {
        double min_rh = (std::min)(data->mWheel[0].mRideHeight, data->mWheel[1].mRideHeight);
        if (min_rh < BOTTOMING_RH_THRESHOLD_M && min_rh > -1.0) { // < 2mm
            triggered = true;
            intensity = (BOTTOMING_RH_THRESHOLD_M - min_rh) / BOTTOMING_RH_THRESHOLD_M; // Map 2mm->0mm to 0.0->1.0
        }
    } else {
        // Method 1: Suspension Force Impulse (Rate of Change)
        double dForceL = (data->mWheel[0].mSuspForce - m_prev_susp_force[0]) / ctx.dt;
        double dForceR = (data->mWheel[1].mSuspForce - m_prev_susp_force[1]) / ctx.dt;
        double max_dForce = (std::max)(dForceL, dForceR);
        
        if (max_dForce > BOTTOMING_IMPULSE_THRESHOLD_N_S) { // 100kN/s impulse
            triggered = true;
            intensity = (max_dForce - BOTTOMING_IMPULSE_THRESHOLD_N_S) / BOTTOMING_IMPULSE_RANGE_N_S;
        }
    }

    // Safety Trigger: Raw Load Peak (Catches Method 0/1 failures)
    if (!triggered) {
        double max_load = (std::max)(data->mWheel[0].mTireLoad, data->mWheel[1].mTireLoad);
        double bottoming_threshold = m_static_front_load * BOTTOMING_LOAD_MULT;
        if (max_load > bottoming_threshold) {
            triggered = true;
            double excess = max_load - bottoming_threshold;
            intensity = std::sqrt(excess) * BOTTOMING_INTENSITY_SCALE; // Non-linear response
        }
    }

    if (triggered) {
        // Generate high-intensity low-frequency "thump"
        double bump_magnitude = intensity * m_bottoming_gain * (double)BASE_NM_BOTTOMING;
        double freq = BOTTOMING_FREQ_HZ;
        
        m_bottoming_phase += freq * ctx.dt * TWO_PI;
        m_bottoming_phase = std::fmod(m_bottoming_phase, TWO_PI);
        
        ctx.bottoming_crunch = std::sin(m_bottoming_phase) * bump_magnitude * ctx.speed_gate;
    }
}
