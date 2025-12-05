#include <windows.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

#include "rF2Data.h"

// vJoy Interface Headers (Assume user has these in include path)
#include "public.h"
#include "vjoyinterface.h"

// Constants
const char* SHARED_MEMORY_NAME = "$rFactor2SMMP_Telemetry$";
const int VJOY_DEVICE_ID = 1;

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
        // mGripFract is typically 0.0 (no grip) to 1.0 (full grip), can go higher?
        // Logic: Output = GameForce * (1.0 - (1.0 - Grip) * EffectStrength)
        // If EffectStrength is 0, Output = GameForce.
        // If EffectStrength is 1, Output = GameForce * Grip.
        
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
        // We do this on the normalized signal or absolute force? 
        // Usually better on the final sum before clip, or normalized.
        // Let's normalize first.
        
        double max_force_ref = 4000.0; 
        double norm_force = total_force / max_force_ref;
        
        // Apply Master Gain
        norm_force *= m_gain;
        
        // Apply Min Force
        if (std::abs(norm_force) < m_min_force && std::abs(norm_force) > 0.001) {
            // Sign check
            double sign = (norm_force > 0) ? 1.0 : -1.0;
            norm_force = sign * m_min_force;
        }

        // Clip
        return std::max(-1.0, std::min(1.0, norm_force));
    }
};

int main() {
    std::cout << "Starting LMUFFB (C++ Port)..." << std::endl;

    // 1. Setup Shared Memory
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        // Retry loop could be added here
        std::cerr << "Could not open file mapping object (" << GetLastError() << ")." << std::endl;
        std::cerr << "Make sure the game is running and the Shared Memory Plugin is enabled." << std::endl;
        return 1;
    }

    auto* pTelemetry = (rF2Telemetry*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(rF2Telemetry));
    if (pTelemetry == NULL) {
        std::cerr << "Could not map view of file (" << GetLastError() << ")." << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    std::cout << "Connected to Shared Memory." << std::endl;

    // 2. Setup vJoy
    if (!vJoyEnabled()) {
        std::cerr << "vJoy driver not enabled - Failed to initialize." << std::endl;
        return 1;
    }

    VjdStat status = GetVJDStatus(VJOY_DEVICE_ID);
    if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (!AcquireVJD(VJOY_DEVICE_ID)))) {
        std::cerr << "Failed to acquire vJoy device " << VJOY_DEVICE_ID << std::endl;
        return 1;
    }
    std::cout << "Acquired vJoy Device " << VJOY_DEVICE_ID << std::endl;

    // 3. Main Loop
    FFBEngine engine;
    long axis_min = 1;
    long axis_max = 32768;

    std::cout << "Running... Press Ctrl+C to stop." << std::endl;

    try {
        while (true) {
            // Read and Calculate
            double force = engine.calculate_force(pTelemetry);

            // Scale to vJoy Axis
            long axis_val = (long)((force + 1.0) * 0.5 * (axis_max - axis_min) + axis_min);

            // Output
            SetAxis(axis_val, VJOY_DEVICE_ID, HID_USAGE_X);

            // 400Hz Loop = 2.5ms
            // Using Sleep(2) is rough; high-res timer would be better (e.g., std::this_thread::sleep_until)
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); 
        }
    }
    catch (...) {
        std::cerr << "Exception occurred." << std::endl;
    }

    // Cleanup
    RelinquishVJD(VJOY_DEVICE_ID);
    UnmapViewOfFile(pTelemetry);
    CloseHandle(hMapFile);

    return 0;
}
