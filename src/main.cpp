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
#include "GameConnector.h"
#include "Version.h"
#include "Logger.h"    // Added Logger
#include "RateMonitor.h"
#include <optional>
#include <atomic>
#include <mutex>

// Constants

// Threading Globals
std::atomic<bool> g_running(true);
std::atomic<bool> g_ffb_active(true);

SharedMemoryObjectOut g_localData; // Local copy of shared memory

FFBEngine g_engine;
std::mutex g_engine_mutex; // Protects settings access if GUI changes them

// --- FFB Loop (High Priority 400Hz) ---
void FFBThread() {
    std::cout << "[FFB] Loop Started." << std::endl;
    RateMonitor loopMonitor;
    RateMonitor telemMonitor;
    RateMonitor hwMonitor;
    RateMonitor torqueMonitor;
    RateMonitor genTorqueMonitor;
    double lastET = -1.0;
    double lastTorque = -9999.0;
    float lastGenTorque = -9999.0f;

    // Precise Timing: Target 400Hz (2500 microseconds)
    const std::chrono::microseconds target_period(2500);
    auto next_tick = std::chrono::steady_clock::now();

    while (g_running) {
        loopMonitor.RecordEvent();
        next_tick += target_period;

        double force = 0.0;
        double dt = 0.0025; // Default 400Hz
        bool restricted = true;

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
                    info.slope_threshold = (float)g_engine.m_slope_min_threshold;
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
            
            bool should_output = false;

            if (in_realtime && !is_stale && g_localData.telemetry.playerHasVehicle) {
                uint8_t idx = g_localData.telemetry.playerVehicleIdx;
                if (idx < 104) {
                    auto& scoring = g_localData.scoring.vehScoringInfo[idx];
                    TelemInfoV01* pPlayerTelemetry = &g_localData.telemetry.telemInfo[idx];
                    dt = pPlayerTelemetry->mDeltaTime;

                    // Track telemetry update rate
                    if (pPlayerTelemetry->mElapsedTime != lastET) {
                        telemMonitor.RecordEvent();
                        lastET = pPlayerTelemetry->mElapsedTime;
                    }

                    // Track torque update rates
                    if (pPlayerTelemetry->mSteeringShaftTorque != lastTorque) {
                        torqueMonitor.RecordEvent();
                        lastTorque = pPlayerTelemetry->mSteeringShaftTorque;
                    }
                    if (g_localData.generic.FFBTorque != lastGenTorque) {
                        genTorqueMonitor.RecordEvent();
                        lastGenTorque = g_localData.generic.FFBTorque;
                    }

                    std::lock_guard<std::mutex> lock(g_engine_mutex);
                    if (g_engine.IsFFBAllowed(scoring, g_localData.scoring.scoringInfo.mGamePhase)) {
                        force = g_engine.calculate_force(pPlayerTelemetry, scoring.mVehicleClass, scoring.mVehicleName, g_localData.generic.FFBTorque);
                        should_output = true;
                        restricted = (scoring.mFinishStatus != 0);
                    }
                }
            }
            
            if (!should_output) force = 0.0;

            // Warning for low sample rate (Issue #129)
            static auto lastWarningTime = std::chrono::steady_clock::now();
            if (in_realtime && (telemMonitor.GetRate() < 380.0 || torqueMonitor.GetRate() < 380.0) && telemMonitor.GetRate() > 1.0) {
                 auto now = std::chrono::steady_clock::now();
                 if (std::chrono::duration_cast<std::chrono::seconds>(now - lastWarningTime).count() >= 5) {
                     std::cout << "[WARNING] Low Sample Rate detected: Telemetry=" << telemMonitor.GetRate() << "Hz, Torque=" << torqueMonitor.GetRate() << "Hz (Target: 400 Hz)" << std::endl;
                     Logger::Get().Log("Low Sample Rate detected: Telemetry=%.1fHz, Torque=%.1fHz", telemMonitor.GetRate(), torqueMonitor.GetRate());
                     lastWarningTime = now;
                 }
            }
        }

        // Safety Layer (v0.7.49): Slew Rate Limiting and NaN protection
        // v0.7.48: Always update hardware even if disconnected/inactive to ensure zeroing
        {
            std::lock_guard<std::mutex> lock(g_engine_mutex);
            if (dt < 0.0001) dt = 0.0025;

            // Push rates to engine for GUI/Snapshot
            g_engine.m_ffb_rate = loopMonitor.GetRate();
            g_engine.m_telemetry_rate = telemMonitor.GetRate();
            g_engine.m_hw_rate = hwMonitor.GetRate();
            g_engine.m_torque_rate = torqueMonitor.GetRate();
            g_engine.m_gen_torque_rate = genTorqueMonitor.GetRate();

            force = g_engine.ApplySafetySlew(force, dt, restricted);
        }

        if (DirectInputFFB::Get().UpdateForce(force)) {
            hwMonitor.RecordEvent();
        }

        // Precise Timing: Sleep until next tick
        std::this_thread::sleep_until(next_tick);
    }

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
    // Initialize persistent debug logging for crash analysis
    Logger::Get().Init("lmuffb_debug.log");
    Logger::Get().Log("Application Started. Version: %s", LMUFFB_VERSION);
    if (headless) Logger::Get().Log("Mode: HEADLESS");
    else Logger::Get().Log("Mode: GUI");

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
        GuiLayer::Render(g_engine);
        // Maintain a consistent 60Hz message loop even when backgrounded
        // to ensure DirectInput performance and reliability.
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    Config::Save(g_engine);
    if (!headless) {
        Logger::Get().Log("Shutting down GUI...");
        GuiLayer::Shutdown(g_engine);
    }
    if (ffb_thread.joinable()) {
        Logger::Get().Log("Stopping FFB Thread...");
        g_running = false; // Ensure loop breaks
        ffb_thread.join();
        Logger::Get().Log("FFB Thread Stopped.");
    }
    DirectInputFFB::Get().Shutdown();
    Logger::Get().Log("Main Loop Ended. Clean Exit.");
    
    return 0;
}
