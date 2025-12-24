#ifndef FFBENGINE_H
#define FFBENGINE_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>
#include "src/lmu_sm_interface/InternalsPlugin.hpp"

// Mathematical Constants
static constexpr double PI = 3.14159265358979323846;
static constexpr double TWO_PI = 2.0 * PI;

// Stats helper
struct ChannelStats {
    // Session-wide stats (Persistent)
    double session_min = 1e9;
    double session_max = -1e9;
    
    // Interval stats (Reset every second)
    double interval_sum = 0.0;
    long interval_count = 0;
    
    // Latched values for display/consumption by other threads (Interval)
    double l_avg = 0.0;
    // Latched values for display/consumption by other threads (Session)
    double l_min = 0.0;
    double l_max = 0.0;
    
    void Update(double val) {
        // Update Session Min/Max
        if (val < session_min) session_min = val;
        if (val > session_max) session_max = val;
        
        // Update Interval Accumulator
        interval_sum += val;
        interval_count++;
    }
    
    // Called every interval (e.g. 1s) to latch data and reset interval counters
    void ResetInterval() {
        if (interval_count > 0) {
            l_avg = interval_sum / interval_count;
        } else {
            l_avg = 0.0;
        }
        // Latch current session min/max for display
        l_min = session_min;
        l_max = session_max;
        
        // Reset interval data
        interval_sum = 0.0; 
        interval_count = 0;
    }
    
    // Compatibility helper
    double Avg() { return interval_count > 0 ? interval_sum / interval_count : 0.0; }
    void Reset() { ResetInterval(); }
};

// 1. Define the Snapshot Struct (Unified FFB + Telemetry)
struct FFBSnapshot {
    // --- Header A: FFB Components (Outputs) ---
    float total_output;
    float base_force;
    float sop_force;
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

    // Telemetry Health Flags
    bool warn_load;
    bool warn_grip;
    bool warn_dt;

    float debug_freq; // New v0.4.41: Frequency for diagnostics
    float tire_radius; // New v0.4.41: Tire radius in meters for theoretical freq calculation
};

struct BiquadNotch {
    // Coefficients
    double b0 = 0.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    // State history (Inputs x, Outputs y)
    double x1 = 0.0, x2 = 0.0;
    double y1 = 0.0, y2 = 0.0;

    // Update coefficients based on dynamic frequency
    void Update(double center_freq, double sample_rate, double Q) {
        // Safety: Clamp frequency to Nyquist (sample_rate / 2) and min 1Hz
        center_freq = (std::max)(1.0, (std::min)(center_freq, sample_rate * 0.49));
        
        double omega = 2.0 * PI * center_freq / sample_rate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * Q);

        double a0 = 1.0 + alpha;
        
        // Calculate and Normalize
        b0 = 1.0 / a0;
        b1 = (-2.0 * cs) / a0;
        b2 = 1.0 / a0;
        a1 = (-2.0 * cs) / a0;
        a2 = (1.0 - alpha) / a0;
    }

    // Apply filter to single sample
    double Process(double in) {
        double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        // Shift history
        x2 = x1; x1 = in;
        y2 = y1; y1 = out;
        
        return out;
    }
    
    void Reset() {
        x1 = x2 = y1 = y2 = 0.0;
    }
};

// Helper Result Struct for calculate_grip
struct GripResult {
    double value;           // Final grip value
    bool approximated;      // Was approximation used?
    double original;        // Original telemetry value
    double slip_angle;      // Calculated slip angle (if approximated)
};
    
// FFB Engine Class
class FFBEngine {
public:
    // Settings (GUI Sliders)
    float m_gain = 1.0f;          // Master Gain
    float m_understeer_effect = 38.0f; // Grip Drop (T300 Default)
    float m_sop_effect = 1.0f;    // Lateral G injection strength
    float m_min_force = 0.0f;     // 0.0 - 0.20 (Deadzone removal)
    
    // Configurable Smoothing & Caps (v0.3.9)
    float m_sop_smoothing_factor = 0.85f; // 0.0 (Max Smoothing) - 1.0 (Raw). Default 0.85 for responsive feel (15ms lag).
    float m_max_load_factor = 1.5f;      // Cap for load scaling (Default 1.5x)
    float m_sop_scale = 1.0f;            // SoP base scaling factor (Default 20.0 was too high for Nm)
    
    // v0.4.4 Features
    float m_max_torque_ref = 100.0f;      // Reference torque for 100% output (Default 100.0 Nm)
    bool m_invert_force = true;         // Invert final output signal
    
    // Base Force Debugging (v0.4.13)
    float m_steering_shaft_gain = 1.0f; // 0.0 - 1.0 (Base force attenuation)
    int m_base_force_mode = 0;          // 0=Native, 1=Synthetic, 2=Muted

    // New Effects (v0.2)
    float m_oversteer_boost = 1.0f; // Rear grip loss boost
    float m_rear_align_effect = 5.0f; 
    float m_sop_yaw_gain = 5.0f;      
    float m_gyro_gain = 0.0f;         
    float m_gyro_smoothing = 0.010f; // v0.5.8: Changed to Time Constant (Seconds). Default 10ms.
    float m_yaw_accel_smoothing = 0.010f;      // v0.5.8: Time Constant (Seconds). Default 10ms.
    float m_chassis_inertia_smoothing = 0.025f; // v0.5.8: Time Constant (Seconds). Default 25ms.
    
    bool m_lockup_enabled = false;
    float m_lockup_gain = 0.5f;
    
    bool m_spin_enabled = false;
    float m_spin_gain = 0.5f;

    // Texture toggles
    bool m_slide_texture_enabled = false; // Default off (T300 standard)
    float m_slide_texture_gain = 0.5f; // 0.0 - 5.0
    float m_slide_freq_scale = 1.0f;   // NEW: Frequency Multiplier (v0.4.36)
    
    bool m_road_texture_enabled = false;
    float m_road_texture_gain = 0.5f; // 0.0 - 1.0
    
    // Bottoming Effect (v0.3.2)
    bool m_bottoming_enabled = true;
    float m_bottoming_gain = 1.0f;

    float m_slip_angle_smoothing = 0.015f; // v0.4.40: Expose tau (Smoothing Time Constant in seconds)
    
    // NEW: Grip Estimation Settings (v0.5.7)
    float m_optimal_slip_angle = 0.10f; // Default 0.10 rad (5.7 deg)
    float m_optimal_slip_ratio = 0.12f; // Default 0.12 (12%)
    
    // NEW: Steering Shaft Smoothing (v0.5.7)
    float m_steering_shaft_smoothing = 0.0f; // Time constant in seconds (0.0 = off)
    
    // v0.4.41: Signal Filtering Settings
    bool m_flatspot_suppression = false;
    float m_notch_q = 2.0f; // Default Q-Factor
    float m_flatspot_strength = 1.0f; // Default 1.0 (100% suppression)
    
    // Static Notch Filter (v0.4.43)
    bool m_static_notch_enabled = false;
    float m_static_notch_freq = 50.0f;
    
    // Signal Diagnostics
    double m_debug_freq = 0.0; // Estimated frequency for GUI
    double m_theoretical_freq = 0.0; // Theoretical wheel frequency for GUI

    // Warning States (Console logging)
    bool m_warned_load = false;
    bool m_warned_grip = false;
    bool m_warned_rear_grip = false; // v0.4.5 Fix
    bool m_warned_dt = false;
    
    // Diagnostics (v0.4.5 Fix)
    struct GripDiagnostics {
        bool front_approximated = false;
        bool rear_approximated = false;
        double front_original = 0.0;
        double rear_original = 0.0;
        double front_slip_angle = 0.0;
        double rear_slip_angle = 0.0;
    } m_grip_diag;
    
    // Hysteresis for missing load
    int m_missing_load_frames = 0;

    // Internal state
    double m_prev_vert_deflection[2] = {0.0, 0.0}; // FL, FR
    double m_prev_slip_angle[4] = {0.0, 0.0, 0.0, 0.0}; // FL, FR, RL, RR (LPF State)
    
    // Gyro State (v0.4.17)
    double m_prev_steering_angle = 0.0;
    double m_steering_velocity_smoothed = 0.0;
    
    // Yaw Acceleration Smoothing State (v0.4.18)
    double m_yaw_accel_smoothed = 0.0;

    // Internal state for Steering Shaft Smoothing (v0.5.7)
    double m_steering_shaft_torque_smoothed = 0.0;

    // Kinematic Smoothing State (v0.4.38)
    double m_accel_x_smoothed = 0.0;
    double m_accel_z_smoothed = 0.0; // Longitudinal
    
    // Kinematic Physics Parameters (v0.4.39)
    // These parameters are used when telemetry (mTireLoad, mSuspForce) is blocked on encrypted content.
    // Values are empirical approximations tuned for typical GT3/LMP2 cars.
    // 
    // Mass: 1100kg represents average weight for GT3 (~1200kg) and LMP2 (~930kg)
    // Aero Coefficient: 2.0 is a simplified scalar for v² downforce (real values vary 1.5-3.5)
    // Weight Bias: 0.55 (55% rear) is typical for mid-engine race cars
    // Roll Stiffness: 0.6 scales lateral weight transfer (0.5=soft, 0.8=stiff)
    float m_approx_mass_kg = 1100.0f;
    float m_approx_aero_coeff = 2.0f;
    float m_approx_weight_bias = 0.55f;
    float m_approx_roll_stiffness = 0.6f;

    // Phase Accumulators for Dynamic Oscillators
    double m_lockup_phase = 0.0;
    double m_spin_phase = 0.0;
    double m_slide_phase = 0.0;
    double m_bottoming_phase = 0.0;
    
    // Internal state for Bottoming (Method B)
    double m_prev_susp_force[2] = {0.0, 0.0}; // FL, FR

    // New Settings (v0.4.5)
    bool m_use_manual_slip = false;
    int m_bottoming_method = 0; // 0=Scraping (Default), 1=Suspension Spike
    float m_scrub_drag_gain = 0.0f; // New Effect: Drag resistance

    // Smoothing State
    double m_sop_lat_g_smoothed = 0.0;
    
    // Filter Instances (v0.4.41)
    // Filter Instances (v0.4.41)
    BiquadNotch m_notch_filter;
    BiquadNotch m_static_notch_filter;

    // Frequency Estimator State (v0.4.41)
    double m_last_crossing_time = 0.0;
    double m_torque_ac_smoothed = 0.0; // For High-Pass
    double m_prev_ac_torque = 0.0;

    // Telemetry Stats
    ChannelStats s_torque;
    ChannelStats s_load;
    ChannelStats s_grip;
    ChannelStats s_lat_g;
    std::chrono::steady_clock::time_point last_log_time;

    // Thread-Safe Buffer (Producer-Consumer)
    std::vector<FFBSnapshot> m_debug_buffer;
    std::mutex m_debug_mutex;
    
    FFBEngine() {
        last_log_time = std::chrono::steady_clock::now();
    }
    
    // Helper to retrieve data (Consumer)
    std::vector<FFBSnapshot> GetDebugBatch() {
        std::vector<FFBSnapshot> batch;
        {
            std::lock_guard<std::mutex> lock(m_debug_mutex);
            if (!m_debug_buffer.empty()) {
                batch.swap(m_debug_buffer); // Fast swap
            }
        }
        return batch;
    }

    // ========================================
    // UI Reference & Physics Multipliers (v0.4.50)
    // ========================================
    // These constants represent the physical force (in Newton-meters) that each effect 
    // produces at a Gain setting of 1.0 (100%) and a MaxTorqueRef of 20.0 Nm.
    static constexpr float BASE_NM_SOP_LATERAL      = 1.0f;
    static constexpr float BASE_NM_REAR_ALIGN       = 3.0f;
    static constexpr float BASE_NM_YAW_KICK         = 5.0f;
    static constexpr float BASE_NM_GYRO_DAMPING     = 1.0f;
    static constexpr float BASE_NM_SLIDE_TEXTURE    = 1.5f;
    static constexpr float BASE_NM_ROAD_TEXTURE     = 2.5f;
    static constexpr float BASE_NM_LOCKUP_VIBRATION = 4.0f;
    static constexpr float BASE_NM_SPIN_VIBRATION   = 2.5f;
    static constexpr float BASE_NM_SCRUB_DRAG       = 5.0f;
    static constexpr float BASE_NM_BOTTOMING        = 1.0f;

private:
    // ========================================
    // Physics Constants (v0.4.9+)
    // ========================================
    // These constants are extracted from the calculation logic to improve maintainability
    // and provide a single source of truth for tuning. See docs/dev_docs/FFB_formulas.md
    // for detailed mathematical derivations.
    
    // Slip Angle Singularity Protection (v0.4.9)
    // Prevents division by zero when calculating slip angle at very low speeds.
    // Value: 0.5 m/s (~1.8 km/h) - Below this speed, slip angle is clamped.
    static constexpr double MIN_SLIP_ANGLE_VELOCITY = 0.5; // m/s
    
    // Rear Tire Stiffness Coefficient (v0.4.10)
    // Used in the LMU 1.2 rear lateral force workaround calculation.
    // Formula: F_lat = SlipAngle * Load * STIFFNESS
    // Value: 15.0 N/(rad·N) - Empirical approximation based on typical race tire cornering stiffness.
    // Real-world values range from 10-20 depending on tire compound, temperature, and pressure.
    // This value was tuned to produce realistic rear-end behavior when the game API fails to
    // report rear mLateralForce (known bug in LMU 1.2).
    // See: docs/dev_docs/FFB_formulas.md "Rear Aligning Torque (v0.4.10 Workaround)"
    static constexpr double REAR_TIRE_STIFFNESS_COEFFICIENT = 15.0; // N per (rad * N_load)
    
    // Maximum Rear Lateral Force Clamp (v0.4.10)
    // Safety limit to prevent physics explosions if slip angle spikes unexpectedly.
    // Value: ±6000 N - Represents maximum lateral force a race tire can generate.
    // This clamp is applied AFTER the workaround calculation to ensure stability.
    // Without this clamp, extreme slip angles (e.g., during spins) could generate
    // unrealistic forces that would saturate the FFB output or cause oscillations.
    static constexpr double MAX_REAR_LATERAL_FORCE = 6000.0; // N
    
    // Rear Align Torque Coefficient (v0.4.11)
    // Converts rear lateral force (Newtons) to steering torque (Newton-meters).
    // Formula: T_rear = F_lat * COEFFICIENT * m_rear_align_effect
    // Value: 0.001 Nm/N - Tuned to produce ~3.0 Nm at 3000N lateral force with effect=1.0.
    // This provides a distinct counter-steering cue during oversteer without overwhelming
    // the base steering feel. Increased from 0.00025 in v0.4.10 (4x) to boost rear-end feedback.
    // See: docs/dev_docs/FFB_formulas.md "Rear Aligning Torque"
    static constexpr double REAR_ALIGN_TORQUE_COEFFICIENT = 0.001; // Nm per N
    
    // Synthetic Mode Deadzone Threshold (v0.4.13)
    // Prevents sign flickering at steering center when using Synthetic (Constant) base force mode.
    // Value: 0.5 Nm - If abs(game_force) < threshold, base input is set to 0.0.
    // This creates a small deadzone around center to avoid rapid direction changes
    // when the steering shaft torque oscillates near zero.
    static constexpr double SYNTHETIC_MODE_DEADZONE_NM = 0.5; // Nm
 

    // Gyroscopic Damping Constants (v0.4.17)
    // Default steering range (540 degrees) if physics range is missing
    static constexpr double DEFAULT_STEERING_RANGE_RAD = 9.4247; 
    // Normalizes car speed (m/s) to 0-1 range for typical speeds (10m/s baseline)
    static constexpr double GYRO_SPEED_SCALE = 10.0;
    
    // Kinematic Load Model Constants (v0.4.39)
    // Weight Transfer Scaling: Approximates (Mass * Accel * CG_Height / Wheelbase)
    // Value of 2000.0 is empirically tuned for typical race car geometry
    // Real calculation would be: ~1100kg * 1.0G * 0.5m / 2.8m ≈ 1960N
    static constexpr double WEIGHT_TRANSFER_SCALE = 2000.0; // N per G
    
    // Suspension Force Validity Threshold (v0.4.39)
    // If mSuspForce < this value, assume telemetry is blocked (encrypted content)
    // 10.0N is well below any realistic suspension force for a moving car
    static constexpr double MIN_VALID_SUSP_FORCE = 10.0; // N 

    // Lockup Frequency Differentiation Constants (v0.5.11)
    // These constants control the tactile differentiation between front and rear wheel lockups.
    // Front lockup uses 1.0x frequency (high pitch "Screech") for standard understeer feedback.
    // Rear lockup uses 0.5x frequency (low pitch "Heavy Judder") to warn of rear axle instability.
    // The amplitude boost emphasizes the danger of potential spin during rear lockups.
    static constexpr double LOCKUP_FREQ_MULTIPLIER_REAR = 0.3;  // Rear lockup frequency (0.5: 50% of base) // 0.3;  // Even lower pitch
    static constexpr double LOCKUP_AMPLITUDE_BOOST_REAR = 1.5;  // Rear lockup amplitude boost (1.2: 20% increase) //  1.5;  // 50% boost


public:
    // Helper: Calculate Raw Slip Angle for a pair of wheels (v0.4.9 Refactor)
    // Returns the average slip angle of two wheels using atan2(lateral_vel, longitudinal_vel)
    // v0.4.19: Removed abs() from lateral velocity to preserve sign for debug visualization
    double calculate_raw_slip_angle_pair(const TelemWheelV01& w1, const TelemWheelV01& w2) {
        double v_long_1 = std::abs(w1.mLongitudinalGroundVel);
        double v_long_2 = std::abs(w2.mLongitudinalGroundVel);
        if (v_long_1 < MIN_SLIP_ANGLE_VELOCITY) v_long_1 = MIN_SLIP_ANGLE_VELOCITY;
        if (v_long_2 < MIN_SLIP_ANGLE_VELOCITY) v_long_2 = MIN_SLIP_ANGLE_VELOCITY;
        // v0.4.19: PRESERVE SIGN for debug graphs - do NOT use abs()
        double raw_angle_1 = std::atan2(w1.mLateralPatchVel, v_long_1);
        double raw_angle_2 = std::atan2(w2.mLateralPatchVel, v_long_2);
        return (raw_angle_1 + raw_angle_2) / 2.0;
    }

    // Helper: Calculate Slip Angle (v0.4.6 LPF + Logic)
    // v0.4.37: Added Time-Corrected Smoothing (Report v0.4.37)
    // v0.4.19 CRITICAL FIX: Removed abs() from mLateralPatchVel to preserve sign
    // This allows rear aligning torque to provide correct counter-steering in BOTH directions
    double calculate_slip_angle(const TelemWheelV01& w, double& prev_state, double dt) {
        double v_long = std::abs(w.mLongitudinalGroundVel);
        if (v_long < MIN_SLIP_ANGLE_VELOCITY) v_long = MIN_SLIP_ANGLE_VELOCITY;
        
        // v0.4.19: PRESERVE SIGN - Do NOT use abs() on lateral velocity
        // Positive lateral vel (+X = left) → Positive slip angle
        // Negative lateral vel (-X = right) → Negative slip angle
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
    GripResult calculate_grip(const TelemWheelV01& w1, 
                              const TelemWheelV01& w2,
                              double avg_load,
                              bool& warned_flag,
                              double& prev_slip1,
                              double& prev_slip2,
                              double car_speed,
                              double dt) {
        GripResult result;
        result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
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
            
            // Low Speed Cutoff (v0.4.6)
            if (car_speed < 5.0) {
                // Note: We still keep the calculated slip_angle in result.slip_angle
                // for visualization/rear torque, even if we force grip to 1.0 here.
                result.value = 1.0; 
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
                    // Sigmoid-like drop-off: 1 / (1 + 2x)
                    result.value = 1.0 / (1.0 + excess * 2.0);
                } else {
                    result.value = 1.0;
                }
            }
            
            // Safety Clamp (v0.4.6): Never drop below 0.2 in approximation
            result.value = (std::max)(0.2, result.value);
            
            if (!warned_flag) {
                std::cout << "[WARNING] Missing Grip. Using Approx based on Slip Angle." << std::endl;
                warned_flag = true;
            }
        }
        
        result.value = (std::max)(0.0, (std::min)(1.0, result.value));
        return result;
    }

    // Helper: Approximate Load (v0.4.5)
    double approximate_load(const TelemWheelV01& w) {
        // Base: Suspension Force + Est. Unsprung Mass (300N)
        // Note: mSuspForce captures weight transfer and aero
        return w.mSuspForce + 300.0;
    }

    // Helper: Approximate Rear Load (v0.4.10)
    double approximate_rear_load(const TelemWheelV01& w) {
        // Base: Suspension Force + Est. Unsprung Mass (300N)
        // This captures weight transfer (braking/accel) and aero downforce implicitly via suspension compression
        return w.mSuspForce + 300.0;
    }

    // Helper: Calculate Kinematic Load (v0.4.39)
    // Estimates tire load from chassis physics when telemetry (mSuspForce) is missing.
    // This is critical for encrypted DLC content where suspension sensors are blocked.
    double calculate_kinematic_load(const TelemInfoV01* data, int wheel_index) {
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
        // - Braking: Chassis decelerates → Inertial force pushes rearward → +Z acceleration
        // - Result: Front wheels GAIN load, Rear wheels LOSE load
        // - Source: docs/dev_docs/coordinate_system_reference.md
        // 
        // Formula: (Accel / g) * WEIGHT_TRANSFER_SCALE
        // We use SMOOTHED acceleration to simulate chassis pitch inertia (~35ms lag)
        double long_transfer = (m_accel_z_smoothed / 9.81) * WEIGHT_TRANSFER_SCALE; 
        if (is_rear) long_transfer *= -1.0; // Subtract from Rear during Braking

        // 4. Lateral Weight Transfer (Cornering)
        // COORDINATE SYSTEM VERIFIED (v0.4.39):
        // - LMU: +X axis points LEFT (out the left side of the car)
        // - Right Turn: Centrifugal force pushes LEFT → +X acceleration
        // - Result: LEFT wheels (outside) GAIN load, RIGHT wheels (inside) LOSE load
        // - Source: docs/dev_docs/coordinate_system_reference.md
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
    double calculate_manual_slip_ratio(const TelemWheelV01& w, double car_speed_ms) {
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

    double calculate_force(const TelemInfoV01* data) {
        if (!data) return 0.0;
        
        double dt = data->mDeltaTime;

        // Sanity Check Flags for this frame
        bool frame_warn_load = false;
        bool frame_warn_grip = false;
        bool frame_warn_dt = false;

        // --- SANITY CHECK: DELTA TIME ---
        if (dt <= 0.000001) {
            dt = 0.0025; // Default to 400Hz
            if (!m_warned_dt) {
                std::cout << "[WARNING] Invalid DeltaTime (<=0). Using default 0.0025s." << std::endl;
                m_warned_dt = true;
            }
            frame_warn_dt = true;
        }

        // Front Left and Front Right (Note: mWheel, not mWheels)
        const TelemWheelV01& fl = data->mWheel[0];
        const TelemWheelV01& fr = data->mWheel[1];

        // Critical: Use mSteeringShaftTorque instead of mSteeringArmForce
        // Explanation: LMU 1.2 introduced mSteeringShaftTorque (Nm) as the definitive FFB output.
        // Legacy mSteeringArmForce (N) is often 0.0 or inaccurate for Hypercars due to 
        // complex power steering modeling in the new engine.
        double game_force = data->mSteeringShaftTorque;

        // --- NEW: Steering Shaft Smoothing (v0.5.7) ---
        if (m_steering_shaft_smoothing > 0.0001f) {
            double tau_shaft = (double)m_steering_shaft_smoothing;
            double alpha_shaft = dt / (tau_shaft + dt);
            // Safety clamp
            alpha_shaft = (std::min)(1.0, (std::max)(0.001, alpha_shaft));
            
            m_steering_shaft_torque_smoothed += alpha_shaft * (game_force - m_steering_shaft_torque_smoothed);
            game_force = m_steering_shaft_torque_smoothed;
        } else {
            m_steering_shaft_torque_smoothed = game_force; // Reset state
        }

        // --- v0.4.41: Frequency Estimator & Dynamic Notch Filter ---
        
        // 1. Frequency Estimator Logic
        // Isolate AC component (Vibration) using simple High Pass (remove DC offset)
        // Alpha for HPF: fast smoothing to get the "average" center
        double alpha_hpf = dt / (0.1 + dt); 
        m_torque_ac_smoothed += alpha_hpf * (game_force - m_torque_ac_smoothed);
        double ac_torque = game_force - m_torque_ac_smoothed;

        // Detect Zero Crossing (Sign change)
        // Add hysteresis (0.05 Nm) to avoid noise triggering
        if ((m_prev_ac_torque < -0.05 && ac_torque > 0.05) || 
            (m_prev_ac_torque > 0.05 && ac_torque < -0.05)) {
            
            double now = data->mElapsedTime;
            double period = now - m_last_crossing_time;
            
            // Sanity check period (e.g., 1Hz to 200Hz)
            if (period > 0.005 && period < 1.0) {
                // Half-cycle * 2 = Full Cycle Period
                // Let's assume we detect every crossing (2 per cycle).
                double inst_freq = 1.0 / (period * 2.0);
                
                // Smooth the readout for GUI
                m_debug_freq = m_debug_freq * 0.9 + inst_freq * 0.1;
            }
            m_last_crossing_time = now;
        }
        m_prev_ac_torque = ac_torque;


        // 2. Dynamic Notch Filter Logic
        // Calculate Wheel Frequency (always, for GUI display)
        double car_v_long = std::abs(data->mLocalVel.z);
        
        // Get radius (convert cm to m)
        // Use Front Left as reference
        const TelemWheelV01& fl_ref = data->mWheel[0];
        double radius = (double)fl_ref.mStaticUndeflectedRadius / 100.0;
        if (radius < 0.1) radius = 0.33; // Safety fallback
        
        double circumference = 2.0 * PI * radius;
        
        // Avoid divide by zero
        double wheel_freq = (circumference > 0.0) ? (car_v_long / circumference) : 0.0;
        
        // Store for GUI display
        m_theoretical_freq = wheel_freq;
        
        // Apply filter if enabled
        if (m_flatspot_suppression) {
            // Only filter if moving fast enough (> 1Hz)
            if (wheel_freq > 1.0) {
                // Update filter coefficients
                m_notch_filter.Update(wheel_freq, 1.0/dt, (double)m_notch_q);
                
                // Apply filter
                double input_force = game_force;
                double filtered_force = m_notch_filter.Process(input_force);
                
                // Blend Output (Linear Interpolation)
                // Strength 1.0 = Fully Filtered. Strength 0.0 = Raw.
                game_force = input_force * (1.0f - m_flatspot_strength) + filtered_force * m_flatspot_strength;

            } else {
                // Reset filter state when stopped to prevent "ringing" on start
                m_notch_filter.Reset();
            }
        }
        
        // 3. Static Notch Filter (v0.4.43)
        if (m_static_notch_enabled) {
             // Fixed Q of 5.0 (Surgical)
             m_static_notch_filter.Update((double)m_static_notch_freq, 1.0/dt, 5.0);
             game_force = m_static_notch_filter.Process(game_force);
        } else {
             m_static_notch_filter.Reset();
        }
        
        // --- 0. UPDATE STATS ---
        double raw_torque = game_force;
        double raw_load = (fl.mTireLoad + fr.mTireLoad) / 2.0;
        double raw_grip = (fl.mGripFract + fr.mGripFract) / 2.0;
        double raw_lat_g = data->mLocalAccel.x;
        
        // --- SIGNAL CONDITIONING (Inertia Simulation) ---
        // Filter accelerometers to simulate chassis weight transfer lag
        double chassis_tau = (double)m_chassis_inertia_smoothing;
        if (chassis_tau < 0.0001) chassis_tau = 0.0001;
        double alpha_chassis = dt / (chassis_tau + dt);
        m_accel_x_smoothed += alpha_chassis * (data->mLocalAccel.x - m_accel_x_smoothed);
        m_accel_z_smoothed += alpha_chassis * (data->mLocalAccel.z - m_accel_z_smoothed);

        s_torque.Update(raw_torque);
        s_load.Update(raw_load);
        s_grip.Update(raw_grip);
        s_lat_g.Update(raw_lat_g);

        // Blocking I/O removed for performance (Report v0.4.2)
        // Stats logic preserved in s_* objects for potential GUI display or async logging.
        // If console logging is desired for debugging, it should be done in a separate thread.
        
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1) {
            // Latch stats for external reading
            s_torque.ResetInterval(); 
            s_load.ResetInterval(); 
            s_grip.ResetInterval(); 
            s_lat_g.ResetInterval();
            last_log_time = now;
        }

        // Debug variables (initialized to 0)
        double road_noise = 0.0;
        double slide_noise = 0.0;
        double lockup_rumble = 0.0;
        double spin_rumble = 0.0;
        double bottoming_crunch = 0.0;
        double scrub_drag_force = 0.0; // v0.4.7

        // --- PRE-CALCULATION: TIRE LOAD FACTOR ---
        double avg_load = raw_load;

        // SANITY CHECK: Hysteresis Logic
        // If load is exactly 0.0 but car is moving, telemetry is likely broken.
        // Use a counter to prevent flickering if data is noisy.
        if (avg_load < 1.0 && std::abs(data->mLocalVel.z) > 1.0) {
            m_missing_load_frames++;
        } else {
            // Decay count if data is good
            m_missing_load_frames = (std::max)(0, m_missing_load_frames - 1);
        }

        // Only trigger fallback if missing for > 20 frames (approx 50ms at 400Hz)
        if (m_missing_load_frames > 20) {
            // v0.4.39: Adaptive Kinematic Load
            // If SuspForce is ALSO missing (common in encrypted content), use Kinematic Model.
            // Check FL SuspForce (index 0). If < MIN_VALID_SUSP_FORCE, assume blocked.
            if (fl.mSuspForce > MIN_VALID_SUSP_FORCE) {
                double calc_load_fl = approximate_load(fl);
                double calc_load_fr = approximate_load(fr);
                avg_load = (calc_load_fl + calc_load_fr) / 2.0;
            } else {
                // SuspForce blocked -> Use Kinematic Model (Mass + Aero + Transfer)
                double kin_load_fl = calculate_kinematic_load(data, 0);
                double kin_load_fr = calculate_kinematic_load(data, 1);
                avg_load = (kin_load_fl + kin_load_fr) / 2.0;
            }
            
            if (!m_warned_load) {
                std::cout << "[WARNING] Missing Tire Load. Using Approximation." << std::endl;
                m_warned_load = true;
            }
            frame_warn_load = true;
        }
        
        // Normalize: 4000N is a reference "loaded" GT tire.
        double load_factor = avg_load / 4000.0;
        
        // SAFETY CLAMP (v0.4.6): Hard clamp at 2.0 (regardless of config) to prevent explosion
        // Also respect configured max if lower.
        double safe_max = (std::min)(2.0, (double)m_max_load_factor);
        load_factor = (std::min)(safe_max, (std::max)(0.0, load_factor));

        // --- 1. GAIN COMPENSATION (Decoupling) ---
        // Baseline: 20.0 Nm (The standard reference where 1.0 gain was tuned).
        // If MaxTorqueRef increases, we scale effects up to maintain relative intensity.
        double decoupling_scale = (double)m_max_torque_ref / 20.0;
        if (decoupling_scale < 0.1) decoupling_scale = 0.1; // Safety clamp

        // --- 1. Understeer Effect (Grip Modulation) ---
        // FRONT WHEEL GRIP CALCULATION (Refactored v0.4.5)
        
        double car_speed = std::abs(data->mLocalVel.z);

        // Calculate Front Grip using helper (handles fallback and diagnostics)
        // Pass persistent state for LPF (v0.4.6) - Indices 0 and 1
        GripResult front_grip_res = calculate_grip(fl, fr, avg_load, m_warned_grip, 
                                                   m_prev_slip_angle[0], m_prev_slip_angle[1], car_speed, dt);
        double avg_grip = front_grip_res.value;
        
        // Update Diagnostics
        m_grip_diag.front_original = front_grip_res.original;
        m_grip_diag.front_approximated = front_grip_res.approximated;
        m_grip_diag.front_slip_angle = front_grip_res.slip_angle;
        
        // Update Frame Warning Flag
        if (front_grip_res.approximated) {
            frame_warn_grip = true;
        }
        
        // Apply grip to steering force
        // grip_factor: 1.0 = full force, 0.0 = no force (full understeer)
        // m_understeer_effect: 0.0 = disabled, 1.0 = full effect
        double grip_loss = (1.0 - avg_grip) * m_understeer_effect;
        double grip_factor = 1.0 - grip_loss;
        
        // FIX: Clamp to 0.0 to prevent negative force (inversion) if effect > 1.0
        grip_factor = (std::max)(0.0, grip_factor);
        
        // --- BASE FORCE PROCESSING (v0.4.13) ---
        double base_input = 0.0;
        
        if (m_base_force_mode == 0) {
            // Mode 0: Native (Steering Shaft Torque)
            base_input = game_force;
        } else if (m_base_force_mode == 1) {
            // Mode 1: Synthetic (Constant with Direction)
            // Apply deadzone to prevent sign flickering at center
            if (std::abs(game_force) > SYNTHETIC_MODE_DEADZONE_NM) {
                double sign = (game_force > 0.0) ? 1.0 : -1.0;
                base_input = sign * (double)m_max_torque_ref; // Use Max Torque as reference constant
            } else {
                base_input = 0.0;
            }
        } else {
            // Mode 2: Muted
            base_input = 0.0;
        }
        
        // Apply Gain and Grip Modulation
        double output_force = (base_input * (double)m_steering_shaft_gain) * grip_factor;
        
        // --- 2. Seat of Pants (SoP) / Oversteer ---
        // Lateral G-force
        // v0.4.6: Clamp Input to reasonable Gs (+/- 5G)
        double raw_g = (std::max)(-49.05, (std::min)(49.05, data->mLocalAccel.x));
        
        // v0.4.30 FIX: Removed inversion. 
        // Analysis shows mLocalAccel.x sign matches desired FFB direction.
        // Right Turn -> Accel +X (Centrifugal Left) -> Force + (Left Pull / Aligning).
        // Left Turn -> Accel -X (Centrifugal Right) -> Force - (Right Pull / Aligning).
        double lat_g = (raw_g / 9.81);
        
        // SoP Smoothing (Time-Corrected Low Pass Filter) (Report v0.4.2)
        // m_sop_smoothing_factor (0.0 to 1.0) is treated as a "Smoothness" knob.
        // 0.0 = Very slow (High smoothness), 1.0 = Instant (Raw).
        // We map 0-1 to a Time Constant (tau) from ~0.2s to 0.0s.
        // Formula: alpha = dt / (tau + dt)
        
        double smoothness = 1.0 - (double)m_sop_smoothing_factor; // Invert: 1.0 input -> 0.0 smoothness
        smoothness = (std::max)(0.0, (std::min)(0.999, smoothness));
        
        // Map smoothness to tau: 0.0 -> 0s, 1.0 -> 0.1s (approx 1.5Hz cutoff)
        double tau = smoothness * 0.1; 
        
        double alpha = dt / (tau + dt);
        
        // Safety clamp
        alpha = (std::max)(0.001, (std::min)(1.0, alpha));

        m_sop_lat_g_smoothed = m_sop_lat_g_smoothed + alpha * (lat_g - m_sop_lat_g_smoothed);
        
        double sop_base_force = m_sop_lat_g_smoothed * m_sop_effect * (double)m_sop_scale * decoupling_scale;
        double sop_total = sop_base_force;
        
        // REAR WHEEL GRIP CALCULATION (Refactored v0.4.5)
        
        // Calculate Rear Grip using helper (now includes fallback)
        // Pass persistent state for LPF (v0.4.6) - Indices 2 and 3
        GripResult rear_grip_res = calculate_grip(data->mWheel[2], data->mWheel[3], avg_load, m_warned_rear_grip,
                                                  m_prev_slip_angle[2], m_prev_slip_angle[3], car_speed, dt);
        double avg_rear_grip = rear_grip_res.value;
        
        // Update Diagnostics
        m_grip_diag.rear_original = rear_grip_res.original;
        m_grip_diag.rear_approximated = rear_grip_res.approximated;
        m_grip_diag.rear_slip_angle = rear_grip_res.slip_angle;
        
        // Update local frame warning for rear grip
        bool frame_warn_rear_grip = rear_grip_res.approximated;

        // Delta between front and rear grip
        double grip_delta = avg_grip - avg_rear_grip;
        if (grip_delta > 0.0) {
            sop_total *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
        }
        
        // ========================================
        // --- 2a. Rear Aligning Torque Integration ---
        // ========================================
        // WORKAROUND for LMU 1.2 API Bug (v0.4.10)
        // 
        // PROBLEM: LMU 1.2 reports mLateralForce = 0.0 for rear tires, making it impossible
        // to calculate rear aligning torque using the standard formula. This breaks oversteer
        // feedback and rear-end feel.
        // 
        // SOLUTION: Manually calculate rear lateral force using tire physics approximation:
        //   F_lateral = SlipAngle × Load × TireStiffness
        // 
        // This workaround will be removed when the LMU API is fixed to report rear lateral forces.
        // See: docs/dev_docs/FFB_formulas.md "Rear Aligning Torque (v0.4.10 Workaround)"
        
        // Step 1: Calculate Rear Loads
        // Use suspension force + estimated unsprung mass (300N) to approximate tire load.
        // This captures weight transfer (braking/accel) and aero downforce via suspension compression.
        // 
        // TODO (v0.4.40): If mSuspForce is also blocked for rear wheels (encrypted content),
        // this approximation will be weak. Consider using calculate_kinematic_load() here as well.
        // However, empirical testing shows mSuspForce is typically available even when mTireLoad
        // is blocked, so this is a low-priority enhancement.
        // See: docs/dev_docs/code_reviews/rear_load_approximation_note.md
        double calc_load_rl = approximate_rear_load(data->mWheel[2]);
        double calc_load_rr = approximate_rear_load(data->mWheel[3]);
        double avg_rear_load = (calc_load_rl + calc_load_rr) / 2.0;

        // Step 2: Calculate Rear Lateral Force (Workaround for missing mLateralForce)
        // Use the slip angle calculated by the grip approximation logic (if triggered).
        // The grip calculator computes slip angle = atan2(lateral_vel, longitudinal_vel)
        // and applies low-pass filtering for stability.
        double rear_slip_angle = m_grip_diag.rear_slip_angle; 
        
        // Apply simplified tire model: F = α × F_z × C_α
        // Where:
        //   α (alpha) = slip angle in radians
        //   F_z = vertical load on tire (N)
        //   C_α = tire cornering stiffness coefficient (N/rad per N of load)
        // 
        // Using REAR_TIRE_STIFFNESS_COEFFICIENT = 15.0 N/(rad·N)
        // This is an empirical value tuned for realistic behavior.
        double calc_rear_lat_force = rear_slip_angle * avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;

        // Step 3: Safety Clamp (Prevent physics explosions)
        // Clamp to ±MAX_REAR_LATERAL_FORCE (6000 N) to prevent unrealistic forces
        // during extreme conditions (e.g., spins, collisions, teleports).
        // Without this clamp, slip angle spikes could saturate FFB or cause oscillations.
        calc_rear_lat_force = (std::max)(-MAX_REAR_LATERAL_FORCE, (std::min)(MAX_REAR_LATERAL_FORCE, calc_rear_lat_force));

        // Step 4: Convert to Torque and Apply to SoP
        // Scale from Newtons to Newton-meters for torque output.
        // Coefficient was tuned to produce ~3.0 Nm contribution at 3000N lateral force (v0.4.11).
        // This provides a distinct counter-steering cue.
        // Multiplied by m_rear_align_effect to allow user tuning of rear-end sensitivity.
        // v0.4.19: INVERTED to provide counter-steering (restoring) torque instead of destabilizing force
        // When rear slides left (+slip), we want left pull (-torque) to correct the slide
        double rear_torque = -calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect * decoupling_scale; 
        sop_total += rear_torque;

        // --- 2b. Yaw Acceleration Injector (The "Kick") ---
        // Reads rotational acceleration (radians/sec^2)
        // 
        // v0.4.18 FIX: Apply Low Pass Filter to prevent noise feedback loop
        // PROBLEM: Slide Rumble injects high-frequency vibrations -> Yaw Accel spikes (derivatives are noise-sensitive)
        //          -> Yaw Kick amplifies the noise -> Wheel shakes harder -> Feedback loop
        // SOLUTION: Smooth the yaw acceleration to filter out high-frequency noise while keeping low-frequency signal
        double raw_yaw_accel = data->mLocalRotAccel.y;
        
        // v0.4.42: Signal Conditioning - Eliminate idle jitter and road noise
        // Low Speed Cutoff: Mute below 5 m/s (18 kph) to prevent parking lot jitter
        if (car_v_long < 5.0) {
            raw_yaw_accel = 0.0;
        }
        // Noise Gate (Deadzone): Filter out micro-corrections and road bumps
        // Real slides generate >> 2.0 rad/s², road noise is typically < 0.2 rad/s²
        else if (std::abs(raw_yaw_accel) < 0.2) {
            raw_yaw_accel = 0.0;
        }
        
        // Apply Smoothing (Low Pass Filter)
        // Yaw Kick Smoothing (LPF): Prevents "Slide Texture" vibration (40-200Hz) from being 
        // misinterpreted by physics as Yaw Acceleration spikes, which causes feedback loops.
        // - 31.8ms (5Hz): Car body motion; too laggy.
        // - 22.5ms (7Hz): Aggressive; turns sharp "Kick" into soft "Push", delays reaction.
        // - 10.0ms (New Default, ~16Hz): Optimal balance; responsive and filters the 40Hz+ vibration.
        // - 3.2ms (50Hz): "Raw" feel; kills electrical buzz but risks feedback loops.
        double tau_yaw = (double)m_yaw_accel_smoothing;
        if (tau_yaw < 0.0001) tau_yaw = 0.0001; 
        double alpha_yaw = dt / (tau_yaw + dt);
        
        m_yaw_accel_smoothed = m_yaw_accel_smoothed + alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
        
        // Use SMOOTHED value for the kick
        // Scaled by BASE_NM_YAW_KICK (5.0 Nm at Gain 1.0)
        // Added AFTER Lateral G Boost (Slide) to provide a clean, independent cue.
        // v0.4.20 FIX: Invert to provide counter-steering torque
        // Positive yaw accel (right rotation) -> Negative force (left pull)
        double yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK * decoupling_scale;
        sop_total += yaw_force;
        
        double total_force = output_force + sop_total;

        // --- 2c. Synthetic Gyroscopic Damping (v0.4.17) ---
        // Calculate Steering Angle (Radians)
        float range = data->mPhysicalSteeringWheelRange;
        if (range <= 0.0f) range = (float)DEFAULT_STEERING_RANGE_RAD; // Fallback 540 deg
        
        double steer_angle = data->mUnfilteredSteering * (range / 2.0);
        
        // Calculate Velocity (rad/s)
        double steer_vel = (steer_angle - m_prev_steering_angle) / dt;
        m_prev_steering_angle = steer_angle; // Update history
        
        // Smoothing (LPF)
        // v0.5.8: m_gyro_smoothing is now a Time Constant (Seconds)
        double tau_gyro = (double)m_gyro_smoothing;
        if (tau_gyro < 0.0001) tau_gyro = 0.0001;
        double alpha_gyro = dt / (tau_gyro + dt);
        
        m_steering_velocity_smoothed += alpha_gyro * (steer_vel - m_steering_velocity_smoothed);
        
        // Damping Force: Opposes velocity, scales with car speed
        double gyro_force = -1.0 * m_steering_velocity_smoothed * m_gyro_gain * (car_speed / GYRO_SPEED_SCALE) * decoupling_scale;
        
        // Add to total
        total_force += gyro_force;
        
        // --- Helper: Calculate Slip Data (Approximation) ---
        // The new LMU interface does not expose mSlipRatio/mSlipAngle directly.
        // We approximate them from mLongitudinalPatchVel and mLateralPatchVel.
        
        // Slip Ratio = PatchVelLong / GroundVelLong
        // Slip Angle = atan(PatchVelLat / GroundVelLong)
        
        double car_speed_ms = std::abs(data->mLocalVel.z); // Or mLongitudinalGroundVel per wheel
        
        auto get_slip_ratio = [&](const TelemWheelV01& w) {
            // v0.4.5: Option to use manual calculation
            if (m_use_manual_slip) {
                return calculate_manual_slip_ratio(w, data->mLocalVel.z);
            }
            // Default Game Data
            double v_long = std::abs(w.mLongitudinalGroundVel);
            if (v_long < MIN_SLIP_ANGLE_VELOCITY) v_long = MIN_SLIP_ANGLE_VELOCITY;
            return w.mLongitudinalPatchVel / v_long;
        };
        
        // get_slip_angle was moved up for grip approximation reuse

        // --- 2b. Progressive Lockup (Front & Rear with Differentiation) ---
        if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
            // 1. Calculate Slip Ratios for all wheels
            double slip_fl = get_slip_ratio(data->mWheel[0]);
            double slip_fr = get_slip_ratio(data->mWheel[1]);
            double slip_rl = get_slip_ratio(data->mWheel[2]);
            double slip_rr = get_slip_ratio(data->mWheel[3]);

            // 2. Find worst slip per axle (Slip is negative during braking, so use min)
            double max_slip_front = (std::min)(slip_fl, slip_fr);
            double max_slip_rear  = (std::min)(slip_rl, slip_rr);

            // 3. Determine dominant lockup source
            double effective_slip = 0.0;
            double freq_multiplier = 1.0; // Default to Front (High Pitch)

            // Check if Rear is locking up worse than Front
            // (e.g. Rear -0.5 vs Front -0.1)
            if (max_slip_rear < max_slip_front) {
                effective_slip = max_slip_rear;
                freq_multiplier = LOCKUP_FREQ_MULTIPLIER_REAR; // Lower pitch for Rear -> "Heavy Judder"
            } else {
                effective_slip = max_slip_front;
                freq_multiplier = 1.0; // Standard pitch for Front -> "Screech"
            }

            // 4. Generate Effect
            if (effective_slip < -0.1) {
                double severity = (std::abs(effective_slip) - 0.1) / 0.4;
                severity = (std::min)(1.0, severity);
                
                // Base Frequency linked to Car Speed
                double base_freq = 10.0 + (car_speed_ms * 1.5); 
                
                // Apply Axle Differentiation
                double final_freq = base_freq * freq_multiplier;

                // Phase Integration
                m_lockup_phase += final_freq * dt * TWO_PI;
                m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI);

                // Amplitude
                double amp = severity * m_lockup_gain * 4.0 * decoupling_scale;
                
                // Boost rear amplitude to emphasize danger
                if (freq_multiplier < 1.0) amp *= LOCKUP_AMPLITUDE_BOOST_REAR;

                lockup_rumble = std::sin(m_lockup_phase) * amp;
                total_force += lockup_rumble;
            }
        }

        // --- 2c. Wheel Spin (Tire Physics Based) ---
        if (m_spin_enabled && data->mUnfilteredThrottle > 0.05) {
            double slip_rl = get_slip_ratio(data->mWheel[2]); // mWheel
            double slip_rr = get_slip_ratio(data->mWheel[3]); // mWheel
            double max_slip = (std::max)(slip_rl, slip_rr);
            
            if (max_slip > 0.2) {
                double severity = (max_slip - 0.2) / 0.5;
                severity = (std::min)(1.0, severity);
                
                // 1. Torque Drop (Floating feel)
                total_force *= (1.0 - (severity * m_spin_gain * 0.6)); 

                // 2. Vibration Frequency: Based on SLIP SPEED (Not RPM)
                // Calculate how fast the tire surface is moving relative to the road.
                // Slip Speed (m/s) approx = Car Speed (m/s) * Slip Ratio
                double slip_speed_ms = car_speed_ms * max_slip;

                // Mapping:
                // 2 m/s (~7kph) slip -> 15Hz (Judder/Grip fighting)
                // 20 m/s (~72kph) slip -> 60Hz (Smooth spin)
                double freq = 10.0 + (slip_speed_ms * 2.5);
                
                // Cap frequency to prevent ultrasonic feeling on high speed burnouts
                if (freq > 80.0) freq = 80.0;

                // PHASE ACCUMULATION (FIXED)
                m_spin_phase += freq * dt * TWO_PI;
                m_spin_phase = std::fmod(m_spin_phase, TWO_PI); // Wrap correctly

                // Base Sinusoid
                double amp = severity * m_spin_gain * (double)BASE_NM_SPIN_VIBRATION * decoupling_scale; // Scaled for Nm (was 500)
                spin_rumble = std::sin(m_spin_phase) * amp;
                
                total_force += spin_rumble;
            }
        }

        // --- 3. Slide Texture (Dynamic Sawtooth) ---
        if (m_slide_texture_enabled) {
            // New logic: Use mLateralPatchVel directly instead of Angle
            // This is cleaner as it represents actual scrubbing speed.
            
            // Front Slip Speed
            double lat_vel_fl = std::abs(fl.mLateralPatchVel);
            double lat_vel_fr = std::abs(fr.mLateralPatchVel);
            double front_slip_avg = (lat_vel_fl + lat_vel_fr) / 2.0;

            // Rear Slip Speed (New v0.4.34)
            double lat_vel_rl = std::abs(data->mWheel[2].mLateralPatchVel);
            double lat_vel_rr = std::abs(data->mWheel[3].mLateralPatchVel);
            double rear_slip_avg = (lat_vel_rl + lat_vel_rr) / 2.0;

            // Use the WORST slip (Max)
            // This ensures we feel understeer (Front) AND oversteer/drifting (Rear)
            double effective_slip_vel = (std::max)(front_slip_avg, rear_slip_avg);
            
            if (effective_slip_vel > 0.5) {
                
                // BASE FORMULA (v0.4.36): 10Hz start, +5Hz per m/s. (Rumble optimized for scale 1.0)
                double base_freq = 10.0 + (effective_slip_vel * 5.0);
                
                // APPLY USER SCALE
                double freq = base_freq * (double)m_slide_freq_scale;
                
                // CAP AT 250Hz (Nyquist safety for 400Hz loop)
                if (freq > 250.0) freq = 250.0;

                // PHASE ACCUMULATION (CRITICAL FIX)
                // Use fmod to handle large dt spikes safely
                m_slide_phase += freq * dt * TWO_PI;
                m_slide_phase = std::fmod(m_slide_phase, TWO_PI);

                // Sawtooth wave
                double sawtooth = (m_slide_phase / TWO_PI) * 2.0 - 1.0;

                // Amplitude: Scaled by PRE-CALCULATED global load_factor
                // v0.4.38: Work-Based Scrubbing
                // Scale by Load * (1.0 - Grip). Scrubbing happens when grip is LOST.
                // High Load + Low Grip = Max Vibration.
                // We use avg_grip (from understeer calc) which includes longitudinal slip.
                double grip_scale = (std::max)(0.0, 1.0 - avg_grip);
                // Resulting force
                slide_noise = sawtooth * m_slide_texture_gain * (double)BASE_NM_SLIDE_TEXTURE * load_factor * grip_scale * decoupling_scale;
                total_force += slide_noise;
            }
        }
        
        // --- 4. Road Texture (High Pass Filter) ---
        if (m_road_texture_enabled) {
            // Scrub Drag (v0.4.5)
            // Add resistance when sliding laterally (Dragging rubber)
            if (m_scrub_drag_gain > 0.0) {
                double avg_lat_vel = (fl.mLateralPatchVel + fr.mLateralPatchVel) / 2.0;
                // v0.4.6: Linear Fade-In Window (0.0 - 0.5 m/s)
                double abs_lat_vel = std::abs(avg_lat_vel);
                if (abs_lat_vel > 0.001) { // Avoid noise
                    double fade = (std::min)(1.0, abs_lat_vel / 0.5);
                    // v0.4.20 FIX: Provide counter-steering (stabilizing) torque
                    // Game: +X = Left, DirectInput: +Force = Right
                    // If sliding left (+vel), we want left torque (-force) to resist the slide
                    double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;
                    // 3. Final force calculation
                    scrub_drag_force = drag_dir * m_scrub_drag_gain * (double)BASE_NM_SCRUB_DRAG * fade * decoupling_scale; // Scaled & Faded
                    total_force += scrub_drag_force;
                }
            }

            // Use change in suspension deflection
            // 
            // TODO (v0.4.40 - Encrypted Content Gap A): Road Texture Fallback
            // If mVerticalTireDeflection is blocked (0.0) on encrypted content, the delta will be 0.0,
            // resulting in silent road texture (no bumps or curbs felt).
            // 
            // Risk: If mSuspensionDeflection is blocked, mVerticalTireDeflection and mRideHeight
            // are likely also blocked (same suspension physics packet).
            // 
            // Potential Fix: If deflection is static/zero while car is moving, fallback to using
            // Vertical G-Force (mLocalAccel.y) through a high-pass filter to generate road noise.
            // See: docs/dev_docs/Improving FFB App Tyres.md "Gap A: Road Texture"
            double vert_l = fl.mVerticalTireDeflection;
            double vert_r = fr.mVerticalTireDeflection;
            
            // Delta from previous frame
            double delta_l = vert_l - m_prev_vert_deflection[0];
            double delta_r = vert_r - m_prev_vert_deflection[1];
            
            // v0.4.6: Delta Clamping (+/- 0.01m)
            delta_l = (std::max)(-0.01, (std::min)(0.01, delta_l));
            delta_r = (std::max)(-0.01, (std::min)(0.01, delta_r));

            // Amplify sudden changes
            double road_noise_val = (delta_l + delta_r) * 50.0 * m_road_texture_gain * decoupling_scale; // Scaled for Nm (was 5000)
            
            // Apply LOAD FACTOR: Bumps feel harder under compression
            road_noise = road_noise_val * load_factor;
            
            total_force += road_noise;
        }

        // --- 5. Suspension Bottoming (High Load Impulse) ---
        if (m_bottoming_enabled) {
            bool triggered = false;
            double intensity = 0.0;

            if (m_bottoming_method == 0) {
                // Method A: Scraping (Ride Height)
                // 
                // TODO (v0.4.40 - Encrypted Content Gap B): Bottoming False Positive
                // If mRideHeight is blocked (0.0) on encrypted content, the check `min_rh < 0.002`
                // will be constantly true, causing permanent scraping vibration.
                // 
                // Risk: If mSuspensionDeflection is blocked, mRideHeight is likely also blocked
                // (same suspension physics packet).
                // 
                // Potential Fix: Add sanity check - if mRideHeight is exactly 0.0 while car is moving
                // (physically impossible), disable Method A or switch to Method B.
                // See: docs/dev_docs/Improving FFB App Tyres.md "Gap B: Bottoming Effect"
                // Threshold: 2mm (0.002m)
                double min_rh = (std::min)(fl.mRideHeight, fr.mRideHeight);
                if (min_rh < 0.002 && min_rh > -1.0) { // Check valid range
                    triggered = true;
                    // Closer to 0 = stronger. Map 0.002->0.0 to 0.0->1.0 intensity
                    intensity = (0.002 - min_rh) / 0.002;
                }
            } else {
                // Method B: Suspension Force Spike (Derivative)
                double susp_l = fl.mSuspForce;
                double susp_r = fr.mSuspForce;
                double dForceL = (susp_l - m_prev_susp_force[0]) / dt;
                double dForceR = (susp_r - m_prev_susp_force[1]) / dt;
                
                double max_dForce = (std::max)(dForceL, dForceR);
                // Threshold: 100,000 N/s
                if (max_dForce > 100000.0) {
                    triggered = true;
                    intensity = (max_dForce - 100000.0) / 200000.0; // Scale
                }
            }
            
            // Legacy/Fallback check: High Load
            if (!triggered) {
                double max_load = (std::max)(fl.mTireLoad, fr.mTireLoad);
                if (max_load > 8000.0) {
                    triggered = true;
                    double excess = max_load - 8000.0;
                    intensity = std::sqrt(excess) * 0.05; // Tuned
                }
            }

            if (triggered) {
                // Non-linear response (Square root softens the initial onset)
                double bump_magnitude = intensity * m_bottoming_gain * (double)BASE_NM_BOTTOMING * decoupling_scale; // Scaled for Nm
                
                // FIX: Use a 50Hz "Crunch" oscillation instead of directional DC offset
                double freq = 50.0; 
                
                // Phase Integration (FIXED)
                m_bottoming_phase += freq * dt * TWO_PI;
                m_bottoming_phase = std::fmod(m_bottoming_phase, TWO_PI); // Wrap correctly

                // Generate vibration (Sine wave)
                // This creates a heavy shudder regardless of steering direction
                double crunch = std::sin(m_bottoming_phase) * bump_magnitude;
                
                total_force += crunch;
            }
        }

        // --- 6. Min Force & Output Scaling ---
        // Boost small forces to overcome wheel friction
        // Use the configurable reference instead of hardcoded 20.0 (v0.4.4 Fix)
        double max_force_ref = (double)m_max_torque_ref; 
        
        // Safety: Prevent divide by zero
        if (max_force_ref < 1.0) max_force_ref = 1.0;

        double norm_force = total_force / max_force_ref;
        
        // Apply Master Gain
        norm_force *= m_gain;
        
        // Apply Min Force
        // If force is non-zero but smaller than min_force, boost it.
        if (std::abs(norm_force) > 0.0001 && std::abs(norm_force) < m_min_force) {
            // Sign check
            double sign = (norm_force > 0.0) ? 1.0 : -1.0;
            norm_force = sign * m_min_force;
        }
        
        // APPLY INVERSION HERE (Before clipping)
        if (m_invert_force) {
            norm_force *= -1.0;
        }
        
        // ==================================================================================
        // CRITICAL: UNCONDITIONAL STATE UPDATES (Fix for Toggle Spikes)
        // ==================================================================================
        // We must update history variables every frame, even if effects are disabled.
        // This prevents "stale state" spikes when effects are toggled on.
        
        // Road Texture State
        m_prev_vert_deflection[0] = fl.mVerticalTireDeflection;
        m_prev_vert_deflection[1] = fr.mVerticalTireDeflection;

        // Bottoming Method B State
        m_prev_susp_force[0] = fl.mSuspForce;
        m_prev_susp_force[1] = fr.mSuspForce;
        // ==================================================================================

        // --- SNAPSHOT LOGIC ---
        // Capture all internal states for visualization
        {
            std::lock_guard<std::mutex> lock(m_debug_mutex);
            if (m_debug_buffer.size() < 100) {
                FFBSnapshot snap;
                
                // --- Header A: Outputs ---
                snap.total_output = (float)norm_force;
                snap.base_force = (float)base_input; // Show the processed base input
                snap.sop_force = (float)sop_base_force;
                snap.understeer_drop = (float)((base_input * m_steering_shaft_gain) * (1.0 - grip_factor));
                snap.oversteer_boost = (float)(sop_total - sop_base_force - rear_torque - yaw_force); // Split boost from other SoP components
                snap.ffb_rear_torque = (float)rear_torque;
                snap.ffb_scrub_drag = (float)scrub_drag_force;
                snap.ffb_yaw_kick = (float)yaw_force;
                snap.ffb_gyro_damping = (float)gyro_force; // New v0.4.17
                snap.texture_road = (float)road_noise;
                snap.texture_slide = (float)slide_noise;
                snap.texture_lockup = (float)lockup_rumble;
                snap.texture_spin = (float)spin_rumble;
                snap.texture_bottoming = (float)bottoming_crunch;
                snap.clipping = (std::abs(norm_force) > 0.99f) ? 1.0f : 0.0f;
                
                // --- Header B: Internal Physics (Calculated) ---
                snap.calc_front_load = (float)avg_load; // This is the final load used (maybe approximated)
                snap.calc_rear_load = (float)avg_rear_load; // New v0.4.10
                snap.calc_rear_lat_force = (float)calc_rear_lat_force; // New v0.4.10
                snap.calc_front_grip = (float)avg_grip; // This is the final grip used (maybe approximated)
                snap.calc_rear_grip = (float)avg_rear_grip;
                snap.calc_front_slip_ratio = (float)((calculate_manual_slip_ratio(fl, data->mLocalVel.z) + calculate_manual_slip_ratio(fr, data->mLocalVel.z)) / 2.0);
                snap.calc_front_slip_angle_smoothed = (float)m_grip_diag.front_slip_angle; // Smoothed Slip Angle
                snap.calc_rear_slip_angle_smoothed = (float)m_grip_diag.rear_slip_angle; // Smoothed Rear Slip Angle
                
                // Calculate Raw Slip Angles for visualization (v0.4.9 Refactored)
                snap.raw_front_slip_angle = (float)calculate_raw_slip_angle_pair(fl, fr);
                snap.raw_rear_slip_angle = (float)calculate_raw_slip_angle_pair(data->mWheel[2], data->mWheel[3]);

                // Helper for Raw Game Slip Ratio
                auto get_raw_game_slip = [&](const TelemWheelV01& w) {
                    double v_long = std::abs(w.mLongitudinalGroundVel);
                    if (v_long < MIN_SLIP_ANGLE_VELOCITY) v_long = MIN_SLIP_ANGLE_VELOCITY;
                    return w.mLongitudinalPatchVel / v_long;
                };

                // --- Header C: Raw Game Telemetry (Inputs) ---
                snap.steer_force = (float)raw_torque;
                snap.raw_input_steering = (float)data->mUnfilteredSteering;
                snap.raw_front_tire_load = (float)raw_load; // Raw from game
                snap.raw_front_grip_fract = (float)raw_grip; // Raw from game
                snap.raw_rear_grip = (float)((data->mWheel[2].mGripFract + data->mWheel[3].mGripFract) / 2.0);
                snap.raw_front_susp_force = (float)((fl.mSuspForce + fr.mSuspForce) / 2.0);
                snap.raw_front_ride_height = (float)((std::min)(fl.mRideHeight, fr.mRideHeight));
                snap.raw_rear_lat_force = (float)((data->mWheel[2].mLateralForce + data->mWheel[3].mLateralForce) / 2.0);
                snap.raw_car_speed = (float)data->mLocalVel.z;
                snap.raw_front_slip_ratio = (float)((get_raw_game_slip(fl) + get_raw_game_slip(fr)) / 2.0);
                snap.raw_input_throttle = (float)data->mUnfilteredThrottle;
                snap.raw_input_brake = (float)data->mUnfilteredBrake;
                snap.accel_x = (float)data->mLocalAccel.x;
                snap.raw_front_lat_patch_vel = (float)((std::abs(fl.mLateralPatchVel) + std::abs(fr.mLateralPatchVel)) / 2.0);
                snap.raw_front_deflection = (float)((fl.mVerticalTireDeflection + fr.mVerticalTireDeflection) / 2.0);
                
                // New Patch Velocities (v0.4.9)
                snap.raw_front_long_patch_vel = (float)((fl.mLongitudinalPatchVel + fr.mLongitudinalPatchVel) / 2.0);
                snap.raw_rear_lat_patch_vel = (float)((std::abs(data->mWheel[2].mLateralPatchVel) + std::abs(data->mWheel[3].mLateralPatchVel)) / 2.0);
                snap.raw_rear_long_patch_vel = (float)((data->mWheel[2].mLongitudinalPatchVel + data->mWheel[3].mLongitudinalPatchVel) / 2.0);

                // Warnings
                snap.warn_load = frame_warn_load;
                snap.warn_grip = frame_warn_grip || frame_warn_rear_grip; // Combined warning
                snap.warn_dt = frame_warn_dt;
                snap.debug_freq = (float)m_debug_freq;
                snap.tire_radius = (float)fl.mStaticUndeflectedRadius / 100.0f; // Convert cm to m


                m_debug_buffer.push_back(snap);
            }
        }

        // Clip
        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
};

#endif // FFBENGINE_H
