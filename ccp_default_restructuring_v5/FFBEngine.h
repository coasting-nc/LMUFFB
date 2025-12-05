#ifndef FFBENGINE_H
#define FFBENGINE_H

#include <cmath>
#include <algorithm>
#include "rF2Data.h"

// FFB Engine Class
class FFBEngine {
public:
    // Settings (GUI Sliders)
    double m_gain = 1.0;          // Master Gain (0.0 - 2.0)
    double m_smoothing = 0.5;     // Smoothing factor (placeholder)
    double m_understeer_effect = 1.0; // 0.0 - 1.0 (How much grip loss affects force)
    double m_sop_effect = 0.5;    // 0.0 - 1.0 (Lateral G injection strength)
    double m_min_force = 0.0;     // 0.0 - 0.20 (Deadzone removal)
    
    // Texture toggles
    bool m_slide_texture_enabled = true;
    double m_slide_texture_gain = 0.5; // 0.0 - 1.0
    
    bool m_road_texture_enabled = false;
    double m_road_texture_gain = 0.5; // 0.0 - 1.0

    // Internal state
    double m_prev_vert_deflection[2] = {0.0, 0.0}; // FL, FR

    double calculate_force(const rF2Telemetry* data) {
        if (!data) return 0.0;

        // Front Left and Front Right
        const rF2Wheel& fl = data->mWheels[0];
        const rF2Wheel& fr = data->mWheels[1];

        double game_force = data->mSteeringArmForce;

        // --- 1. Understeer Effect (Grip Modulation) ---
        // Grip Fraction (Average of front tires)
        double grip_l = fl.mGripFract;
        double grip_r = fr.mGripFract;
        double avg_grip = (grip_l + grip_r) / 2.0;
        
        // Clamp grip 0-1 for safety
        avg_grip = std::max(0.0, std::min(1.0, avg_grip));
        
        double grip_factor = 1.0 - ((1.0 - avg_grip) * m_understeer_effect);
        double output_force = game_force * grip_factor;

        // --- 2. Seat of Pants (SoP) / Oversteer ---
        // Lateral G-force
        double lat_g = data->mLocalAccel.x / 9.81;
        double sop_force = lat_g * m_sop_effect * 1000.0; // Base scaling needs tuning
        
        double total_force = output_force + sop_force;
        
        // --- 3. Slide Texture (Detail Booster) ---
        if (m_slide_texture_enabled) {
            // Simple noise generator triggered by slip angle or grip loss
            // If slipping, inject high freq noise
            double avg_slip = (std::abs(fl.mSlipAngle) + std::abs(fr.mSlipAngle)) / 2.0;
            // Threshold for sliding (radians? degrees? rF2 uses radians usually)
            // Let's assume > 0.1 rad (~5 deg) is sliding
            if (avg_slip > 0.1 || avg_grip < 0.8) {
                // Pseudo-random noise based on time or just a sine wave
                double noise = std::sin(data->mElapsedTime * 500.0) * m_slide_texture_gain * 200.0;
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
            total_force += road_noise;
        }

        // --- 5. Min Force (Deadzone Removal) ---
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
        return std::max(-1.0, std::min(1.0, norm_force));
    }
};

#endif // FFBENGINE_H
