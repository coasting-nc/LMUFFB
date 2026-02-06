#include <windows.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

#include "FFBEngine.h"
#include "GuiLayer.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "DynamicVJoy.h"
#include "GameConnector.h"
#include "Version.h"
#include "TelemetryProcessor.h"
#include <optional>

// Constants
const int VJOY_DEVICE_ID = 1;

#include <atomic>
#include <mutex>

// Threading Globals
std::atomic<bool> g_running(true);
std::atomic<bool> g_ffb_active(true);

SharedMemoryObjectOut g_localData; // Local copy of shared memory

FFBEngine g_engine;
std::mutex g_engine_mutex; // Protects settings access if GUI changes them

// --- FFB Loop (High Priority 400Hz) ---
void FFBThread() {
    long axis_min = 1;
    long axis_max = 32768;
    
    // Attempt to load vJoy (silently - no popups if missing)
    bool vJoyDllLoaded = false;
    if (DynamicVJoy::Get().Load()) {
        vJoyDllLoaded = true;
    } else {
        // vJoy not found - this is fine, DirectInput FFB works without it
        std::cout << "[vJoy] Not found (optional component, not required)" << std::endl;
    }

    // Track acquisition state locally
    bool vJoyAcquired = false;

    std::cout << "[FFB] Loop Started." << std::endl;

    while (g_running) {
        if (g_ffb_active && GameConnector::Get().IsConnected()) {
            
            // --- CRITICAL SECTION: READ DATA ---
            // CopyTelemetry now returns realtime status to avoid extra lock acquisition
            bool in_realtime = GameConnector::Get().CopyTelemetry(g_localData);
            
            // Check if player is in an active driving session (not in menu/replay)
            static bool was_in_menu = true;
            
            if (was_in_menu && in_realtime) {
                std::cout << "[Game] User entered driving session." << std::endl;
                
                // Auto-Start Logging
                if (Config::m_auto_start_logging && !AsyncLogger::Get().IsLogging()) {
                    SessionInfo info;
                    info.app_version = LMUFFB_VERSION;
                    
                    std::lock_guard<std::mutex> lock(g_engine_mutex);
                    info.vehicle_name = g_engine.m_vehicle_name;
                    info.track_name = g_engine.m_track_name;
                    info.driver_name = "Auto";
                    
                    info.gain = g_engine.m_gain;
                    info.understeer_effect = g_engine.m_understeer_effect;
                    info.sop_effect = g_engine.m_sop_effect;
                    info.slope_enabled = g_engine.m_slope_detection_enabled;
                    info.slope_sensitivity = g_engine.m_slope_sensitivity;
                    info.slope_threshold = (float)g_engine.m_slope_negative_threshold;
                    info.slope_alpha_threshold = g_engine.m_slope_alpha_threshold;
                    info.slope_decay_rate = g_engine.m_slope_decay_rate;
                    
                    AsyncLogger::Get().Start(info, Config::m_log_path);
                }
            } else if (!was_in_menu && !in_realtime) {
                std::cout << "[Game] User exited to menu (FFB Muted)." << std::endl;
                
                // Auto-Stop Logging
                if (Config::m_auto_start_logging && AsyncLogger::Get().IsLogging()) {
                    AsyncLogger::Get().Stop();
                }
            }
            was_in_menu = !in_realtime;
            
            double force = 0.0;
            bool should_output = false;

            // Only calculate FFB if actually driving
            if (in_realtime && g_localData.telemetry.playerHasVehicle) {
                uint8_t idx = g_localData.telemetry.playerVehicleIdx;
                if (idx < 104) {
                    // Get pointer to specific car data
                    TelemInfoV01* pPlayerTelemetry = &g_localData.telemetry.telemInfo[idx];
                    
                    {
                        // PROTECT SETTINGS: Use mutex because GUI modifies engine parameters
                        std::lock_guard<std::mutex> lock(g_engine_mutex);
                        force = g_engine.calculate_force(pPlayerTelemetry, &g_localData.scoring.scoringInfo);
                    }
                    should_output = true;
                }
            }
            
            // --- FIX: Explicitly send 0.0 if not driving ---
            if (!should_output) {
                force = 0.0;
            }

            // --- DYNAMIC vJoy LOGIC (State Machine) ---
            if (vJoyDllLoaded && DynamicVJoy::Get().Enabled()) { 
                // STATE 1: User enabled vJoy -> ACQUIRE
                if (Config::m_enable_vjoy && !vJoyAcquired) {
                    VjdStat status = DynamicVJoy::Get().GetStatus(VJOY_DEVICE_ID);
                    if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && DynamicVJoy::Get().Acquire(VJOY_DEVICE_ID))) {
                        vJoyAcquired = true;
                        std::cout << "[vJoy] Device " << VJOY_DEVICE_ID << " acquired." << std::endl;
                    }
                }
                // STATE 2: User disabled vJoy -> RELEASE
                else if (!Config::m_enable_vjoy && vJoyAcquired) {
                    DynamicVJoy::Get().Relinquish(VJOY_DEVICE_ID);
                    vJoyAcquired = false;
                    std::cout << "[vJoy] Device " << VJOY_DEVICE_ID << " relinquished." << std::endl;
                }

                // STATE 3: Update Axis (Only if Acquired AND Monitoring enabled)
                if (vJoyAcquired && Config::m_output_ffb_to_vjoy) {
                    long axis_val = (long)((force + 1.0) * 0.5 * (axis_max - axis_min) + axis_min);
                    DynamicVJoy::Get().SetAxis(axis_val, VJOY_DEVICE_ID, 0x30); 
                }
            }
            
            // Update DirectInput (Physical Wheel)
            // This will now send 0.0 when in menu/paused, releasing the tension.
            DirectInputFFB::Get().UpdateForce(force);
        }

        // Sleep 2ms ~ 500Hz. Ideally use high_resolution_clock wait for precise 400Hz.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    if (vJoyAcquired) {
        DynamicVJoy::Get().Relinquish(VJOY_DEVICE_ID);
    }
    std::cout << "[FFB] Loop Stopped." << std::endl;
}

// --- GUI / Main Loop (Low Priority 60Hz or Lazy) ---
int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Improve timer resolution for sleep accuracy (Report v0.4.2)
    timeBeginPeriod(1);
#endif

    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--headless") {
            headless = true;
        }
    }

    std::cout << "Starting lmuFFB (C++ Port)..." << std::endl;

    // Initialize FFBEngine with T300 defaults (Single Source of Truth: Config.h Preset struct)
    Preset::ApplyDefaultsToEngine(g_engine);

    // Load Configuration (overwrites defaults if config.ini exists)
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
    // Check for conflicts (silent - no popup, just log to console)
    if (GameConnector::Get().CheckLegacyConflict()) {
        std::cout << "[Info] Legacy rF2 plugin detected (not a problem for LMU 1.2+)" << std::endl;
    }

    if (!GameConnector::Get().TryConnect()) {
        std::cout << "Game not running or Shared Memory not ready. Waiting..." << std::endl;
        // Don't exit, just continue to GUI. FFB Loop will wait.
    }

    // 3. Start FFB Thread
    std::thread ffb_thread(FFBThread);

    // 4. Main GUI Loop
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
    if (!headless) GuiLayer::Shutdown(g_engine);
    if (ffb_thread.joinable()) ffb_thread.join();
    
    DirectInputFFB::Get().Shutdown();
    
    // GameConnector cleans itself up
    
    return 0;
}
