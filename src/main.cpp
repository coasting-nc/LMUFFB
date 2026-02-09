#ifdef _WIN32
#include <windows.h>
#endif
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
#include <optional>
#include <atomic>
#include <mutex>

// Constants
const int VJOY_DEVICE_ID = 1;

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
    
    // Attempt to load vJoy (silently)
    bool vJoyDllLoaded = false;
    if (DynamicVJoy::Get().Load()) {
        vJoyDllLoaded = true;
    } else {
        std::cout << "[vJoy] Not found (optional component, not required)" << std::endl;
    }

    bool vJoyAcquired = false;
    std::cout << "[FFB] Loop Started." << std::endl;

    while (g_running) {
        if (g_ffb_active && GameConnector::Get().IsConnected()) {
            bool in_realtime = GameConnector::Get().CopyTelemetry(g_localData);
            bool is_stale = GameConnector::Get().IsStale(100);

            static bool was_in_menu = true;
            if (was_in_menu && in_realtime) {
                std::cout << "[Game] User entered driving session." << std::endl;
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
                if (Config::m_auto_start_logging && AsyncLogger::Get().IsLogging()) {
                    AsyncLogger::Get().Stop();
                }
            }
            was_in_menu = !in_realtime;
            
            double force = 0.0;
            bool should_output = false;

            if (in_realtime && !is_stale && g_localData.telemetry.playerHasVehicle) {
                uint8_t idx = g_localData.telemetry.playerVehicleIdx;
                if (idx < 104) {
                    TelemInfoV01* pPlayerTelemetry = &g_localData.telemetry.telemInfo[idx];
                    {
                        std::lock_guard<std::mutex> lock(g_engine_mutex);
                        force = g_engine.calculate_force(pPlayerTelemetry);
                    }
                    should_output = true;
                }
            }
            
            if (!should_output) force = 0.0;

            if (vJoyDllLoaded && DynamicVJoy::Get().Enabled()) { 
                if (Config::m_enable_vjoy && !vJoyAcquired) {
                    VjdStat status = DynamicVJoy::Get().GetStatus(VJOY_DEVICE_ID);
                    if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && DynamicVJoy::Get().Acquire(VJOY_DEVICE_ID))) {
                        vJoyAcquired = true;
                        std::cout << "[vJoy] Device " << VJOY_DEVICE_ID << " acquired." << std::endl;
                    }
                } else if (!Config::m_enable_vjoy && vJoyAcquired) {
                    DynamicVJoy::Get().Relinquish(VJOY_DEVICE_ID);
                    vJoyAcquired = false;
                    std::cout << "[vJoy] Device " << VJOY_DEVICE_ID << " relinquished." << std::endl;
                }
                if (vJoyAcquired && Config::m_output_ffb_to_vjoy) {
                    long axis_val = (long)((force + 1.0) * 0.5 * (axis_max - axis_min) + axis_min);
                    DynamicVJoy::Get().SetAxis(axis_val, VJOY_DEVICE_ID, 0x30); 
                }
            }
            DirectInputFFB::Get().UpdateForce(force);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    if (vJoyAcquired) DynamicVJoy::Get().Relinquish(VJOY_DEVICE_ID);
    std::cout << "[FFB] Loop Stopped." << std::endl;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--headless") headless = true;
    }

    std::cout << "Starting lmuFFB (C++ Port)..." << std::endl;
    Preset::ApplyDefaultsToEngine(g_engine);
    Config::Load(g_engine);

    if (!headless) {
        if (!GuiLayer::Init()) {
            std::cerr << "Failed to initialize GUI." << std::endl;
        }
        DirectInputFFB::Get().Initialize((HWND)GuiLayer::GetWindowHandle());
    } else {
        std::cout << "Running in HEADLESS mode." << std::endl;
        DirectInputFFB::Get().Initialize(NULL);
    }

    if (GameConnector::Get().CheckLegacyConflict()) {
        std::cout << "[Info] Legacy rF2 plugin detected (not a problem for LMU 1.2+)" << std::endl;
    }

    if (!GameConnector::Get().TryConnect()) {
        std::cout << "Game not running or Shared Memory not ready. Waiting..." << std::endl;
    }

    std::thread ffb_thread(FFBThread);
    std::cout << "[GUI] Main Loop Started." << std::endl;

    while (g_running) {
        bool active = GuiLayer::Render(g_engine);
        if (active) std::this_thread::sleep_for(std::chrono::milliseconds(16));
        else std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    Config::Save(g_engine);
    if (!headless) GuiLayer::Shutdown(g_engine);
    if (ffb_thread.joinable()) ffb_thread.join();
    DirectInputFFB::Get().Shutdown();
    
    return 0;
}
