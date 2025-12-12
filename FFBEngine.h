#ifndef FFBENGINE_H
#define FFBENGINE_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>
#include "src/lmu_sm_interface/InternalsPlugin.hpp"

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
    // FFB Outputs
    float base_force;
    float sop_force;
    float understeer_drop;
    float oversteer_boost;
    float texture_road;
    float texture_slide;
    float texture_lockup;
    float texture_spin;
    float texture_bottoming;
    float total_output;
    float clipping;

    // Telemetry Inputs
    float steer_force;
    float accel_x;
    float tire_load;
    float grip_fract;
    float slip_ratio;
    float slip_angle;
    float patch_vel;
    float deflection;

    // Telemetry Health Flags
    bool warn_load;
    bool warn_grip;
    bool warn_dt;
};

// FFB Engine Class
class FFBEngine {
public:
    // Settings (GUI Sliders)
    float m_gain = 0.5f;          // Master Gain (Default 0.5 for safety)
    float m_understeer_effect = 1.0f; // 0.0 - 1.0 (How much grip loss affects force)
    float m_sop_effect = 0.15f;    // 0.0 - 1.0 (Lateral G injection strength - Default 0.15 for balanced feel) (0 to prevent jerking)
    float m_min_force = 0.0f;     // 0.0 - 0.20 (Deadzone removal)
    
    // Configurable Smoothing & Caps (v0.3.9)
    float m_sop_smoothing_factor = 0.05f; // 0.0 (Max Smoothing) - 1.0 (Raw). Default Default 0.05 for responsive feel. (0.1 ~5Hz.)
    float m_max_load_factor = 1.5f;      // Cap for load scaling (Default 1.5x)
    float m_sop_scale = 5.0f;            // SoP base scaling factor (Default 5.0 for Nm)
    
    // v0.4.4 Features
    float m_max_torque_ref = 40.0f;      // Reference torque for 100% output (Default 40.0 Nm)
    bool m_invert_force = false;         // Invert final output signal

    // New Effects (v0.2)
    float m_oversteer_boost = 0.0f; // 0.0 - 1.0 (Rear grip loss boost)
    
    bool m_lockup_enabled = false;
    float m_lockup_gain = 0.5f;
    
    bool m_spin_enabled = false;
    float m_spin_gain = 0.5f;

    // Texture toggles
    bool m_slide_texture_enabled = true;
    float m_slide_texture_gain = 0.5f; // 0.0 - 1.0
    
    bool m_road_texture_enabled = false;
    float m_road_texture_gain = 0.5f; // 0.0 - 1.0
    
    // Bottoming Effect (v0.3.2)
    bool m_bottoming_enabled = true;
    float m_bottoming_gain = 1.0f;

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

    // Helper Result Struct for calculate_grip
    struct GripResult {
        double value;           // Final grip value
        bool approximated;      // Was approximation used?
        double original;        // Original telemetry value
        double slip_angle;      // Calculated slip angle (if approximated)
    };

    // Helper: Calculate Slip Angle (v0.4.6 LPF + Logic)
    double calculate_slip_angle(const TelemWheelV01& w, double& prev_state) {
        double v_long = std::abs(w.mLongitudinalGroundVel);
        double min_speed = 0.5;
        if (v_long < min_speed) v_long = min_speed;
        
        double raw_angle = std::atan2(std::abs(w.mLateralPatchVel), v_long);
        
        // LPF: Alpha ~0.1 (Strong smoothing for stability)
        double alpha = 0.1;
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
                              double car_speed) {
        GripResult result;
        result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
        result.value = result.original;
        result.approximated = false;
        result.slip_angle = 0.0;
        
        // Fallback condition: Grip is essentially zero BUT car has significant load
        if (result.value < 0.0001 && avg_load > 100.0) {
            result.approximated = true;
            
            // Low Speed Cutoff (v0.4.6)
            if (car_speed < 5.0) {
                result.slip_angle = 0.0;
                result.value = 1.0; // Force full grip at low speeds
            } else {
                double slip1 = calculate_slip_angle(w1, prev_slip1);
                double slip2 = calculate_slip_angle(w2, prev_slip2);
                result.slip_angle = (slip1 + slip2) / 2.0;
                
                double excess = (std::max)(0.0, result.slip_angle - 0.15);
                result.value = 1.0 - (excess * 2.0);
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
        const double TWO_PI = 6.28318530718;

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
        double game_force = data->mSteeringShaftTorque;
        
        // --- 0. UPDATE STATS ---
        double raw_torque = game_force;
        double raw_load = (fl.mTireLoad + fr.mTireLoad) / 2.0;
        double raw_grip = (fl.mGripFract + fr.mGripFract) / 2.0;
        double raw_lat_g = data->mLocalAccel.x;

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
        // v0.4.5: Use calculated physics load instead of static 4000N
        if (m_missing_load_frames > 20) {
            double calc_load_fl = approximate_load(fl);
            double calc_load_fr = approximate_load(fr);
            avg_load = (calc_load_fl + calc_load_fr) / 2.0;
            
            if (!m_warned_load) {
                std::cout << "[WARNING] Missing Tire Load. Using Approx (SuspForce + 300N)." << std::endl;
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

        // --- 1. Understeer Effect (Grip Modulation) ---
        // FRONT WHEEL GRIP CALCULATION (Refactored v0.4.5)
        
        double car_speed = std::abs(data->mLocalVel.z);

        // Calculate Front Grip using helper (handles fallback and diagnostics)
        // Pass persistent state for LPF (v0.4.6) - Indices 0 and 1
        GripResult front_grip_res = calculate_grip(fl, fr, avg_load, m_warned_grip, 
                                                   m_prev_slip_angle[0], m_prev_slip_angle[1], car_speed);
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
        double grip_factor = 1.0 - ((1.0 - avg_grip) * m_understeer_effect);
        double output_force = game_force * grip_factor;
        
        // --- 2. Seat of Pants (SoP) / Oversteer ---
        // Lateral G-force
        // v0.4.6: Clamp Input to reasonable Gs (+/- 5G)
        double raw_g = (std::max)(-49.05, (std::min)(49.05, data->mLocalAccel.x));
        double lat_g = raw_g / 9.81;
        
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
        
        double sop_base_force = m_sop_lat_g_smoothed * m_sop_effect * (double)m_sop_scale;
        double sop_total = sop_base_force;
        
        // REAR WHEEL GRIP CALCULATION (Refactored v0.4.5)
        
        // Calculate Rear Grip using helper (now includes fallback)
        // Pass persistent state for LPF (v0.4.6) - Indices 2 and 3
        GripResult rear_grip_res = calculate_grip(data->mWheel[2], data->mWheel[3], avg_load, m_warned_rear_grip,
                                                  m_prev_slip_angle[2], m_prev_slip_angle[3], car_speed);
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
        
        // --- 2a. Rear Aligning Torque Integration ---
        double rear_lat_force = (data->mWheel[2].mLateralForce + data->mWheel[3].mLateralForce) / 2.0; // mWheel
        // Scaled down for Nm. Old was 0.05. 2000N * 0.05 = 100.
        // New target ~0.5 Nm contribution? 2000 * K = 0.5 => K = 0.00025
        double rear_torque = rear_lat_force * 0.00025 * m_oversteer_boost; 
        sop_total += rear_torque;
        
        double total_force = output_force + sop_total;
        
        // --- Helper: Calculate Slip Data (Approximation) ---
        // The new LMU interface does not expose mSlipRatio/mSlipAngle directly.
        // We approximate them from mLongitudinalPatchVel and mLateralPatchVel.
        
        // Slip Ratio = PatchVelLong / GroundVelLong
        // Slip Angle = atan(PatchVelLat / GroundVelLong)
        
        double car_speed_ms = std::abs(data->mLocalVel.z); // Or mLongitudinalGroundVel per wheel
        double min_speed = 0.5; // Avoid div-by-zero
        
        auto get_slip_ratio = [&](const TelemWheelV01& w) {
            // v0.4.5: Option to use manual calculation
            if (m_use_manual_slip) {
                return calculate_manual_slip_ratio(w, data->mLocalVel.z);
            }
            // Default Game Data
            double v_long = std::abs(w.mLongitudinalGroundVel);
            if (v_long < min_speed) v_long = min_speed;
            return w.mLongitudinalPatchVel / v_long;
        };
        
        // get_slip_angle was moved up for grip approximation reuse

        // --- 2b. Progressive Lockup (Dynamic) ---
        // Ensure phase updates even if force is small, but gated by enabled
        if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
            double slip_fl = get_slip_ratio(data->mWheel[0]);
            double slip_fr = get_slip_ratio(data->mWheel[1]);
            // Use worst slip
            double max_slip = (std::min)(slip_fl, slip_fr); // Slip is negative for braking
            
            // Thresholds: -0.1 (Peak Grip), -1.0 (Locked)
            // Range of interest: -0.1 to -0.5
            if (max_slip < -0.1) {
                double severity = (std::abs(max_slip) - 0.1) / 0.4; // 0.0 to 1.0 scale
                severity = (std::min)(1.0, severity);
                
                // DYNAMIC FREQUENCY: Linked to Car Speed (Slower car = Lower pitch grinding)
                // As the car slows down, the "scrubbing" pitch drops.
                // Speed is in m/s. 
                // Example: 300kmh (83m/s) -> ~80Hz. 50kmh (13m/s) -> ~20Hz.
                double freq = 10.0 + (car_speed_ms * 1.5); 

                // PHASE ACCUMULATION
                m_lockup_phase += freq * dt * TWO_PI;
                if (m_lockup_phase > TWO_PI) m_lockup_phase -= TWO_PI;

                double amp = severity * m_lockup_gain * 4.0; // Scaled for Nm (was 800)
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

                // PHASE ACCUMULATION
                m_spin_phase += freq * dt * TWO_PI;
                if (m_spin_phase > TWO_PI) m_spin_phase -= TWO_PI;

                // Amplitude
                double amp = severity * m_spin_gain * 2.5; // Scaled for Nm (was 500)
                spin_rumble = std::sin(m_spin_phase) * amp;
                
                total_force += spin_rumble;
            }
        }

        // --- 3. Slide Texture (Dynamic Sawtooth) ---
        if (m_slide_texture_enabled) {
            // New logic: Use mLateralPatchVel directly instead of Angle
            // This is cleaner as it represents actual scrubbing speed.
            double lat_vel_l = std::abs(fl.mLateralPatchVel);
            double lat_vel_r = std::abs(fr.mLateralPatchVel);
            double avg_lat_vel = (lat_vel_l + lat_vel_r) / 2.0;
            
            // Threshold: 0.5 m/s (~2 kph) slip
            if (avg_lat_vel > 0.5) {
                
                // Map 1 m/s -> 40Hz, 10 m/s -> 200Hz
                double freq = 40.0 + (avg_lat_vel * 17.0);
                if (freq > 250.0) freq = 250.0;

                m_slide_phase += freq * dt * TWO_PI;
                if (m_slide_phase > TWO_PI) m_slide_phase -= TWO_PI;

                // Sawtooth wave
                double sawtooth = (m_slide_phase / TWO_PI) * 2.0 - 1.0;

                // Amplitude: Scaled by PRE-CALCULATED global load_factor
                slide_noise = sawtooth * m_slide_texture_gain * 1.5 * load_factor; // Scaled for Nm (was 300)
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
                    double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;
                    double drag_force = drag_dir * m_scrub_drag_gain * 2.0 * fade; // Scaled & Faded
                    total_force += drag_force;
                }
            }

            // Use change in suspension deflection
            double vert_l = fl.mVerticalTireDeflection;
            double vert_r = fr.mVerticalTireDeflection;
            
            // Delta from previous frame
            double delta_l = vert_l - m_prev_vert_deflection[0];
            double delta_r = vert_r - m_prev_vert_deflection[1];
            
            // v0.4.6: Delta Clamping (+/- 0.01m)
            delta_l = (std::max)(-0.01, (std::min)(0.01, delta_l));
            delta_r = (std::max)(-0.01, (std::min)(0.01, delta_r));

            // Store for next frame
            m_prev_vert_deflection[0] = vert_l;
            m_prev_vert_deflection[1] = vert_r;
            
            // Amplify sudden changes
            double road_noise = (delta_l + delta_r) * 25.0 * m_road_texture_gain; // Scaled for Nm (was 5000)
            
            // Apply LOAD FACTOR: Bumps feel harder under compression
            road_noise *= load_factor;
            
            total_force += road_noise;
        }

        // --- 5. Suspension Bottoming (High Load Impulse) ---
        if (m_bottoming_enabled) {
            bool triggered = false;
            double intensity = 0.0;

            if (m_bottoming_method == 0) {
                // Method A: Scraping (Ride Height)
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
                m_prev_susp_force[0] = susp_l;
                m_prev_susp_force[1] = susp_r;
                
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
                double bump_magnitude = intensity * m_bottoming_gain * 0.05 * 20.0; // Scaled for Nm
                
                // FIX: Use a 50Hz "Crunch" oscillation instead of directional DC offset
                double freq = 50.0; 
                
                // Phase Integration
                m_bottoming_phase += freq * dt * TWO_PI;
                if (m_bottoming_phase > TWO_PI) m_bottoming_phase -= TWO_PI;

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
        
        // --- SNAPSHOT LOGIC ---
        // Capture all internal states for visualization
        {
            std::lock_guard<std::mutex> lock(m_debug_mutex);
            if (m_debug_buffer.size() < 100) {
                FFBSnapshot snap;
                snap.total_output = (float)norm_force;
                snap.base_force = (float)game_force;
                snap.sop_force = (float)sop_base_force;
                snap.understeer_drop = (float)(game_force * (1.0 - grip_factor));
                snap.oversteer_boost = (float)(sop_total - sop_base_force);
                snap.texture_road = (float)road_noise;
                snap.texture_slide = (float)slide_noise;
                snap.texture_lockup = (float)lockup_rumble;
                snap.texture_spin = (float)spin_rumble;
                snap.texture_bottoming = (float)bottoming_crunch;
                snap.clipping = (std::abs(norm_force) > 0.99f) ? 1.0f : 0.0f;
                
                // Telemetry inputs
                snap.steer_force = (float)game_force;
                snap.accel_x = (float)data->mLocalAccel.x;
                snap.tire_load = (float)avg_load;
                snap.grip_fract = (float)avg_grip;
                
                // Snapshot Approximations
                snap.slip_ratio = (float)((get_slip_ratio(fl) + get_slip_ratio(fr)) / 2.0);
                
                // FIX (v0.4.6): Use the actual smoothed slip angle from diagnostics instead of recalculating with dummy state
                // This ensures the graph matches the physics logic.
                snap.slip_angle = (float)m_grip_diag.front_slip_angle;
                
                snap.patch_vel = (float)((std::abs(fl.mLateralPatchVel) + std::abs(fr.mLateralPatchVel)) / 2.0);
                snap.deflection = (float)((fl.mVerticalTireDeflection + fr.mVerticalTireDeflection) / 2.0);
                
                // Warnings
                snap.warn_load = frame_warn_load;
                snap.warn_grip = frame_warn_grip || frame_warn_rear_grip; // Combined warning
                snap.warn_dt = frame_warn_dt;

                m_debug_buffer.push_back(snap);
            }
        }

        // Clip
        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
};

#endif // FFBENGINE_H
