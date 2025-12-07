#include <windows.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

#include "rF2Data.h"
#include "FFBEngine.h"
#include "src/GuiLayer.h"
#include "src/Config.h"
#include "src/DirectInputFFB.h"
#include "src/DynamicVJoy.h"

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
    
    // Attempt to load vJoy
    bool vJoyActive = false;
    if (DynamicVJoy::Get().Load()) {
        if (DynamicVJoy::Get().Enabled()) {
            VjdStat status = DynamicVJoy::Get().GetStatus(VJOY_DEVICE_ID);
            if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && DynamicVJoy::Get().Acquire(VJOY_DEVICE_ID))) {
                vJoyActive = true;
                std::cout << "[vJoy] Device " << VJOY_DEVICE_ID << " acquired." << std::endl;
            } else {
                std::cerr << "[vJoy] Failed to acquire device " << VJOY_DEVICE_ID << std::endl;
            }
        } else {
            std::cout << "[vJoy] Driver not enabled." << std::endl;
        }
    } else {
        std::cerr << "[vJoy] Failed to load vJoyInterface.dll. Please ensure it is in the same folder as the executable." << std::endl;
        MessageBoxA(NULL, "Failed to load vJoyInterface.dll.\n\nPlease ensure vJoy is installed and the DLL is in the app folder.", "LMUFFB Error", MB_ICONERROR | MB_OK);
    }

    std::cout << "[FFB] Loop Started." << std::endl;

    while (g_running) {
        if (g_ffb_active && g_pTelemetry) {
            double force = 0.0;
            {
                // PROTECT SETTINGS: Use mutex because GUI modifies engine parameters
                std::lock_guard<std::mutex> lock(g_engine_mutex);
                force = g_engine.calculate_force(g_pTelemetry);
            }

            // Update vJoy Axis (for monitoring) if active
            if (vJoyActive) {
                long axis_val = (long)((force + 1.0) * 0.5 * (axis_max - axis_min) + axis_min);
                DynamicVJoy::Get().SetAxis(axis_val, VJOY_DEVICE_ID, 0x30); // 0x30 = HID_USAGE_X (hardcoded to avoid public.h dependency issues)
            }
            
            // Update DirectInput (for FFB)
            DirectInputFFB::Get().UpdateForce(force);
        }

        // Sleep 2ms ~ 500Hz. Ideally use high_resolution_clock wait for precise 400Hz.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    if (vJoyActive) {
        DynamicVJoy::Get().Relinquish(VJOY_DEVICE_ID);
    }
    std::cout << "[FFB] Loop Stopped." << std::endl;
}

// --- GUI / Main Loop (Low Priority 60Hz or Lazy) ---
int main(int argc, char* argv[]) {
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--headless") {
            headless = true;
        }
    }

    std::cout << "Starting LMUFFB (C++ Port)..." << std::endl;

    // Load Configuration
    Config::Load(g_engine);

    // Initialize GUI Early (if not headless)
    if (!headless) {
        if (!GuiLayer::Init()) {
            std::cerr << "Failed to initialize GUI." << std::endl;
            // Fallback? Or exit?
            // If explicit GUI build failed, we probably want to exit or warn.
            // For now, continue but set g_running false if critical.
            // Actually, GuiLayer::Init() handles window creation.
        }
        
        // Initialize DirectInput (Requires HWND)
        DirectInputFFB::Get().Initialize((HWND)GuiLayer::GetWindowHandle());
        
    } else {
        std::cout << "Running in HEADLESS mode." << std::endl;
        // Headless DI init (might fail if HWND is NULL but some drivers allow it, or windowless mode)
        DirectInputFFB::Get().Initialize(NULL);
    }

    // 1. Setup Shared Memory
    // We try to connect. If fails, we still keep the app running (if GUI is active) 
    // so user can see config and try again later or restart game.
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        std::cerr << "Could not open file mapping object. Ensure game is running." << std::endl;
        if (!headless) {
             // Show non-blocking error or just a popup but DON'T exit
             MessageBoxA(NULL, "Could not open file mapping object.\n\nApp will remain open. Start Le Mans Ultimate and restart this app or wait for connection logic (future feature).", "LMUFFB Warning", MB_ICONWARNING | MB_OK);
        } else {
             return 1; // Headless has no UI to wait, so exit
        }
    } else {
        g_pTelemetry = (rF2Telemetry*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(rF2Telemetry));
        if (g_pTelemetry == NULL) {
            std::cerr << "Could not map view of file." << std::endl;
            CloseHandle(hMapFile);
        } else {
            std::cout << "Connected to Shared Memory." << std::endl;
        }
    }

    // 2. Start FFB Thread
    std::thread ffb_thread(FFBThread);

    // 3. Main GUI Loop
    std::cout << "[GUI] Main Loop Started." << std::endl;

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
    
    // Save Config on Exit
    Config::Save(g_engine);

    // Cleanup
    if (!headless) GuiLayer::Shutdown();
    if (ffb_thread.joinable()) ffb_thread.join();
    
    DirectInputFFB::Get().Shutdown();
    
    if (g_pTelemetry) UnmapViewOfFile(g_pTelemetry);
    if (hMapFile) CloseHandle(hMapFile);
    
    return 0;
}
