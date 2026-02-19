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
#include "MathUtils.h"
#include "PerfStats.h"
#include "VehicleUtils.h"

#ifdef _WIN32
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

// Bring common math into scope
using namespace ffb_math;

// ChannelStats moved to PerfStats.h

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
    float ffb_abs_pulse;    // New v0.7.53
    float ffb_soft_lock;    // New v0.7.61 (Issue #117)
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

    // Rate Monitoring (Issue #129)
    float ffb_rate;
    float telemetry_rate;
    float hw_rate;
    float torque_rate;
    float gen_torque_rate;
};

// BiquadNotch moved to MathUtils.h

// Helper Result Struct for calculate_grip
struct GripResult {
    double value;           // Final grip value
    bool approximated;      // Was approximation used?
    double original;        // Original telemetry value
    double slip_angle;      // Calculated slip angle (if approximated)
};

namespace FFBEngineTests { class FFBEngineTestAccess; }

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
    double soft_lock_force = 0.0;
    double gain_reduction_factor = 1.0;
};
    
// FFB Engine Class
class FFBEngine {
public:
    using ParsedVehicleClass = ::ParsedVehicleClass;

    // Settings (GUI Sliders)
    float m_gain;
    float m_understeer_effect;
    float m_sop_effect;
    float m_min_force;
    float m_dynamic_weight_gain; 
    
    // Smoothing Settings (v0.7.47)
    float m_dynamic_weight_smoothing;
    float m_grip_smoothing_steady;
    float m_grip_smoothing_fast;
    float m_grip_smoothing_sensitivity;

    // Configurable Smoothing & Caps (v0.3.9)
    float m_sop_smoothing_factor;
    float m_texture_load_cap = 1.5f; 
    float m_brake_load_cap = 1.5f;   
    float m_sop_scale;
    
    // v0.4.4 Features
    float m_max_torque_ref;
    bool m_invert_force;
    
    // Base Force Debugging (v0.4.13)
    float m_steering_shaft_gain;
    int m_base_force_mode;
    int m_torque_source = 0; 

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
    float m_lockup_gamma = 2.0f;           
    float m_lockup_prediction_sens = 50.0f; 
    float m_lockup_bump_reject = 1.0f;     
    
    bool m_abs_pulse_enabled = true;      
    float m_abs_gain = 1.0f;               
    
    bool m_spin_enabled;
    float m_spin_gain;

    // Texture toggles
    bool m_slide_texture_enabled;
    float m_slide_texture_gain;
    float m_slide_freq_scale;
    
    bool m_road_texture_enabled;
    float m_road_texture_gain;
    
    // Bottoming Effect (v0.3.2)
    bool m_bottoming_enabled = true;  
    float m_bottoming_gain = 1.0f;    

    // Soft Lock (Issue #117)
    bool m_soft_lock_enabled = true;
    float m_soft_lock_stiffness = 20.0f;
    float m_soft_lock_damping = 0.5f;

    float m_slip_angle_smoothing;
    
    // NEW: Grip Estimation Settings (v0.5.7)
    float m_optimal_slip_angle;
    float m_optimal_slip_ratio;
    
    // NEW: Steering Shaft Smoothing (v0.5.7)
    float m_steering_shaft_smoothing;
    
    // v0.4.41: Signal Filtering Settings
    bool m_flatspot_suppression = false;
    float m_notch_q = 2.0f; 
    float m_flatspot_strength = 1.0f; 
    
    // Static Notch Filter (v0.4.43)
    bool m_static_notch_enabled = false;
    float m_static_notch_freq = 11.0f;
    float m_static_notch_width = 2.0f; 
    float m_yaw_kick_threshold = 0.2f; 

    // v0.6.23: User-Adjustable Speed Gate
    float m_speed_gate_lower = 1.0f; 
    float m_speed_gate_upper = 5.0f; 

    // v0.6.23: Additional Advanced Physics (Reserved for future use)
    float m_road_fallback_scale = 0.05f;
    bool m_understeer_affects_sop = false;
    
    // ===== SLOPE DETECTION (v0.7.0 -> v0.7.3 stability fixes) =====
    bool m_slope_detection_enabled = false;
    int m_slope_sg_window = 15;
    float m_slope_sensitivity = 0.5f;            

    float m_slope_smoothing_tau = 0.04f;         

    // NEW v0.7.3: Stability fixes
    float m_slope_alpha_threshold = 0.02f;    
    float m_slope_decay_rate = 5.0f;          
    bool m_slope_confidence_enabled = true;   
    float m_slope_confidence_max_rate = 0.10f; 

    // NEW v0.7.11: Min/Max Threshold System
    float m_slope_min_threshold = -0.3f;   
    float m_slope_max_threshold = -2.0f;   

    // NEW v0.7.40: Advanced Features
    float m_slope_g_slew_limit = 50.0f;
    bool m_slope_use_torque = true;
    float m_slope_torque_sensitivity = 0.5f;

    // Signal Diagnostics
    double m_debug_freq = 0.0; 
    double m_theoretical_freq = 0.0; 

    // Rate Monitoring (Issue #129)
    double m_ffb_rate = 0.0;
    double m_telemetry_rate = 0.0;
    double m_hw_rate = 0.0;
    double m_torque_rate = 0.0;
    double m_gen_torque_rate = 0.0;

    // Warning States (Console logging)
    bool m_warned_load = false;
    bool m_warned_grip = false;
    bool m_warned_rear_grip = false; 
    bool m_warned_dt = false;
    bool m_warned_lat_force_front = false;
    bool m_warned_lat_force_rear = false;
    bool m_warned_susp_force = false;
    bool m_warned_susp_deflection = false;
    bool m_warned_vert_deflection = false; 
    
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
    int m_missing_vert_deflection_frames = 0; 

    // Internal state
    double m_prev_vert_deflection[4] = {0.0, 0.0, 0.0, 0.0}; 
    double m_prev_vert_accel = 0.0; 
    double m_prev_slip_angle[4] = {0.0, 0.0, 0.0, 0.0}; 
    double m_prev_rotation[4] = {0.0, 0.0, 0.0, 0.0};    
    double m_prev_brake_pressure[4] = {0.0, 0.0, 0.0, 0.0}; 
    
    // Gyro State (v0.4.17)
    double m_prev_steering_angle = 0.0;
    double m_steering_velocity_smoothed = 0.0;
    
    // Yaw Acceleration Smoothing State (v0.4.18)
    double m_yaw_accel_smoothed = 0.0;

    // Internal state for Steering Shaft Smoothing (v0.5.7)
    double m_steering_shaft_torque_smoothed = 0.0;

    // Kinematic Smoothing State (v0.4.38)
    double m_accel_x_smoothed = 0.0;
    double m_accel_z_smoothed = 0.0; 
    
    // Kinematic Physics Parameters (v0.4.39)
    float m_approx_mass_kg = 1100.0f;
    float m_approx_aero_coeff = 2.0f;
    float m_approx_weight_bias = 0.55f;
    float m_approx_roll_stiffness = 0.6f;

    // Phase Accumulators for Dynamic Oscillators
    double m_lockup_phase = 0.0;
    double m_spin_phase = 0.0;
    double m_slide_phase = 0.0;
    double m_abs_phase = 0.0; 
    double m_bottoming_phase = 0.0;
    
    // Phase Accumulators for Dynamic Oscillators (v0.6.20)
    float m_abs_freq_hz = 20.0f;
    float m_lockup_freq_scale = 1.0f;
    float m_spin_freq_scale = 1.0f;
    
    // Internal state for Bottoming (Method B)
    double m_prev_susp_force[2] = {0.0, 0.0}; 

    // New Settings (v0.4.5)
    int m_bottoming_method = 0; 
    float m_scrub_drag_gain; 

    // Smoothing State
    double m_sop_lat_g_smoothed = 0.0;
    
    // Filter Instances (v0.4.41)
    BiquadNotch m_notch_filter;
    BiquadNotch m_static_notch_filter;

    // Slope Detection Buffers (Circular) - v0.7.0
    static constexpr int SLOPE_BUFFER_MAX = 41;  
    std::array<double, SLOPE_BUFFER_MAX> m_slope_lat_g_buffer = {};
    std::array<double, SLOPE_BUFFER_MAX> m_slope_slip_buffer = {};
    int m_slope_buffer_index = 0;
    int m_slope_buffer_count = 0;

    // Slope Detection State (Public for diagnostics) - v0.7.0
    double m_slope_current = 0.0;
    double m_slope_grip_factor = 1.0;
    double m_slope_smoothed_output = 1.0;

    // NEW v0.7.38: Input Smoothing State
    double m_slope_lat_g_smoothed = 0.0;
    double m_slope_slip_smoothed = 0.0;

    // NEW v0.7.38: Steady State Logic
    double m_slope_hold_timer = 0.0;
    static constexpr double SLOPE_HOLD_TIME = 0.25; 

    // NEW v0.7.38: Debug members for Logger
    double m_debug_slope_raw = 0.0;
    double m_debug_slope_num = 0.0;
    double m_debug_slope_den = 0.0;

    // NEW v0.7.40: Advanced Slope Detection State
    double m_slope_lat_g_prev = 0.0;
    std::array<double, SLOPE_BUFFER_MAX> m_slope_torque_buffer = {};
    std::array<double, SLOPE_BUFFER_MAX> m_slope_steer_buffer = {};
    double m_slope_torque_smoothed = 0.0;
    double m_slope_steer_smoothed = 0.0;
    double m_slope_torque_current = 0.0;
    
    // NEW v0.7.40: More Debug members
    double m_debug_slope_torque_num = 0.0;
    double m_debug_slope_torque_den = 0.0;
    double m_debug_lat_g_slew = 0.0;

    // Dynamic Weight State (v0.7.46)
    double m_static_front_load = 0.0; 
    double m_dynamic_weight_smoothed = 1.0; 
    double m_front_grip_smoothed_state = 1.0; 
    double m_rear_grip_smoothed_state = 1.0;  

    // Context for Logging (v0.7.x)
    char m_vehicle_name[64] = "Unknown";
    char m_track_name[64] = "Unknown";

    // Logging intermediate values (exposed for AsyncLogger)
    double m_slope_dG_dt = 0.0;       
    double m_slope_dAlpha_dt = 0.0;   

    // Frequency Estimator State (v0.4.41)
    double m_last_crossing_time = 0.0;
    double m_last_output_force = 0.0; 
    double m_torque_ac_smoothed = 0.0; 
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
    
    friend class FFBEngineTests::FFBEngineTestAccess;

    FFBEngine();

    bool IsFFBAllowed(const VehicleScoringInfoV01& scoring, unsigned char gamePhase) const;
    double ApplySafetySlew(double target_force, double dt, bool restricted);
    std::vector<FFBSnapshot> GetDebugBatch();

    // UI Reference & Physics Multipliers (v0.4.50)
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
    static constexpr float BASE_NM_SOFT_LOCK        = 50.0f;

private:
    static constexpr double MIN_SLIP_ANGLE_VELOCITY = 0.5; // m/s
    static constexpr double REAR_TIRE_STIFFNESS_COEFFICIENT = 15.0; 
    static constexpr double MAX_REAR_LATERAL_FORCE = 6000.0; // N
    static constexpr double REAR_ALIGN_TORQUE_COEFFICIENT = 0.001; // Nm per N
    static constexpr double SYNTHETIC_MODE_DEADZONE_NM = 0.5; // Nm
    static constexpr double DEFAULT_STEERING_RANGE_RAD = 9.4247; 
    static constexpr double GYRO_SPEED_SCALE = 10.0;
    static constexpr double WEIGHT_TRANSFER_SCALE = 2000.0; // N per G
    static constexpr double MIN_VALID_SUSP_FORCE = 10.0; // N 
    static constexpr double LOCKUP_FREQ_MULTIPLIER_REAR = 0.3;  
    static constexpr double LOCKUP_AMPLITUDE_BOOST_REAR = 1.5;  
    static constexpr double AXLE_DIFF_HYSTERESIS = 0.01;  
    static constexpr double ABS_PEDAL_THRESHOLD = 0.5;  
    static constexpr double ABS_PRESSURE_RATE_THRESHOLD = 2.0;  
    static constexpr double PREDICTION_BRAKE_THRESHOLD = 0.02;  
    static constexpr double PREDICTION_LOAD_THRESHOLD = 50.0;   

    double m_auto_peak_load = 4500.0; 
    std::string m_current_class_name = "";
    bool m_auto_load_normalization_enabled = true; 

    void update_static_load_reference(double current_load, double speed, double dt);
    void InitializeLoadReference(const char* className, const char* vehicleName);
    
public:
    double calculate_raw_slip_angle_pair(const TelemWheelV01& w1, const TelemWheelV01& w2);
    double calculate_slip_angle(const TelemWheelV01& w, double& prev_state, double dt);
    
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
                              bool is_front);

    double approximate_load(const TelemWheelV01& w);
    double approximate_rear_load(const TelemWheelV01& w);
    double calculate_kinematic_load(const TelemInfoV01* data, int wheel_index);
    double calculate_manual_slip_ratio(const TelemWheelV01& w, double car_speed_ms);
    NOINLINE double calculate_slope_grip(double lateral_g, double slip_angle, double dt, const TelemInfoV01* data = nullptr);
    double calculate_slope_confidence(double dAlpha_dt);
    double calculate_wheel_slip_ratio(const TelemWheelV01& w);

    double calculate_force(const TelemInfoV01* data, const char* vehicleClass = nullptr, const char* vehicleName = nullptr, float genFFBTorque = 0.0f);

    double apply_signal_conditioning(double raw_torque, const TelemInfoV01* data, FFBCalculationContext& ctx);

private:
    void calculate_sop_lateral(const TelemInfoV01* data, FFBCalculationContext& ctx);
    NOINLINE void calculate_gyro_damping(const TelemInfoV01* data, FFBCalculationContext& ctx);
    NOINLINE void calculate_abs_pulse(const TelemInfoV01* data, FFBCalculationContext& ctx);
    void calculate_lockup_vibration(const TelemInfoV01* data, FFBCalculationContext& ctx);
    void calculate_wheel_spin(const TelemInfoV01* data, FFBCalculationContext& ctx);
    void calculate_slide_texture(const TelemInfoV01* data, FFBCalculationContext& ctx);
    void calculate_road_texture(const TelemInfoV01* data, FFBCalculationContext& ctx);
    void calculate_suspension_bottoming(const TelemInfoV01* data, FFBCalculationContext& ctx);
    void calculate_soft_lock(const TelemInfoV01* data, FFBCalculationContext& ctx);
};

#endif // FFBENGINE_H
