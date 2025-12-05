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
    double m_smoothing = 0.5;
    double m_gain = 1.0;
    double m_sop_factor = 0.5;

    double calculate_force(const rF2Telemetry* data) {
        if (!data) return 0.0;

        // Front Left and Front Right
        const rF2Wheel& fl = data->mWheels[0];
        const rF2Wheel& fr = data->mWheels[1];

        double game_force = data->mSteeringArmForce;

        // Grip Fraction (Average of front tires)
        double grip_l = fl.mGripFract;
        double grip_r = fr.mGripFract;
        double avg_grip = (grip_l + grip_r) / 2.0;

        // 1. Grip Modulation
        // As grip drops (< 1.0), we reduce the force to simulate lightness
        double output_force = game_force * avg_grip;

        // 2. Seat of Pants (SoP)
        // Lateral G-force
        double lat_g = data->mLocalAccel.x / 9.81;
        double sop_force = lat_g * m_sop_factor * 1000.0; // Scale factor matching Python

        double total_force = output_force + sop_force;

        // Normalize (Reference max force ~4000)
        double max_force_ref = 4000.0;
        double norm_force = total_force / max_force_ref;

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
