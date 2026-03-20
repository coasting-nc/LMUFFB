#ifndef FFBENGINE_H
#define FFBENGINE_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <chrono>
#include <array>
#include <cstring>
#include "io/lmu_sm_interface/InternalsPluginWrapper.h"
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
// Default FFB calculation timestep. Used by FFBCalculationContext (defined before
// FFBEngine, so cannot reference FFBEngine::DEFAULT_CALC_DT directly).
// Note: FFBEngine also has a private member of the same name; this file-scope
// constant does NOT trigger GCC's -Wchanges-meaning because it is only looked up
// inside FFBCalculationContext, not inside FFBEngine's own class body.
static constexpr double DEFAULT_CALC_DT = 0.0025; // 400 Hz (1/400 s)


// ChannelStats moved to PerfStats.h

enum class LoadTransform {
    LINEAR = 0,
    CUBIC = 1,
    QUADRATIC = 2,
    HERMITE = 3
};

// 1. Define the Snapshot Struct (Unified FFB + Telemetry)
#include "FFBSnapshot.h"

// Refactored Managers
#include "FFBSafetyMonitor.h"
#include "FFBMetadataManager.h"
#include "FFBDebugBuffer.h"

// BiquadNotch moved to MathUtils.h

// Helper Result Struct for calculate_axle_grip
struct GripResult {
    double value;           // Final grip value
    bool approximated;      // Was approximation used?
    double original;        // Original telemetry value
    double slip_angle;      // Calculated slip angle (if approximated)
};

struct Preset;

namespace FFBEngineTests { class FFBEngineTestAccess; }

struct FFBCalculationContext {
    double dt = DEFAULT_CALC_DT;
    double car_speed = 0.0;       // Absolute m/s
    double car_speed_long = 0.0;  // Longitudinal m/s (Raw)
    double speed_gate = 1.0;
    double texture_load_factor = 1.0;
    double brake_load_factor = 1.0;
    double avg_front_load = 0.0;
    double avg_front_grip = 0.0;

    // Diagnostics
    bool frame_warn_load = false;
    bool frame_warn_grip = false;
    bool frame_warn_rear_grip = false;
    bool frame_warn_dt = false;

    // Intermediate results
    double grip_factor = 1.0;     // 1.0 = full grip, 0.0 = no grip
    double sop_base_force = 0.0;
    double sop_unboosted_force = 0.0; // For snapshot compatibility
    double lat_load_force = 0.0;  // New v0.7.154 (Issue #282)
    double rear_torque = 0.0;
    double yaw_force = 0.0;
    double scrub_drag_force = 0.0;
    double gyro_force = 0.0;
    double stationary_damping_force = 0.0; // New v0.7.206 (Issue #418)
    double avg_rear_grip = 0.0;
    double calc_rear_lat_force = 0.0;
    double avg_rear_load = 0.0;
    double long_load_force = 0.0; // New #301

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

    // Buffer size constants (declared first so they can be used as array bounds below)
    static constexpr int STR_BUF_64 = 64;

    // Settings (GUI Sliders)
    bool m_dynamic_normalization_enabled = false; // Issue #207: Structural force normalization toggle
    bool m_auto_load_normalization_enabled = false; // Stage 3: Vibration Normalization (Load-based)
    float m_vibration_gain = 1.0f; // Issue #206: Global vibration scaling
    float m_gain;
    float m_understeer_effect;
    float m_understeer_gamma = 1.0f; // NEW: Gamma curve for understeer
    float m_sop_effect;
    float m_lat_load_effect = 0.0f; // New v0.7.121 (Issue #213 add, not replace)
    LoadTransform m_lat_load_transform = LoadTransform::LINEAR; // New v0.7.154 (Issue #282)
    float m_long_load_effect = 0.0f; // Renamed from dynamic_weight_gain (#301)
    LoadTransform m_long_load_transform = LoadTransform::LINEAR; // New #301
    float m_min_force;

    // Smoothing Settings (v0.7.47)
    float m_long_load_smoothing; // Renamed from dynamic_weight_smoothing (#301)
    float m_grip_smoothing_steady;
    float m_grip_smoothing_fast;
    float m_grip_smoothing_sensitivity;

    // Configurable Smoothing & Caps (v0.3.9)
    float m_sop_smoothing_factor;
    float m_texture_load_cap = 1.5f; 
    float m_brake_load_cap = 1.5f;   
    float m_sop_scale;
    
    // v0.4.4 Features
    float m_wheelbase_max_nm;
    float m_target_rim_nm;
    bool m_invert_force = true;
    
    // Base Force Debugging (v0.4.13)
    float m_steering_shaft_gain;
    float m_ingame_ffb_gain = 1.0f; // New v0.7.71 (Issue #160)
    int m_torque_source = 0; 
    int m_steering_100hz_reconstruction = 0; // NEW: 0 = Zero Latency, 1 = Smooth
    bool m_torque_passthrough = false;

    // New Effects (v0.2)
    float m_oversteer_boost;
    float m_rear_align_effect;
    float m_kerb_strike_rejection = 0.0f; // NEW: Kerb strike rejection slider
    float m_sop_yaw_gain;
    float m_gyro_gain;
    float m_gyro_smoothing;
    float m_stationary_damping = 0.0f; // New v0.7.206 (Issue #418)
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

    // Unloaded Yaw Kick (Braking/Lift-off) - v0.7.164 (Issue #322)
    float m_unloaded_yaw_gain = 0.0f;
    float m_unloaded_yaw_threshold = 0.2f;
    float m_unloaded_yaw_sens = 1.0f;
    float m_unloaded_yaw_gamma = 0.5f;
    float m_unloaded_yaw_punch = 0.05f;

    // Power Yaw Kick (Acceleration) - v0.7.164 (Issue #322)
    float m_power_yaw_gain = 0.0f;
    float m_power_yaw_threshold = 0.2f;
    float m_power_slip_threshold = 0.10f;
    float m_power_yaw_gamma = 0.5f;
    float m_power_yaw_punch = 0.05f;

    // v0.6.23: User-Adjustable Speed Gate
    float m_speed_gate_lower = 1.0f; 
    float m_speed_gate_upper = 5.0f; 

    // REST API Fallback (Issue #221)
    bool m_rest_api_enabled = false;
    int m_rest_api_port = 6397;

    // v0.6.23: Additional Advanced Physics (Reserved for future use)
    float m_road_fallback_scale = 0.05f;
    bool m_understeer_affects_sop = false;
    bool m_load_sensitivity_enabled = true; // Issue #392: Dynamic Load Sensitivity toggle
    
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
    double m_physics_rate = 0.0; // New v0.7.117 (Issue #217)

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
    TelemInfoV01 m_working_info; // Persistent storage for upsampled telemetry
    double m_last_telemetry_time = -1.0;

    ffb_math::LinearExtrapolator m_upsample_lat_patch_vel[4];
    ffb_math::LinearExtrapolator m_upsample_long_patch_vel[4];
    ffb_math::LinearExtrapolator m_upsample_vert_deflection[4];
    ffb_math::LinearExtrapolator m_upsample_susp_force[4];
    ffb_math::LinearExtrapolator m_upsample_brake_pressure[4];
    ffb_math::LinearExtrapolator m_upsample_rotation[4];
    ffb_math::LinearExtrapolator m_upsample_steering;
    ffb_math::LinearExtrapolator m_upsample_throttle;
    ffb_math::LinearExtrapolator m_upsample_brake;
    ffb_math::LinearExtrapolator m_upsample_local_accel_x;
    ffb_math::LinearExtrapolator m_upsample_local_accel_y;
    ffb_math::LinearExtrapolator m_upsample_local_accel_z;
    ffb_math::LinearExtrapolator m_upsample_local_rot_accel_y;
    ffb_math::LinearExtrapolator m_upsample_local_rot_y;
    ffb_math::HoltWintersFilter  m_upsample_shaft_torque;

    double m_prev_vert_deflection[4] = {0.0, 0.0, 0.0, 0.0}; 
    double m_prev_vert_accel = 0.0; 
    double m_prev_slip_angle[4] = {0.0, 0.0, 0.0, 0.0}; 
    double m_prev_load[4] = {0.0, 0.0, 0.0, 0.0}; // NEW: Smoothed load state
    double m_prev_rotation[4] = {0.0, 0.0, 0.0, 0.0};    
    double m_prev_brake_pressure[4] = {0.0, 0.0, 0.0, 0.0}; 
    
    // Gyro State (v0.4.17)
    double m_prev_steering_angle = 0.0;
    double m_steering_velocity_smoothed = 0.0;
    
    // Yaw Acceleration Smoothing State (v0.4.18)
    double m_yaw_accel_smoothed = 0.0;
    double m_prev_yaw_rate = 0.0;     // New v0.7.144 (Solution 2)
    bool m_yaw_rate_seeded = false;   // New v0.7.144 (Solution 2)
    double m_prev_yaw_rate_log = 0.0;

    // REPLACE m_prev_derived_yaw_accel WITH THESE:
    double m_fast_yaw_accel_smoothed = 0.0;
    double m_prev_fast_yaw_accel = 0.0;
    bool m_yaw_accel_seeded = false;       // New v0.7.164 (Issue #322)

    // NEW: Vulnerability gate smoothing
    double m_unloaded_vulnerability_smoothed = 0.0;
    double m_power_vulnerability_smoothed = 0.0;

    // Derived Acceleration State (Issue #278)
    TelemVect3 m_prev_local_vel = {};
    bool m_local_vel_seeded = false;
    double m_derived_accel_y_100hz = 0.0;
    double m_derived_accel_z_100hz = 0.0;
    bool m_yaw_rate_log_seeded = false;

    // Internal state for Steering Shaft Smoothing (v0.5.7)
    double m_steering_shaft_torque_smoothed = 0.0;

    // Kinematic Smoothing State (v0.4.38)
    double m_accel_x_smoothed = 0.0;
    double m_accel_z_smoothed = 0.0; 
    

    // Phase Accumulators for Dynamic Oscillators
    double m_kerb_timer = 0.0; // NEW: Kerb strike attenuation timer
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
    double m_prev_susp_force[4] = {0.0, 0.0, 0.0, 0.0};

    // Seeding state (Issue #379)
    bool m_derivatives_seeded = false;

    // New Settings (v0.4.5)
    int m_bottoming_method = 0; 
    float m_scrub_drag_gain; 

    // Smoothing State
    double m_sop_lat_g_smoothed = 0.0;
    double m_sop_load_smoothed = 0.0; // New v0.7.121
    
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
    double m_static_rear_load = 0.0; // New v0.7.164 (Issue #322)
    bool m_static_load_latched = false;
    double m_smoothed_vibration_mult = 1.0;
    double m_long_load_smoothed = 1.0; // Renamed from dynamic_weight_smoothed (#301)
    double m_front_grip_smoothed_state = 1.0; 
    double m_rear_grip_smoothed_state = 1.0;  

    // Metadata Manager (Option A Extracted)
    FFBMetadataManager m_metadata;

    // Logging intermediate values (exposed for AsyncLogger)
    double m_slope_dG_dt = 0.0;       
    double m_slope_dAlpha_dt = 0.0;   

    // Frequency Estimator State (v0.4.41)
    double m_last_crossing_time = 0.0;
    double m_torque_ac_smoothed = 0.0; 
    double m_prev_ac_torque = 0.0;

    // Safety Monitor (Option A Extracted)
    FFBSafetyMonitor m_safety;

    // Telemetry Stats
    ChannelStats s_torque;
    ChannelStats s_front_load;
    ChannelStats s_front_grip;
    ChannelStats s_lat_g;
    std::chrono::steady_clock::time_point last_log_time;

    // Thread-Safe Buffer (Producer-Consumer)
    FFBDebugBuffer m_debug_buffer{100}; // DEBUG_BUFFER_CAP
    
    friend class FFBEngineTests::FFBEngineTestAccess;
    friend struct Preset;

    FFBEngine();

    std::vector<FFBSnapshot> GetDebugBatch() { return m_debug_buffer.GetBatch(); }

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
    static constexpr double KERB_LOAD_CAP_MULT = 1.5;
    static constexpr double KERB_DETECTION_THRESHOLD_M_S = 0.8;
    static constexpr double KERB_HOLD_TIME_S = 0.1;
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

    double m_auto_peak_front_load = 4500.0; // DEFAULT_AUTO_PEAK_LOAD

    static constexpr double HPF_TIME_CONSTANT_S = 0.1;
    static constexpr double ZERO_CROSSING_EPSILON = 0.05;
    static constexpr double MIN_FREQ_PERIOD = 0.005;
    static constexpr double MAX_FREQ_PERIOD = 1.0;
    static constexpr double RADIUS_FALLBACK_DEFAULT_M = 0.33;
    static constexpr double RADIUS_FALLBACK_MIN_M = 0.1;
    static constexpr double UNIT_CM_TO_M = 100.0;
    static constexpr double GRAVITY_MS2 = 9.81;
    static constexpr double EPSILON_DIV = 1e-9;
    static constexpr double TORQUE_SPIKE_RATIO = 3.0;
    static constexpr double TORQUE_SPIKE_MIN_NM = 15.0;
    static constexpr int    MISSING_TELEMETRY_WARN_THRESHOLD = 50;
    static constexpr double PHASE_WRAP_LIMIT = 50.0;
    static constexpr double LAT_G_CLEAN_LIMIT = 8.0;
    static constexpr double TORQUE_SLEW_CLEAN_LIMIT = 1000.0;
    static constexpr double TORQUE_ROLL_AVG_TAU = 1.0;

    static constexpr float  PEAK_TORQUE_DECAY = 0.005f;
    static constexpr float  PEAK_TORQUE_FLOOR = 1.0f;
    static constexpr float  PEAK_TORQUE_CEILING = 100.0f;
    static constexpr float  GAIN_SMOOTHING_TAU = 0.25f;
    static constexpr float  STRUCT_MULT_SMOOTHING_TAU = 0.25f;
    static constexpr float  IDLE_SPEED_MIN_M_S = 3.0f;
    static constexpr float  IDLE_BLEND_FACTOR = 0.1f;
    static constexpr float  SESSION_PEAK_DECAY_RATE = 0.005f;

    static constexpr double DT_EPSILON = 0.000001;
    static constexpr double DEFAULT_DT = 0.0025;
    static constexpr double SPEED_EPSILON = 1.0;
    static constexpr double ACCEL_EPSILON = 1.0;
    static constexpr double G_FORCE_THRESHOLD = 3.0;
    static constexpr double SPEED_HIGH_THRESHOLD = 10.0;
    static constexpr double LOAD_SAFETY_FLOOR = 3000.0;
    static constexpr double LOAD_DECAY_RATE = 100.0;
    static constexpr double COMPRESSION_KNEE_THRESHOLD = 1.5;
    static constexpr double COMPRESSION_KNEE_WIDTH = 0.5;
    static constexpr double COMPRESSION_RATIO = 4.0;
    static constexpr double VIBRATION_EMA_TAU = 0.1;
    static constexpr double USER_CAP_MAX = 10.0;
    static constexpr double CLIPPING_THRESHOLD = 0.99;
    static constexpr int    STR_MAX_64 = 63;
    static constexpr int    STR_MAX_256 = 255;
    static constexpr int    MISSING_LOAD_WARN_THRESHOLD = 20;
    static constexpr double LONG_LOAD_MIN = 0.0; // Relaxed #301
    static constexpr double LONG_LOAD_MAX = 10.0; // Increased #301
    static constexpr double MIN_TAU_S = 0.0001;
    static constexpr double ALPHA_MIN = 0.001;
    static constexpr double ALPHA_MAX = 1.0;
    static constexpr double DUAL_DIVISOR = 2.0;
    static constexpr double HALF_PERIOD_MULT = 0.5;
    static constexpr double MIN_NOTCH_WIDTH_HZ = 0.1;
    static constexpr int    VEHICLE_NAME_CHECK_IDX = 10;
    static constexpr int    DEBUG_BUFFER_CAP = 100;
    static constexpr double OVERSTEER_BOOST_MULT = 2.0;
    static constexpr double MIN_YAW_KICK_SPEED_MS = 5.0;
    static constexpr double MIN_SLIP_WINDOW = 0.01;
    static constexpr double SAWTOOTH_SCALE = 2.0;
    static constexpr double SAWTOOTH_OFFSET = 1.0;
    static constexpr double FFB_EPSILON = 0.0001;
    static constexpr double SOFT_LOCK_MUTE_THRESHOLD_NM = 0.1;
    static constexpr double SOP_SMOOTHING_MAX_TAU = 0.1;

    // Default Values
    static constexpr float  DEFAULT_SAFETY_SLEW_FULL_SCALE_TIME_S = 1.0f; // matches Issue #316 expectations
    static constexpr float  DEFAULT_SAFETY_WINDOW_DURATION = 0.0f;
    static constexpr float  DEFAULT_SAFETY_GAIN_REDUCTION = 0.3f;
    static constexpr float  DEFAULT_SAFETY_SMOOTHING_TAU = 0.2f;
    static constexpr float  DEFAULT_SPIKE_DETECTION_THRESHOLD = 500.0f;
    static constexpr float  DEFAULT_IMMEDIATE_SPIKE_THRESHOLD = 1500.0f;

    static constexpr bool   DEFAULT_STUTTER_SAFETY_ENABLED = false;
    static constexpr float  DEFAULT_STUTTER_THRESHOLD = 1.5f;

    static constexpr double MIN_LFM_ALPHA = 0.001;
    static constexpr double G_LIMIT_5G = 5.0;
    static constexpr double SMOOTHNESS_LIMIT_0999 = 0.999;
    static constexpr double BOTTOMING_RH_THRESHOLD_M = 0.002;
    static constexpr double BOTTOMING_IMPULSE_THRESHOLD_N_S = 100000.0;
    static constexpr double BOTTOMING_IMPULSE_RANGE_N_S = 200000.0;
    static constexpr double BOTTOMING_LOAD_MULT = 4.0;
    static constexpr double BOTTOMING_INTENSITY_SCALE = 0.05;
    static constexpr double BOTTOMING_FREQ_HZ = 50.0;
    static constexpr double SPIN_THROTTLE_THRESHOLD = 0.05;
    static constexpr double SPIN_SLIP_THRESHOLD = 0.2;
    static constexpr double SPIN_SEVERITY_RANGE = 0.5;
    static constexpr double SPIN_TORQUE_DROP_FACTOR = 0.6;
    static constexpr double SPIN_BASE_FREQ = 10.0;
    static constexpr double SPIN_FREQ_SLIP_MULT = 2.5;
    static constexpr double SPIN_MAX_FREQ = 80.0;
    static constexpr double LOCKUP_BASE_FREQ = 10.0;
    static constexpr double LOCKUP_FREQ_SPEED_MULT = 1.5;
    static constexpr double SLIDE_VEL_THRESHOLD = 1.5;
    static constexpr double SLIDE_BASE_FREQ = 10.0;
    static constexpr double SLIDE_FREQ_VEL_MULT = 5.0;
    static constexpr double SLIDE_MAX_FREQ = 250.0;
    static constexpr double LOCKUP_ACCEL_MARGIN = 2.0;
    static constexpr double LOW_PRESSURE_LOCKUP_THRESHOLD = 0.1;
    static constexpr double LOW_PRESSURE_LOCKUP_FIX = 0.5;
    static constexpr double ABS_PULSE_MAGNITUDE_SCALER = 2.0;
    static constexpr double PERCENT_TO_DECIMAL = 100.0;
    static constexpr double SCRUB_VEL_THRESHOLD = 0.001;
    static constexpr double SCRUB_FADE_RANGE = 0.5;

    static constexpr double DEFLECTION_DELTA_LIMIT = 0.01;
    static constexpr double DEFLECTION_ACTIVE_THRESHOLD = 1e-6;
    static constexpr double DEFLECTION_NEAR_ZERO_M = 1e-6;       // Near-zero threshold for susp/vert deflection checks (m)
    static constexpr double MIN_VALID_LAT_FORCE_N = 1.0;          // Minimum lateral force to consider data present (N)
    static constexpr double ROAD_TEXTURE_SPEED_THRESHOLD = 5.0;
    static constexpr double DEFLECTION_NM_SCALE = 50.0;
    static constexpr double ACCEL_ROAD_TEXTURE_SCALE = 0.05;
    static constexpr double DEBUG_FREQ_SMOOTHING = 0.9;
    static constexpr double GAIN_REDUCTION_MAX = 50.0;
    double m_session_peak_torque = 25.0; // DEFAULT_SESSION_PEAK_TORQUE
    double m_smoothed_structural_mult = 1.0 / 25.0; 
    double m_rolling_average_torque = 0.0; 
    double m_last_raw_torque = 0.0; 
    bool m_was_allowed = true; // Track transition for filter reset

    // Diagnostic Log Cooldowns (v0.7.x)
    double m_last_core_nan_log_time = -999.0;
    double m_last_aux_nan_log_time = -999.0;
    double m_last_math_nan_log_time = -999.0;

    void update_static_load_reference(double current_front_load, double current_rear_load, double speed, double dt);
    void InitializeLoadReference(const char* className, const char* vehicleName);
    
public:
    double calculate_raw_slip_angle_pair(const TelemWheelV01& w1, const TelemWheelV01& w2);
    double calculate_slip_angle(const TelemWheelV01& w, double& prev_state, double dt);
    
GripResult calculate_axle_grip(const TelemWheelV01& w1,
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
                              bool is_front);

    double approximate_load(const TelemWheelV01& w);
    double approximate_rear_load(const TelemWheelV01& w);
    double calculate_manual_slip_ratio(const TelemWheelV01& w, double car_speed_ms);
    NOINLINE double calculate_slope_grip(double lateral_g, double slip_angle, double dt, const TelemInfoV01* data = nullptr);
    double calculate_slope_confidence(double dAlpha_dt);
    double calculate_wheel_slip_ratio(const TelemWheelV01& w);

    double calculate_force(const TelemInfoV01* data, const char* vehicleClass = nullptr, const char* vehicleName = nullptr, float genFFBTorque = 0.0f, bool allowed = true, double override_dt = -1.0, signed char mControl = 0);

    void UpdateMetadata(const struct SharedMemoryObjectOut& data);
    double apply_signal_conditioning(double raw_torque, const TelemInfoV01* data, FFBCalculationContext& ctx);
    void ResetNormalization();

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
