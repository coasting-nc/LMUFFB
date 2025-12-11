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
    double min = 1e9;
    double max = -1e9;
    double sum = 0.0;
    long count = 0;
    
    void Update(double val) {
        if (val < min) min = val;
        if (val > max) max = val;
        sum += val;
        count++;
    }
    
    void Reset() {
        min = 1e9; max = -1e9; sum = 0.0; count = 0;
    }
    
    double Avg() { return count > 0 ? sum / count : 0.0; }
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
    bool m_warned_dt = false;
    
    // Hysteresis for missing load
    int m_missing_load_frames = 0;

    // Internal state
    double m_prev_vert_deflection[2] = {0.0, 0.0}; // FL, FR
    
    // Phase Accumulators for Dynamic Oscillators
    double m_lockup_phase = 0.0;
    double m_spin_phase = 0.0;
    double m_slide_phase = 0.0;
    double m_bottoming_phase = 0.0;
    
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
            s_torque.Reset(); s_load.Reset(); s_grip.Reset(); s_lat_g.Reset();
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
        if (m_missing_load_frames > 20) {
            avg_load = 4000.0; // Default load
            if (!m_warned_load) {
                std::cout << "[WARNING] Missing Tire Load data (persistent). Defaulting to 4000N." << std::endl;
                m_warned_load = true;
            }
            frame_warn_load = true;
        }
        
        // Normalize: 4000N is a reference "loaded" GT tire.
        double load_factor = avg_load / 4000.0;
        
        // SAFETY CLAMP: Cap at 1.5x (or configured max) to prevent violent jolts.
        load_factor = (std::min)((double)m_max_load_factor, (std::max)(0.0, load_factor));

        // --- 1. Understeer Effect (Grip Modulation) ---
        // Grip Fraction (Average of front tires)
        double grip_l = fl.mGripFract;
        double grip_r = fr.mGripFract;
        double avg_grip = (grip_l + grip_r) / 2.0;
        
        // SANITY CHECK: If grip is 0.0 but we have load, it's suspicious.
        if (avg_grip < 0.0001 && avg_load > 100.0) {
            avg_grip = 1.0; // Default to full grip
            if (!m_warned_grip) {
                std::cout << "[WARNING] Missing Grip data. Defaulting to 1.0." << std::endl;
                m_warned_grip = true;
            }
            frame_warn_grip = true;
        }

        // Clamp grip 0-1 for safety
        avg_grip = (std::max)(0.0, (std::min)(1.0, avg_grip));
        
        double grip_factor = 1.0 - ((1.0 - avg_grip) * m_understeer_effect);
        double output_force = game_force * grip_factor;
        
        // --- 2. Seat of Pants (SoP) / Oversteer ---
        // Lateral G-force
        double lat_g = data->mLocalAccel.x / 9.81;
        
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
        
        // Oversteer Boost: If Rear Grip < Front Grip (car is rotating), boost SoP
        double grip_rl = data->mWheel[2].mGripFract; // mWheel
        double grip_rr = data->mWheel[3].mGripFract; // mWheel
        double avg_rear_grip = (grip_rl + grip_rr) / 2.0;
        
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
            double v_long = std::abs(w.mLongitudinalGroundVel);
            if (v_long < min_speed) v_long = min_speed;
            // PatchVel is (WheelVel - GroundVel). Ratio is Patch/Ground.
            // Note: mLongitudinalPatchVel signs might differ from rF2 legacy.
            // Assuming negative is slip (braking)
            return w.mLongitudinalPatchVel / v_long;
        };
        
        auto get_slip_angle = [&](const TelemWheelV01& w) {
            double v_long = std::abs(w.mLongitudinalGroundVel);
            if (v_long < min_speed) v_long = min_speed;
            return std::atan2(std::abs(w.mLateralPatchVel), v_long);
        };

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
            // Use change in suspension deflection
            double vert_l = fl.mVerticalTireDeflection;
            double vert_r = fr.mVerticalTireDeflection;
            
            // Delta from previous frame
            double delta_l = vert_l - m_prev_vert_deflection[0];
            double delta_r = vert_r - m_prev_vert_deflection[1];
            
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
            // Detect sudden high load spikes which indicate bottoming out
            // Using Tire Load as proxy for suspension travel limit (bump stop)
            double max_load = (std::max)(fl.mTireLoad, fr.mTireLoad);
            
            // Threshold: 8000N is a heavy hit for a race car corner/bump
            const double BOTTOM_THRESHOLD = 8000.0;
            
            if (max_load > BOTTOM_THRESHOLD) {
                double excess = max_load - BOTTOM_THRESHOLD;
                
                // Non-linear response (Square root softens the initial onset)
                double bump_magnitude = std::sqrt(excess) * m_bottoming_gain * 0.0025; // Scaled (was 0.5)
                
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

        // --- 6. Min Force (Deadzone Removal) ---
        // Boost small forces to overcome wheel friction
        // NOTE: Changed from 4000.0 (Newtons for old mSteeringArmForce) to 20.0 (Nm for new mSteeringShaftTorque)
        // Typical GT3/Hypercar max torque is 15-25 Nm. Adjust based on testing if needed.
        double max_force_ref = 20.0; 
        double norm_force = total_force / max_force_ref;
        
        // Apply Master Gain
        norm_force *= m_gain;
        
        // Apply Min Force
        // If force is non-zero but smaller than min_force, boost it.
        // Also handle the zero case if necessary, but typically we want a minimal signal if *any* force exists.
        if (std::abs(norm_force) > 0.0001 && std::abs(norm_force) < m_min_force) {
            // Sign check
            double sign = (norm_force > 0.0) ? 1.0 : -1.0;
            norm_force = sign * m_min_force;
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
                snap.slip_angle = (float)((get_slip_angle(fl) + get_slip_angle(fr)) / 2.0);
                
                snap.patch_vel = (float)((std::abs(fl.mLateralPatchVel) + std::abs(fr.mLateralPatchVel)) / 2.0);
                snap.deflection = (float)((fl.mVerticalTireDeflection + fr.mVerticalTireDeflection) / 2.0);
                
                // Warnings
                snap.warn_load = frame_warn_load;
                snap.warn_grip = frame_warn_grip;
                snap.warn_dt = frame_warn_dt;

                m_debug_buffer.push_back(snap);
            }
        }

        // Clip
        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
};

#endif // FFBENGINE_H
