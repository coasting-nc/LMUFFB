#include <windows.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

#include "rF2Data.h"
#include "FFBEngine.h"
#include "src/GuiLayer.h"

// vJoy Interface Headers (Assume user has these in include path)
#include "public.h"
#include "vjoyinterface.h"

// Constants
const char* SHARED_MEMORY_NAME = "$rFactor2SMMP_Telemetry$";
const int VJOY_DEVICE_ID = 1;

#include <atomic>
#include <mutex>

// Threading Globals
std::atomic<bool> g_running(true);
std::atomic<bool> g_ffb_active(true);
rF2Telemetry* g_pTelemetry = nullptr;
FFBEngine g_engine;
std::mutex g_engine_mutex; // Protects settings access if GUI changes them

// --- FFB Loop (High Priority 400Hz) ---
void FFBThread() {
    long axis_min = 1;
    long axis_max = 32768;
    
    // Acquire vJoy
    if (!vJoyEnabled()) {
        std::cerr << "vJoy driver not enabled." << std::endl;
        g_running = false;
        return;
    }
    VjdStat status = GetVJDStatus(VJOY_DEVICE_ID);
    if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (!AcquireVJD(VJOY_DEVICE_ID)))) {
        std::cerr << "Failed to acquire vJoy device " << VJOY_DEVICE_ID << std::endl;
        g_running = false;
        return;
    }

    std::cout << "[FFB] Loop Started." << std::endl;

    while (g_running) {
        if (g_ffb_active && g_pTelemetry) {
            double force = 0.0;
            {
                // Lock only if we are changing settings often, otherwise std::atomic for settings is better.
                // For this prototype, we assume minimal contention.
                // std::lock_guard<std::mutex> lock(g_engine_mutex);
                force = g_engine.calculate_force(g_pTelemetry);
            }

            long axis_val = (long)((force + 1.0) * 0.5 * (axis_max - axis_min) + axis_min);
            SetAxis(axis_val, VJOY_DEVICE_ID, HID_USAGE_X);
        }

        // Sleep 2ms ~ 500Hz. Ideally use high_resolution_clock wait for precise 400Hz.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    RelinquishVJD(VJOY_DEVICE_ID);
    std::cout << "[FFB] Loop Stopped." << std::endl;
}

// --- GUI / Main Loop (Low Priority 60Hz or Lazy) ---
int main() {
    std::cout << "Starting LMUFFB (C++ Port)..." << std::endl;

    // 1. Setup Shared Memory
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        std::cerr << "Could not open file mapping object. Ensure game is running." << std::endl;
        // In a real app, show a popup and wait for game
        return 1;
    }

    g_pTelemetry = (rF2Telemetry*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(rF2Telemetry));
    if (g_pTelemetry == NULL) {
        std::cerr << "Could not map view of file." << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }
    std::cout << "Connected to Shared Memory." << std::endl;

    // 2. Start FFB Thread
    std::thread ffb_thread(FFBThread);

    // 3. Main GUI Loop
    std::cout << "[GUI] Main Loop Started. Press Ctrl+C to exit." << std::endl;
    
    if (!GuiLayer::Init()) {
        std::cerr << "Failed to initialize GUI." << std::endl;
        g_running = false;
    }

    while (g_running) {
        // Render returns true if the GUI is active (mouse over, focused).
        // If false, we can sleep longer (Lazy Rendering).
        bool active = GuiLayer::Render(g_engine);
        
        if (active) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // ~10Hz Background
        }
    }
    
    // Cleanup
    GuiLayer::Shutdown();
    if (ffb_thread.joinable()) ffb_thread.join();
    
    UnmapViewOfFile(g_pTelemetry);
    CloseHandle(hMapFile);
    
    return 0;
}
