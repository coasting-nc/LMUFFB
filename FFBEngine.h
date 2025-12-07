#ifndef FFBENGINE_H
#define FFBENGINE_H

#include <cmath>
#include <algorithm>
#include "rF2Data.h"

// FFB Engine Class
class FFBEngine {
public:
    // Settings (GUI Sliders)
    float m_gain = 0.5f;          // Master Gain (Default 0.5 for safety)
    float m_smoothing = 0.5f;     // Smoothing factor (placeholder)
    float m_understeer_effect = 1.0f; // 0.0 - 1.0 (How much grip loss affects force)
    float m_sop_effect = 0.0f;    // 0.0 - 1.0 (Lateral G injection strength - Default 0 to prevent jerking)
    float m_min_force = 0.0f;     // 0.0 - 0.20 (Deadzone removal)
    
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

    // Internal state
    double m_prev_vert_deflection[2] = {0.0, 0.0}; // FL, FR
    
    // Phase Accumulators for Dynamic Oscillators
    double m_lockup_phase = 0.0;
    double m_spin_phase = 0.0;
    double m_slide_phase = 0.0;
    double m_bottoming_phase = 0.0;
    
    // Smoothing State
    double m_sop_lat_g_smoothed = 0.0;

    double calculate_force(const rF2Telemetry* data) {
        if (!data) return 0.0;
        
        double dt = data->mDeltaTime;
        const double TWO_PI = 6.28318530718;

        // Front Left and Front Right
        const rF2Wheel& fl = data->mWheels[0];
        const rF2Wheel& fr = data->mWheels[1];

        double game_force = data->mSteeringArmForce;

        // --- PRE-CALCULATION: TIRE LOAD FACTOR ---
        // Calculate this once to use across multiple effects.
        // Heavier load = stronger vibration transfer.
        double avg_load = (fl.mTireLoad + fr.mTireLoad) / 2.0;
        
        // Normalize: 4000N is a reference "loaded" GT tire.
        double load_factor = avg_load / 4000.0;
        
        // SAFETY CLAMP: Cap at 1.5x to prevent violent jolts during high-compression
        // or hard clipping when the user already has high gain.
        load_factor = (std::min)(1.5, (std::max)(0.0, load_factor));

        // --- 1. Understeer Effect (Grip Modulation) ---
        // Grip Fraction (Average of front tires)
        double grip_l = fl.mGripFract;
        double grip_r = fr.mGripFract;
        double avg_grip = (grip_l + grip_r) / 2.0;
        
        // Clamp grip 0-1 for safety
        avg_grip = (std::max)(0.0, (std::min)(1.0, avg_grip));
        
        double grip_factor = 1.0 - ((1.0 - avg_grip) * m_understeer_effect);
        double output_force = game_force * grip_factor;

        // --- 2. Seat of Pants (SoP) / Oversteer ---
        // Lateral G-force
        double lat_g = data->mLocalAccel.x / 9.81;
        
        // SoP Smoothing (Simple Low Pass Filter)
        // Alpha = dt / (RC + dt). RC = 1.0 / (2 * PI * freq).
        // For ~5Hz cutoff (smooth but responsive):
        double alpha = 0.1; // Placeholder for now, can be dynamic
        m_sop_lat_g_smoothed = m_sop_lat_g_smoothed + alpha * (lat_g - m_sop_lat_g_smoothed);
        
        double sop_force = m_sop_lat_g_smoothed * m_sop_effect * 1000.0; // Base scaling needs tuning
        
        // Oversteer Boost: If Rear Grip < Front Grip (car is rotating), boost SoP
        double grip_rl = data->mWheels[2].mGripFract;
        double grip_rr = data->mWheels[3].mGripFract;
        double avg_rear_grip = (grip_rl + grip_rr) / 2.0;
        
        // Delta between front and rear grip
        // If front has 1.0 grip and rear has 0.5, delta is 0.5. Boost SoP.
        double grip_delta = avg_grip - avg_rear_grip;
        if (grip_delta > 0.0) {
            sop_force *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
        }
        
        // --- 2a. Rear Aligning Torque Integration (Experimental) ---
        // Add a "Counter-Steer" suggestion based on rear axle force.
        // Approx: RearLateralForce * Trail. 
        // We modulate this by oversteer_boost as well.
        double rear_lat_force = (data->mWheels[2].mLateralForce + data->mWheels[3].mLateralForce) / 2.0;
        // Trail approx: 0.03m (3cm). Scaling factor required.
        // Sign correction: If rear force pushes left, wheel should turn right (align).
        // rF2 signs: LatForce +Y is Left? We need to tune this.
        // For now, simpler approximation: if LocalRotAccel.y (Yaw Accel) is high, add counter torque.
        // Let's stick to the grip delta for now but add a yaw-rate damper if needed.
        // Actually, let's inject a fraction of rear lateral force directly.
        double rear_torque = rear_lat_force * 0.05 * m_oversteer_boost; // 0.05 is arb scale
        sop_force += rear_torque;

        double total_force = output_force + sop_force;
        
        // --- 2b. Progressive Lockup (Dynamic) ---
        // Ensure phase updates even if force is small, but gated by enabled
        if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
            double slip_fl = data->mWheels[0].mSlipRatio;
            double slip_fr = data->mWheels[1].mSlipRatio;
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
                double car_speed_ms = std::abs(data->mLocalVel.z); 
                double freq = 10.0 + (car_speed_ms * 1.5); 

                // PHASE ACCUMULATION
                m_lockup_phase += freq * dt * TWO_PI;
                if (m_lockup_phase > TWO_PI) m_lockup_phase -= TWO_PI;

                double amp = severity * m_lockup_gain * 800.0;
                
                // Use the integrated phase
                double rumble = std::sin(m_lockup_phase) * amp;
                total_force += rumble;
            }
        }

        // --- 2c. Wheel Spin (Tire Physics Based) ---
        if (m_spin_enabled && data->mUnfilteredThrottle > 0.05) {
            double slip_rl = data->mWheels[2].mSlipRatio;
            double slip_rr = data->mWheels[3].mSlipRatio;
            double max_slip = (std::max)(slip_rl, slip_rr);
            
            if (max_slip > 0.2) {
                double severity = (max_slip - 0.2) / 0.5;
                severity = (std::min)(1.0, severity);
                
                // 1. Torque Drop (Floating feel)
                total_force *= (1.0 - (severity * m_spin_gain * 0.6)); 

                // 2. Vibration Frequency: Based on SLIP SPEED (Not RPM)
                // Calculate how fast the tire surface is moving relative to the road.
                // Slip Speed (m/s) approx = Car Speed (m/s) * Slip Ratio
                double car_speed_ms = std::abs(data->mLocalVel.z);
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
                double amp = severity * m_spin_gain * 500.0;
                double rumble = std::sin(m_spin_phase) * amp;
                
                total_force += rumble;
            }
        }

        // --- 3. Slide Texture (Dynamic Sawtooth) ---
        if (m_slide_texture_enabled) {
            double avg_slip = (std::abs(fl.mSlipAngle) + std::abs(fr.mSlipAngle)) / 2.0;
            
            if (avg_slip > 0.15) { // ~8 degrees
                // Frequency: Scrubbing speed
                // Using mLateralPatchVel is more accurate than GroundVel for rubber sliding
                double lat_vel = (std::abs(fl.mLateralPatchVel) + std::abs(fr.mLateralPatchVel)) / 2.0;
                
                // Map 1 m/s -> 40Hz, 10 m/s -> 200Hz
                double freq = 40.0 + (lat_vel * 17.0);
                if (freq > 250.0) freq = 250.0;

                m_slide_phase += freq * dt * TWO_PI;
                if (m_slide_phase > TWO_PI) m_slide_phase -= TWO_PI;

                // Sawtooth wave
                double sawtooth = (m_slide_phase / TWO_PI) * 2.0 - 1.0;

                // Amplitude: Scaled by PRE-CALCULATED global load_factor
                double noise = sawtooth * m_slide_texture_gain * 300.0 * load_factor;
                total_force += noise;
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
            double road_noise = (delta_l + delta_r) * 5000.0 * m_road_texture_gain; 
            
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
                double bump_magnitude = std::sqrt(excess) * m_bottoming_gain * 0.5;
                
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
        double max_force_ref = 4000.0; 
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

        // Clip
        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
};

#endif // FFBENGINE_H
