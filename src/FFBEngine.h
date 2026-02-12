#ifndef FFBENGINE_H
#define FFBENGINE_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>
#include <array>
#include <cstring>
#include "lmu_sm_interface/InternalsPluginWrapper.h"
#include "AsyncLogger.h"

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
    float slope_current; // New v0.7.1: Slope detection derivative value
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

namespace FFBEngineTests { class FFBEngineTestAccess; }

// Calculation Context Struct
// Holds common derived values used across multiple effect calculations
struct FFBCalculationContext {
    double dt = 0.0025;
    double car_speed = 0.0;       // Absolute m/s
    double car_speed_long = 0.0;  // Longitudinal m/s (Raw)
    double decoupling_scale = 1.0;
    double speed_gate = 1.0;
    double texture_load_factor = 1.0;
    double brake_load_factor = 1.0;
    double avg_load = 0.0;
    double avg_grip = 0.0;

    // Diagnostics
    bool frame_warn_load = false;
    bool frame_warn_grip = false;
    bool frame_warn_rear_grip = false;
    bool frame_warn_dt = false;

    // Intermediate results
    double grip_factor = 1.0;     // 1.0 = full grip, 0.0 = no grip
    double sop_base_force = 0.0;
    double sop_unboosted_force = 0.0; // For snapshot compatibility
    double rear_torque = 0.0;
    double yaw_force = 0.0;
    double scrub_drag_force = 0.0;
    double gyro_force = 0.0;
    double avg_rear_grip = 0.0;
    double calc_rear_lat_force = 0.0;
    double avg_rear_load = 0.0;

    // Effect outputs
    double road_noise = 0.0;
    double slide_noise = 0.0;
    double lockup_rumble = 0.0;
    double spin_rumble = 0.0;
    double bottoming_crunch = 0.0;
    double abs_pulse_force = 0.0;
    double gain_reduction_factor = 1.0;
};
    
// FFB Engine Class
class FFBEngine {
    // ⚠ IMPORTANT MAINTENANCE WARNING:
    // When adding new FFB parameters to this class, you MUST also update:
    // 1. Preset struct in Config.h
    // 2. Preset::Apply(), UpdateFromEngine(), and Equals() in Config.h
    // 3. Config::Save() and Config::Load() in Config.cpp

public:
    // Settings (GUI Sliders)
    // NOTE: These are initialized by Preset::ApplyDefaultsToEngine() in the constructor
    // to maintain a single source of truth in Config.h (Preset struct defaults)
    float m_gain;
    float m_understeer_effect;
    float m_sop_effect;
    float m_min_force;
    
    // Configurable Smoothing & Caps (v0.3.9)
    float m_sop_smoothing_factor;
    float m_texture_load_cap = 1.5f; // Renamed from m_max_load_factor (v0.5.11)
    float m_brake_load_cap = 1.5f;   // New v0.5.11
    float m_sop_scale;
    
    // v0.4.4 Features
    float m_max_torque_ref;
    bool m_invert_force;
    
    // Base Force Debugging (v0.4.13)
    float m_steering_shaft_gain;
    int m_base_force_mode;

    // New Effects (v0.2)
    float m_oversteer_boost;
    float m_rear_align_effect;
    float m_sop_yaw_gain;
    float m_gyro_gain;
    float m_gyro_smoothing;
    float m_yaw_accel_smoothing;
    float m_chassis_inertia_smoothing;
    
    bool m_lockup_enabled;
    float m_lockup_gain;
    // NEW Lockup Tuning (v0.5.11)
    float m_lockup_start_pct = 5.0f;
    float m_lockup_full_pct = 15.0f;
    float m_lockup_rear_boost = 1.5f;
    float m_lockup_gamma = 2.0f;           // New v0.6.0
    float m_lockup_prediction_sens = 50.0f; // New v0.6.0
    float m_lockup_bump_reject = 1.0f;     // New v0.6.0
    
    bool m_abs_pulse_enabled = true;      // New v0.6.0
    float m_abs_gain = 1.0f;               // New v0.6.0
    
    bool m_spin_enabled;
    float m_spin_gain;

    // Texture toggles
    bool m_slide_texture_enabled;
    float m_slide_texture_gain;
    float m_slide_freq_scale;
    
    bool m_road_texture_enabled;
    float m_road_texture_gain;
    
    // Bottoming Effect (v0.3.2)
    bool m_bottoming_enabled = true;  // Keep this as it's not in presets
    float m_bottoming_gain = 1.0f;    // Keep this as it's not in presets

    float m_slip_angle_smoothing;
    
    // NEW: Grip Estimation Settings (v0.5.7)
    float m_optimal_slip_angle;
    float m_optimal_slip_ratio;
    
    // NEW: Steering Shaft Smoothing (v0.5.7)
    float m_steering_shaft_smoothing;
    
    // v0.4.41: Signal Filtering Settings
    bool m_flatspot_suppression = false;
    float m_notch_q = 2.0f; // Default Q-Factor
    float m_flatspot_strength = 1.0f; // Default 1.0 (100% suppression)
    
    // Static Notch Filter (v0.4.43)
    bool m_static_notch_enabled = false;
    float m_static_notch_freq = 11.0f;
    float m_static_notch_width = 2.0f; // New v0.6.10: Width in Hz
    float m_yaw_kick_threshold = 0.2f; // New v0.6.10: Threshold in rad/s^2 (Default 0.2 matching legacy gate)

    // v0.6.23: User-Adjustable Speed Gate
    // CHANGED DEFAULTS:
    // Lower: 1.0 m/s (3.6 km/h) - Start fading in
    // Upper: 5.0 m/s (18.0 km/h) - Full strength / End smoothing
    // This ensures the "Violent Shaking" (< 15km/h) is covered by default.
    float m_speed_gate_lower = 1.0f; 
    float m_speed_gate_upper = 5.0f; 

    // v0.6.23: Additional Advanced Physics (Reserved for future use)
    // These settings are declared and persist in config/presets but are not yet
    // implemented in the calculate_force() logic. They will be activated in a future release.
    float m_road_fallback_scale = 0.05f;
    bool m_understeer_affects_sop = false;
    
    // ===== SLOPE DETECTION (v0.7.0 → v0.7.3 stability fixes) =====
    bool m_slope_detection_enabled = false;
    int m_slope_sg_window = 15;
    float m_slope_sensitivity = 0.5f;            // v0.7.1: Reduced from 1.0 (less aggressive)
    float m_slope_negative_threshold = -0.3f;    // v0.7.1: Changed from -0.1 (later trigger)
    float m_slope_smoothing_tau = 0.04f;         // v0.7.1: Changed from 0.02 (smoother transitions)

    // NEW v0.7.3: Stability fixes
    float m_slope_alpha_threshold = 0.02f;    // NEW: Minimum dAlpha/dt to calculate slope
    float m_slope_decay_rate = 5.0f;          // NEW: Decay rate toward 0 when not cornering
    bool m_slope_confidence_enabled = true;   // NEW: Enable confidence-based grip scaling

    // NEW v0.7.11: Min/Max Threshold System
    float m_slope_min_threshold = -0.3f;   // Effect starts here (dead zone edge)
    float m_slope_max_threshold = -2.0f;   // Effect saturates here (100%)

    // Signal Diagnostics
    double m_debug_freq = 0.0; // Estimated frequency for GUI
    double m_theoretical_freq = 0.0; // Theoretical wheel frequency for GUI

    // Warning States (Console logging)
    bool m_warned_load = false;
    bool m_warned_grip = false;
    bool m_warned_rear_grip = false; // v0.4.5 Fix
    bool m_warned_dt = false;
    bool m_warned_lat_force_front = false;
    bool m_warned_lat_force_rear = false;
    bool m_warned_susp_force = false;
    bool m_warned_susp_deflection = false;
    bool m_warned_vert_deflection = false; // v0.6.21
    
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
    int m_missing_lat_force_front_frames = 0;
    int m_missing_lat_force_rear_frames = 0;
    int m_missing_susp_force_frames = 0;
    int m_missing_susp_deflection_frames = 0;
    int m_missing_vert_deflection_frames = 0; // v0.6.21

    // Internal state
    double m_prev_vert_deflection[4] = {0.0, 0.0, 0.0, 0.0}; // FL, FR, RL, RR
    double m_prev_vert_accel = 0.0; // New v0.6.21: For Road Texture Fallback
    double m_prev_slip_angle[4] = {0.0, 0.0, 0.0, 0.0}; // FL, FR, RL, RR (LPF State)
    double m_prev_rotation[4] = {0.0, 0.0, 0.0, 0.0};    // New v0.6.0
    double m_prev_brake_pressure[4] = {0.0, 0.0, 0.0, 0.0}; // New v0.6.0
    
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
    double m_abs_phase = 0.0; // New v0.6.0
    double m_bottoming_phase = 0.0;
    
    // Phase Accumulators for Dynamic Oscillators (v0.6.20)
    float m_abs_freq_hz = 20.0f;
    float m_lockup_freq_scale = 1.0f;
    float m_spin_freq_scale = 1.0f;
    
    // Internal state for Bottoming (Method B)
    double m_prev_susp_force[2] = {0.0, 0.0}; // FL, FR

    // New Settings (v0.4.5)
    int m_bottoming_method = 0; // 0=Scraping (Default), 1=Suspension Spike
    float m_scrub_drag_gain; // Initialized by Preset::ApplyDefaultsToEngine()

    // Smoothing State
    double m_sop_lat_g_smoothed = 0.0;
    
    // Filter Instances (v0.4.41)
    // Filter Instances (v0.4.41)
    BiquadNotch m_notch_filter;
    BiquadNotch m_static_notch_filter;

    // Slope Detection Buffers (Circular) - v0.7.0
    static constexpr int SLOPE_BUFFER_MAX = 41;  // Max window size supported
    std::array<double, SLOPE_BUFFER_MAX> m_slope_lat_g_buffer = {};
    std::array<double, SLOPE_BUFFER_MAX> m_slope_slip_buffer = {};
    int m_slope_buffer_index = 0;
    int m_slope_buffer_count = 0;

    // Slope Detection State (Public for diagnostics) - v0.7.0
    double m_slope_current = 0.0;
    double m_slope_grip_factor = 1.0;
    double m_slope_smoothed_output = 1.0;
    
    // Context for Logging (v0.7.x)
    char m_vehicle_name[64] = "Unknown";
    char m_track_name[64] = "Unknown";

    // Logging intermediate values (exposed for AsyncLogger)
    double m_slope_dG_dt = 0.0;       // Last calculated dG/dt
    double m_slope_dAlpha_dt = 0.0;   // Last calculated dAlpha/dt

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
    
    // Friend class for unit testing private helper methods
    friend class FFBEngineTests::FFBEngineTestAccess;

    FFBEngine();
    
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
    
    // Axle Differentiation Hysteresis (v0.5.13)
    // Prevents rapid switching between front/rear lockup modes due to sensor noise.
    // Rear lockup is only triggered when rear slip exceeds front slip by this margin (1% slip).
    static constexpr double AXLE_DIFF_HYSTERESIS = 0.01;  // 1% slip buffer to prevent mode chattering
    
    // ABS Detection Thresholds (v0.6.0)
    // These constants control when the ABS pulse effect is triggered.
    static constexpr double ABS_PEDAL_THRESHOLD = 0.5;  // 50% pedal input required to detect ABS
    static constexpr double ABS_PRESSURE_RATE_THRESHOLD = 2.0;  // bar/s pressure modulation rate
    
    // Predictive Lockup Gating Thresholds (v0.6.0)
    // These constants define the conditions under which predictive logic is enabled.
    static constexpr double PREDICTION_BRAKE_THRESHOLD = 0.02;  // 2% brake deadzone
    static constexpr double PREDICTION_LOAD_THRESHOLD = 50.0;   // 50N minimum tire load (not airborne)


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
                              double dt,
                              const char* vehicleName,
                              const TelemInfoV01* data,
                              bool is_front = true) {
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
                if (m_slope_detection_enabled && is_front && data) {
                    // Dynamic grip estimation via derivative monitoring
                    result.value = calculate_slope_grip(
                        data->mLocalAccel.x / 9.81,  // Lateral G
                        result.slip_angle,            // Slip angle (radians)
                        dt
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
                        // Sigmoid-like drop-off: 1 / (1 + 2x)
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
        // - Source: docs/dev_docs/references/Reference - coordinate_system_reference.md
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

    // Helper: Calculate Savitzky-Golay First Derivative - v0.7.0
    // Uses closed-form coefficient generation for quadratic polynomial fit.
    // Reference: docs/dev_docs/plans/savitzky-golay coefficients deep research report.md
    double calculate_sg_derivative(const std::array<double, SLOPE_BUFFER_MAX>& buffer, 
                                   int count, int window, double dt) {
        // Ensure we have enough samples
        if (count < window) return 0.0;
        
        int M = window / 2;  // Half-width (e.g., window=15 -> M=7)
        
        // Calculate S_2 = M(M+1)(2M+1)/3
        double S2 = (double)M * (M + 1.0) * (2.0 * M + 1.0) / 3.0;
        
        // Correct Indexing (v0.7.0 Fix)
        // m_slope_buffer_index points to the next slot to write.
        // Latest sample is at (index - 1). Center is at (index - 1 - M).
        int latest_idx = (m_slope_buffer_index - 1 + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX;
        int center_idx = (latest_idx - M + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX;
        
        double sum = 0.0;
        for (int k = 1; k <= M; ++k) {
            int idx_pos = (center_idx + k + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX;
            int idx_neg = (center_idx - k + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX;
            
            // Weights for d=1 are simply k
            sum += (double)k * (buffer[idx_pos] - buffer[idx_neg]);
        }
        
        // Divide by dt to get derivative in units/second
        return sum / (S2 * dt);
    }

    // Helper: Calculate Grip Factor from Slope - v0.7.0
    // Main slope detection algorithm entry point
    double calculate_slope_grip(double lateral_g, double slip_angle, double dt) {
        // 1. Update Buffers
        m_slope_lat_g_buffer[m_slope_buffer_index] = lateral_g;
        m_slope_slip_buffer[m_slope_buffer_index] = std::abs(slip_angle);
        m_slope_buffer_index = (m_slope_buffer_index + 1) % SLOPE_BUFFER_MAX;
        if (m_slope_buffer_count < SLOPE_BUFFER_MAX) m_slope_buffer_count++;

        // 2. Calculate Derivatives
        // We need d(Lateral_G) / d(Slip_Angle)
        // By chain rule: dG/dAlpha = (dG/dt) / (dAlpha/dt)
        double dG_dt = calculate_sg_derivative(m_slope_lat_g_buffer, m_slope_buffer_count, m_slope_sg_window, dt);
        double dAlpha_dt = calculate_sg_derivative(m_slope_slip_buffer, m_slope_buffer_count, m_slope_sg_window, dt);

        // Store for logging
        m_slope_dG_dt = dG_dt;
        m_slope_dAlpha_dt = dAlpha_dt;

        // 3. Estimate Slope (dG/dAlpha)
        // v0.7.21 FIX: Denominator protection and gated calculation.
        // We calculate the physical slope when steering movement is sufficient,
        // and decay it toward zero otherwise to prevent "sticky" understeer.
        if (std::abs(dAlpha_dt) > (double)m_slope_alpha_threshold) {
            double abs_dAlpha = std::abs(dAlpha_dt);
            double sign_dAlpha = (dAlpha_dt >= 0) ? 1.0 : -1.0;
            double protected_denom = (std::max)(0.005, abs_dAlpha) * sign_dAlpha;
            m_slope_current = dG_dt / protected_denom;

            // v0.7.17 FIX: Hard clamp to physically possible range [-20.0, 20.0]
            m_slope_current = std::clamp(m_slope_current, -20.0, 20.0);
        } else {
            // Decay slope toward 0 when not actively cornering
            m_slope_current += (double)m_slope_decay_rate * dt * (0.0 - m_slope_current);
        }

        double current_grip_factor = 1.0;
        double confidence = calculate_slope_confidence(dAlpha_dt);

        // v0.7.11: InverseLerp based threshold mapping
        // m_slope_min_threshold: Effect starts (e.g., -0.3)
        // m_slope_max_threshold: Effect saturates (e.g., -2.0)
        double loss_percent = inverse_lerp((double)m_slope_min_threshold, (double)m_slope_max_threshold, m_slope_current);
        
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
    inline double calculate_slope_confidence(double dAlpha_dt) {
        if (!m_slope_confidence_enabled) return 1.0;

        // v0.7.21 FIX: Use smoothstep confidence ramp [m_slope_alpha_threshold, 0.10] rad/s
        // to reject singularity artifacts near zero.
        return smoothstep((double)m_slope_alpha_threshold, 0.10, std::abs(dAlpha_dt));
    }

    // Helper: Inverse linear interpolation - v0.7.11
    // Returns normalized position of value between min and max
    // Returns 0 if value >= min, 1 if value <= max (for negative threshold use)
    // Clamped to [0, 1] range
    inline double inverse_lerp(double min_val, double max_val, double value) {
        double range = max_val - min_val;
        if (std::abs(range) >= 0.0001) {
            double t = (value - min_val) / (std::abs(range) >= 0.0001 ? range : 1.0);
            return (std::max)(0.0, (std::min)(1.0, t));
        }
        
        // Degenerate case when range is zero or near-zero
        if (max_val >= min_val) return (value >= min_val) ? 1.0 : 0.0;
        return (value <= min_val) ? 1.0 : 0.0;
    }

    // Helper: Smoothstep interpolation - v0.7.2
    // Returns smooth S-curve interpolation from 0 to 1
    // Uses Hermite polynomial: t² × (3 - 2t)
    // Zero derivative at both endpoints for seamless transitions
    inline double smoothstep(double edge0, double edge1, double x) {
        double range = edge1 - edge0;
        if (std::abs(range) >= 0.0001) {
            double t = (x - edge0) / (std::abs(range) >= 0.0001 ? range : 1.0);
            t = (std::max)(0.0, (std::min)(1.0, t));
            return t * t * (3.0 - 2.0 * t);
        }
        return (x < edge0) ? 0.0 : 1.0;
    }

    // Helper: Calculate Slip Ratio from wheel (v0.6.36 - Extracted from lambdas)
    // Unified slip ratio calculation for lockup and spin detection.
    // Returns the ratio of longitudinal slip: (PatchVel - GroundVel) / GroundVel
    double calculate_wheel_slip_ratio(const TelemWheelV01& w) {
        double v_long = std::abs(w.mLongitudinalGroundVel);
        if (v_long < MIN_SLIP_ANGLE_VELOCITY) v_long = MIN_SLIP_ANGLE_VELOCITY;
        return w.mLongitudinalPatchVel / v_long;
    }

    // Refactored calculate_force
    double calculate_force(const TelemInfoV01* data) {
        if (!data) return 0.0;
        
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
        double raw_torque = data->mSteeringShaftTorque;
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
        
        // Load Factors
        double raw_load_factor = ctx.avg_load / 4000.0;
        double texture_safe_max = (std::min)(2.0, (double)m_texture_load_cap);
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
                                                   ctx.car_speed, ctx.dt, data->mVehicleName, data, true);
        ctx.avg_grip = front_grip_res.value;
        m_grip_diag.front_original = front_grip_res.original;
        m_grip_diag.front_approximated = front_grip_res.approximated;
        m_grip_diag.front_slip_angle = front_grip_res.slip_angle;
        if (front_grip_res.approximated) ctx.frame_warn_grip = true;

        // 2. Signal Conditioning (Smoothing, Notch Filters)
        double game_force_proc = apply_signal_conditioning(data->mSteeringShaftTorque, data, ctx);

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
        
        double output_force = (base_input * (double)m_steering_shaft_gain) * ctx.grip_factor;
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
        
        // --- 6. SUMMATION ---
        // Split into Structural (Attenuated by Spin) and Texture (Raw) groups
        double structural_sum = output_force + ctx.sop_base_force + ctx.rear_torque + ctx.yaw_force + ctx.gyro_force +
                                ctx.abs_pulse_force + ctx.lockup_rumble + ctx.scrub_drag_force;

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
            frame.slope_smoothed = (float)m_slope_smoothed_output;
            
            // Use extracted confidence calculation
            frame.confidence = (float)calculate_slope_confidence(m_slope_dAlpha_dt);
            
            // Rear axle
            frame.calc_grip_rear = (float)ctx.avg_rear_grip;
            frame.grip_delta = (float)(ctx.avg_grip - ctx.avg_rear_grip);
            
            // FFB output
            frame.ffb_total = (float)norm_force;
            frame.ffb_grip_factor = (float)ctx.grip_factor;
            frame.ffb_sop = (float)ctx.sop_base_force;
            frame.speed_gate = (float)ctx.speed_gate;
            frame.clipping = (std::abs(norm_force) > 0.99);
            frame.ffb_base = (float)base_input; // Plan included ffb_base
            
            AsyncLogger::Get().Log(frame);
        }
        
        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }

    // ========================================
    // Helper Methods (v0.6.36 Refactoring) - PUBLIC for testability
    // ========================================

    // Signal Conditioning: Applies idle smoothing and notch filters to raw torque
    // Returns the conditioned force value ready for effect processing
    double apply_signal_conditioning(double raw_torque, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        double game_force_proc = raw_torque;

        // Idle Smoothing
        double effective_shaft_smoothing = (double)m_steering_shaft_smoothing;
        double idle_speed_threshold = (double)m_speed_gate_upper;
        if (idle_speed_threshold < 3.0) idle_speed_threshold = 3.0;
        if (ctx.car_speed < idle_speed_threshold) {
            double idle_blend = (idle_speed_threshold - ctx.car_speed) / idle_speed_threshold;
            double dynamic_smooth = 0.1 * idle_blend; // 0.1s target
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

        // Dynamic Notch Filter
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

private:
    // Effect Helper Methods

    void calculate_sop_lateral(const TelemInfoV01* data, FFBCalculationContext& ctx) {
        // Lateral G
        double raw_g = (std::max)(-49.05, (std::min)(49.05, data->mLocalAccel.x));
        double lat_g = (raw_g / 9.81);
        
        // Smoothing
        double smoothness = 1.0 - (double)m_sop_smoothing_factor;
        smoothness = (std::max)(0.0, (std::min)(0.999, smoothness));
        double tau = smoothness * 0.1;
        double alpha = ctx.dt / (tau + ctx.dt);
        alpha = (std::max)(0.001, (std::min)(1.0, alpha));
        m_sop_lat_g_smoothed += alpha * (lat_g - m_sop_lat_g_smoothed);
        
        double sop_base = m_sop_lat_g_smoothed * m_sop_effect * (double)m_sop_scale * ctx.decoupling_scale;
        ctx.sop_unboosted_force = sop_base; // Store unboosted for snapshot
        
        // Rear Grip Estimation (v0.4.5 FIX)
        GripResult rear_grip_res = calculate_grip(data->mWheel[2], data->mWheel[3], ctx.avg_load, m_warned_rear_grip,
                                                  m_prev_slip_angle[2], m_prev_slip_angle[3],
                                                  ctx.car_speed, ctx.dt, data->mVehicleName, data, false);
        ctx.avg_rear_grip = rear_grip_res.value;
        m_grip_diag.rear_original = rear_grip_res.original;
        m_grip_diag.rear_approximated = rear_grip_res.approximated;
        m_grip_diag.rear_slip_angle = rear_grip_res.slip_angle;
        if (rear_grip_res.approximated) ctx.frame_warn_rear_grip = true;
        
        // Lateral G Boost (Oversteer)
        // v0.7.1 FIX: Disable when slope detection is enabled to prevent oscillations.
        // Slope detection uses a different calculation method for front grip than the
        // static threshold used for rear grip. This asymmetry creates artificial
        // grip_delta values that cause feedback oscillation.
        // See: docs/dev_docs/investigations/slope_detection_issues_v0.7.0.md (Issue 2)
        if (!m_slope_detection_enabled) {
            double grip_delta = ctx.avg_grip - ctx.avg_rear_grip;
            if (grip_delta > 0.0) {
                sop_base *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
            }
        }
        ctx.sop_base_force = sop_base;
        
        // Rear Align Torque
        double calc_load_rl = approximate_rear_load(data->mWheel[2]);
        double calc_load_rr = approximate_rear_load(data->mWheel[3]);
        ctx.avg_rear_load = (calc_load_rl + calc_load_rr) / 2.0;
        
        double rear_slip_angle = m_grip_diag.rear_slip_angle;
        ctx.calc_rear_lat_force = rear_slip_angle * ctx.avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;
        ctx.calc_rear_lat_force = (std::max)(-MAX_REAR_LATERAL_FORCE, (std::min)(MAX_REAR_LATERAL_FORCE, ctx.calc_rear_lat_force));
        
        ctx.rear_torque = -ctx.calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect * ctx.decoupling_scale;
        
        // Yaw Kick
        double raw_yaw_accel = data->mLocalRotAccel.y;
        if (ctx.car_speed < 5.0) raw_yaw_accel = 0.0;
        else if (std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) raw_yaw_accel = 0.0;
        
        double tau_yaw = (double)m_yaw_accel_smoothing;
        if (tau_yaw < 0.0001) tau_yaw = 0.0001;
        double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
        m_yaw_accel_smoothed += alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
        
        ctx.yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK * ctx.decoupling_scale;
        
        // Apply Speed Gate
        ctx.sop_base_force *= ctx.speed_gate;
        ctx.rear_torque *= ctx.speed_gate;
        ctx.yaw_force *= ctx.speed_gate;
    }

    void calculate_gyro_damping(const TelemInfoV01* data, FFBCalculationContext& ctx) {
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

    void calculate_abs_pulse(const TelemInfoV01* data, FFBCalculationContext& ctx) {
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

    // ... Implement other helpers inline ...

    void calculate_lockup_vibration(const TelemInfoV01* data, FFBCalculationContext& ctx) {
        if (!m_lockup_enabled) return;
        
        // ... (Keep existing logic, updating ctx.lockup_rumble) ...
        double worst_severity = 0.0;
        double chosen_freq_multiplier = 1.0;
        double chosen_pressure_factor = 0.0;
        
        // v0.6.36: Use unified helper method instead of lambda

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

    void calculate_wheel_spin(const TelemInfoV01* data, FFBCalculationContext& ctx) {
        if (m_spin_enabled && data->mUnfilteredThrottle > 0.05) {
            // v0.6.36: Use unified helper method instead of lambda
            double slip_rl = calculate_wheel_slip_ratio(data->mWheel[2]);
            double slip_rr = calculate_wheel_slip_ratio(data->mWheel[3]);
            double max_slip = (std::max)(slip_rl, slip_rr);
            
            if (max_slip > 0.2) {
                double severity = (max_slip - 0.2) / 0.5;
                severity = (std::min)(1.0, severity);
                
                // Torque Drop (Floating feel)
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

    void calculate_slide_texture(const TelemInfoV01* data, FFBCalculationContext& ctx) {
        if (!m_slide_texture_enabled) return;
        double lat_vel_fl = std::abs(data->mWheel[0].mLateralPatchVel);
        double lat_vel_fr = std::abs(data->mWheel[1].mLateralPatchVel);
        double front_slip_avg = (lat_vel_fl + lat_vel_fr) / 2.0;
        double lat_vel_rl = std::abs(data->mWheel[2].mLateralPatchVel);
        double lat_vel_rr = std::abs(data->mWheel[3].mLateralPatchVel);
        double rear_slip_avg = (lat_vel_rl + lat_vel_rr) / 2.0;
        double effective_slip_vel = (std::max)(front_slip_avg, rear_slip_avg);
        
        if (effective_slip_vel > 0.5) {
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

    void calculate_road_texture(const TelemInfoV01* data, FFBCalculationContext& ctx) {
        if (!m_road_texture_enabled) return;

        // Scrub Drag
        if (m_scrub_drag_gain > 0.0) {
            double avg_lat_vel = (data->mWheel[0].mLateralPatchVel + data->mWheel[1].mLateralPatchVel) / 2.0;
            double abs_lat_vel = std::abs(avg_lat_vel);
            if (abs_lat_vel > 0.001) {
                double fade = (std::min)(1.0, abs_lat_vel / 0.5);
                double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;
                ctx.scrub_drag_force = drag_dir * m_scrub_drag_gain * (double)BASE_NM_SCRUB_DRAG * fade * ctx.decoupling_scale;
            }
        }
        
        double delta_l = data->mWheel[0].mVerticalTireDeflection - m_prev_vert_deflection[0];
        double delta_r = data->mWheel[1].mVerticalTireDeflection - m_prev_vert_deflection[1];
        delta_l = (std::max)(-0.01, (std::min)(0.01, delta_l));
        delta_r = (std::max)(-0.01, (std::min)(0.01, delta_r));
        
        double road_noise_val = 0.0;
        bool deflection_active = (std::abs(delta_l) > 0.000001 || std::abs(delta_r) > 0.000001);
        
        if (deflection_active || ctx.car_speed < 5.0) {
            road_noise_val = (delta_l + delta_r) * 50.0;
        } else {
            // Fallback: Use Vertical Acceleration (Heave) for road feel
            // m_prev_vert_accel is now updated unconditionally in STATE UPDATES section
            double vert_accel = data->mLocalAccel.y;
            double delta_accel = vert_accel - m_prev_vert_accel;
            road_noise_val = delta_accel * 0.05 * 50.0;
        }
        
        ctx.road_noise = road_noise_val * m_road_texture_gain * ctx.decoupling_scale * ctx.texture_load_factor;
        ctx.road_noise *= ctx.speed_gate;
    }

    void calculate_suspension_bottoming(const TelemInfoV01* data, FFBCalculationContext& ctx) {
        if (!m_bottoming_enabled) return;
        bool triggered = false;
        double intensity = 0.0;
        
        if (m_bottoming_method == 0) {
            double min_rh = (std::min)(data->mWheel[0].mRideHeight, data->mWheel[1].mRideHeight);
            if (min_rh < 0.002 && min_rh > -1.0) {
                triggered = true;
                intensity = (0.002 - min_rh) / 0.002;
            }
        } else {
            double dForceL = (data->mWheel[0].mSuspForce - m_prev_susp_force[0]) / ctx.dt;
            double dForceR = (data->mWheel[1].mSuspForce - m_prev_susp_force[1]) / ctx.dt;
            double max_dForce = (std::max)(dForceL, dForceR);
            if (max_dForce > 100000.0) {
                triggered = true;
                intensity = (max_dForce - 100000.0) / 200000.0;
            }
        }

        if (!triggered) {
            double max_load = (std::max)(data->mWheel[0].mTireLoad, data->mWheel[1].mTireLoad);
            if (max_load > 8000.0) {
                triggered = true;
                double excess = max_load - 8000.0;
                intensity = std::sqrt(excess) * 0.05;
            }
        }

        if (triggered) {
            double bump_magnitude = intensity * m_bottoming_gain * (double)BASE_NM_BOTTOMING * ctx.decoupling_scale;
            double freq = 50.0;
            m_bottoming_phase += freq * ctx.dt * TWO_PI;
            m_bottoming_phase = std::fmod(m_bottoming_phase, TWO_PI);
            ctx.bottoming_crunch = std::sin(m_bottoming_phase) * bump_magnitude * ctx.speed_gate;
        }
    }
};

#endif // FFBENGINE_H
