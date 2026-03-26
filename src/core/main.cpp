#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#endif
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <optional>
#include <atomic>
#include <mutex>

#include "FFBEngine.h"
#include "GuiLayer.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include "Version.h"
#include "Logger.h"    // Added Logger
#include "RateMonitor.h"
#include "HealthMonitor.h"
#include "UpSampler.h"
#include "TimeUtils.h"
#include "ChannelMonitor.h"

namespace LMUFFB {

// --- Helper for Testability ---
void PopulateSessionInfo(SessionInfo& info, const VehicleScoringInfoV01& scoring, const char* trackName, const FFBEngine& engine) {
    info.app_version = LMUFFB_VERSION;
    info.vehicle_name = scoring.mVehicleName;
    info.vehicle_class = VehicleClassToString(ParseVehicleClass(scoring.mVehicleClass, scoring.mVehicleName));
    info.vehicle_brand = ParseVehicleBrand(scoring.mVehicleClass, scoring.mVehicleName);
    info.track_name = trackName ? trackName : "Unknown";
    info.driver_name = "Auto";
    info.general = engine.m_general;
    info.front_axle = engine.m_front_axle;
    info.rear_axle = engine.m_rear_axle;
    info.load_forces = engine.m_load_forces;
    info.grip_estimation = engine.m_grip_estimation;
    info.slope_detection = engine.m_slope_detection;
    info.braking = engine.m_braking;
    info.vibration = engine.m_vibration;
    info.advanced = engine.m_advanced;
    info.safety = engine.m_safety.m_config;
}

// Threading Globals
#ifndef LMUFFB_UNIT_TEST
std::atomic<bool> g_running(true);
std::atomic<bool> g_ffb_active(true);

SharedMemoryObjectOut g_localData; // Local copy of shared memory

FFBEngine g_engine;
std::recursive_mutex g_engine_mutex; // Protects settings access if GUI changes them
#else
extern std::atomic<bool> g_running;
extern std::atomic<bool> g_ffb_active;
extern SharedMemoryObjectOut g_localData;
extern FFBEngine g_engine;
extern std::recursive_mutex g_engine_mutex;
extern std::chrono::steady_clock::time_point g_mock_time;
extern bool g_use_mock_time;
#endif

// --- FFB Loop (High Priority 1000Hz) ---
void FFBThread() {
    Logger::Get().LogFile("[FFB] Loop Started.");
    RateMonitor loopMonitor;
    RateMonitor telemMonitor;
    RateMonitor hwMonitor;
    RateMonitor torqueMonitor;
    RateMonitor genTorqueMonitor;
    RateMonitor physicsMonitor; // New v0.7.117 (Issue #217)

    PolyphaseResampler resampler;
    int phase_accumulator = 0;
    double current_physics_force = 0.0;

    double lastET = -1.0;
    double lastTorque = -9999.0;
    float lastGenTorque = -9999.0f;

    ChannelMonitors channelMonitors;

    // Precise Timing: Target 1000Hz (1000 microseconds)
    const std::chrono::microseconds target_period(1000);
    auto next_tick = TimeUtils::GetTime();

    while (g_running) {
        loopMonitor.RecordEvent();
        next_tick += target_period;

        // --- 1. Physics Phase Accumulator (5/2 Ratio) ---
        phase_accumulator += 2; // M = 2
        bool run_physics = false;

        if (phase_accumulator >= 5) { // L = 5
            phase_accumulator -= 5;
            run_physics = true;
        }

        if (run_physics) {
            physicsMonitor.RecordEvent();
            bool should_output = false;
            double force_physics = 0.0;

            bool in_realtime_phys = false;
            if (g_ffb_active && GameConnector::Get().IsConnected()) {
                GameConnector::Get().CopyTelemetry(g_localData);
                g_engine.m_metadata.UpdateMetadata(g_localData); // Update names/classes immediately

                in_realtime_phys = GameConnector::Get().IsInRealtime();
                long current_session = GameConnector::Get().GetSessionType();

                bool is_stale = GameConnector::Get().IsStale(100);

                static bool was_driving = false;
                static long last_session = -1;

                // is_driving uses IsPlayerActivelyDriving() which correctly gates on
                // inRealtime AND playerControl==0 AND gamePhase!=9 (paused).
                bool is_driving = GameConnector::Get().IsPlayerActivelyDriving();

                bool should_start_log = (is_driving && !was_driving);
                bool should_stop_log  = (!is_driving && was_driving);
                bool session_changed  = (is_driving && was_driving && last_session != -1 && current_session != last_session);

                if (session_changed) {
                    Logger::Get().LogFile("[Game] Session Type Changed (%ld -> %ld). Restarting log.", last_session, current_session);
                    AsyncLogger::Get().Stop();
                    should_start_log = true;
                }

                bool manual_start_requested = Config::m_auto_start_logging && !AsyncLogger::Get().IsLogging() && is_driving;

                if (should_start_log || manual_start_requested) {
                    if (should_start_log) Logger::Get().LogFile("[Game] User entered driving session (Control: Player).");
                    else Logger::Get().LogFile("[Game] Logging manually enabled while driving.");

                    if (Config::m_auto_start_logging && !AsyncLogger::Get().IsLogging()) {
                        uint8_t idx = g_localData.telemetry.playerVehicleIdx;
                        if (idx < 104) {
                            auto& scoring = g_localData.scoring.vehScoringInfo[idx];
                            const char* tName = g_localData.scoring.scoringInfo.mTrackName;

                            SessionInfo info;
                            {
                                std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                                PopulateSessionInfo(info, scoring, tName, g_engine);
                            }
                            AsyncLogger::Get().Start(info, Config::m_log_path);
                        }
                    }
                } else if (should_stop_log) {
                    Logger::Get().LogFile("[Game] User exited driving session.");
                    if (AsyncLogger::Get().IsLogging()) {
                        AsyncLogger::Get().Stop();
                    }
                }
                was_driving = is_driving;
                last_session = current_session;

                if (!is_stale && g_localData.telemetry.playerHasVehicle) {
                    uint8_t idx = g_localData.telemetry.playerVehicleIdx;

                    // --- LOST FRAME DETECTION (Issue #303) ---
                    static double last_telem_et = -1.0;
                    if (g_engine.m_safety.m_config.stutter_safety_enabled && last_telem_et > 0.0 && (g_localData.telemetry.telemInfo[idx].mElapsedTime - last_telem_et) > (g_localData.telemetry.telemInfo[idx].mDeltaTime * g_engine.m_safety.m_config.stutter_threshold)) {
                        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                        g_engine.m_safety.TriggerSafetyWindow("Lost Frames");
                    }
                    last_telem_et = g_localData.telemetry.telemInfo[idx].mElapsedTime;

                    if (idx < 104) {
                        auto& scoring = g_localData.scoring.vehScoringInfo[idx];
                        TelemInfoV01* pPlayerTelemetry = &g_localData.telemetry.telemInfo[idx];

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

                        // Extended monitoring (Issue #133)
                        channelMonitors.UpdateAll(pPlayerTelemetry);

                        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                        bool full_allowed = g_engine.m_safety.IsFFBAllowed(scoring, g_localData.scoring.scoringInfo.mGamePhase) && is_driving;

                        force_physics = g_engine.calculate_force(pPlayerTelemetry, scoring.mVehicleClass, scoring.mVehicleName, g_localData.generic.FFBTorque, full_allowed, 0.0025, scoring.mControl);

                        // v0.7.153: Explicitly target zero force only when player is not in control (Issue #281).
                        // This allows Soft Lock to remain active in the garage and during pause (ControlMode::PLAYER),
                        // while ensuring that AI takeover or other non-player states slew to zero.
                        if (scoring.mControl != static_cast<signed char>(ControlMode::PLAYER)) force_physics = 0.0;

                        // Safety Layer (v0.7.49): Slew Rate Limiting (400Hz)
                        // Applied before up-sampling to prevent reconstruction artifacts on spikes.
                        bool restricted = !full_allowed || (scoring.mFinishStatus != 0);
                        force_physics = g_engine.m_safety.ApplySafetySlew(force_physics, 0.0025, restricted);

                        should_output = true;
                    }
                }
            }
            
            if (!should_output) {
                std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                force_physics = g_engine.m_safety.ApplySafetySlew(0.0, 0.0025, true);
            }
            current_physics_force = force_physics;

            // Warning for low sample rate (Issue #133)
            static auto lastWarningTime = TimeUtils::GetTime();
            HealthStatus health;
            {
                std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                double t_rate = (g_engine.m_front_axle.torque_source == 1) ? genTorqueMonitor.GetRate() : torqueMonitor.GetRate();
                health = HealthMonitor::Check(loopMonitor.GetRate(), telemMonitor.GetRate(), t_rate, g_engine.m_front_axle.torque_source, physicsMonitor.GetRate(),
                                              GameConnector::Get().IsConnected(), GameConnector::Get().IsSessionActive(), GameConnector::Get().GetSessionType(), GameConnector::Get().IsInRealtime(), GameConnector::Get().GetPlayerControl());
            }

            if (in_realtime_phys && !health.is_healthy) {
                 auto now = TimeUtils::GetTime();
                 if (std::chrono::duration_cast<std::chrono::seconds>(now - lastWarningTime).count() >= 60) {
                     std::string reason = "";
                     if (health.loop_low) reason += "Loop=" + std::to_string((int)health.loop_rate) + "Hz ";
                     if (health.physics_low) reason += "Physics=" + std::to_string((int)health.physics_rate) + "Hz ";
                     if (health.telem_low) reason += "Telemetry=" + std::to_string((int)health.telem_rate) + "Hz ";
                     if (health.torque_low) reason += "Torque=" + std::to_string((int)health.torque_rate) + "Hz (Target " + std::to_string((int)health.expected_torque_rate) + "Hz) ";

                     Logger::Get().LogFile("Low Sample Rate detected: %s", reason.c_str());
                     lastWarningTime = now;
                 }
            }
        }

        // --- 2. 1000Hz Output Phase ---

        // Pass physics output through Polyphase Resampler
        double force = resampler.Process(current_physics_force, run_physics);

        // Push rates to engine for GUI/Snapshot (1000Hz)
        {
            std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
            g_engine.m_ffb_rate = loopMonitor.GetRate();
            g_engine.m_physics_rate = physicsMonitor.GetRate();
            g_engine.m_telemetry_rate = telemMonitor.GetRate();
            g_engine.m_hw_rate = hwMonitor.GetRate();
            g_engine.m_torque_rate = torqueMonitor.GetRate();
            g_engine.m_gen_torque_rate = genTorqueMonitor.GetRate();
        }

        if (DirectInputFFB::Get().UpdateForce(force)) {
            hwMonitor.RecordEvent();
        }

        // Precise Timing: Sleep until next tick
#ifdef LMUFFB_UNIT_TEST
        if (g_use_mock_time) {
            // In unit test mode with mock time, we don't sleep.
            // We expect the test to advance g_mock_time.
        } else {
            std::this_thread::sleep_until(next_tick);
        }
#else
        std::this_thread::sleep_until(next_tick);
#endif
    }

    Logger::Get().LogFile("[FFB] Loop Stopped.");
}

#ifndef _WIN32
void handle_sigterm(int sig) {
    g_running = false;
}
#endif

int lmuffb_app_main(int argc, char* argv[]) noexcept {
    try {
#ifdef _WIN32
        timeBeginPeriod(1);
#else
        signal(SIGTERM, handle_sigterm);
        signal(SIGINT, handle_sigterm);
#endif

    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--headless") headless = true;
    }

    // Initialize persistent debug logging for crash analysis
    // First init in current directory or logs folder to catch startup
    Logger::Get().Init("lmuffb_debug.log", "logs");
    Logger::Get().Log("Starting lmuFFB (C++ Port)...");
    Logger::Get().LogFile("Application Started. Version: %s", LMUFFB_VERSION);
    if (headless) Logger::Get().LogFile("Mode: HEADLESS");
    else Logger::Get().LogFile("Mode: GUI");

    Preset::ApplyDefaultsToEngine(g_engine);
    Config::Load(g_engine);

    // Re-initialize logger with user-configured path if it changed
    if (!Config::m_log_path.empty() && Config::m_log_path != "logs") {
        Logger::Get().Init("lmuffb_debug.log", Config::m_log_path);
        Logger::Get().LogFile("Logger re-initialized with user path: %s", Config::m_log_path.c_str());
    }

    if (!headless) {
        if (!GuiLayer::Init()) {
            Logger::Get().Log("Failed to initialize GUI.");
            Logger::Get().Log("Failed to initialize GUI.");
        }
        DirectInputFFB::Get().Initialize(reinterpret_cast<HWND>(GuiLayer::GetWindowHandle()));
    } else {
        Logger::Get().Log("Running in HEADLESS mode.");
        Logger::Get().Log("Running in HEADLESS mode.");
        DirectInputFFB::Get().Initialize(NULL);
    }

    if (GameConnector::Get().CheckLegacyConflict()) {
        Logger::Get().LogFile("[Info] Legacy rF2 plugin detected (not a problem for LMU 1.2+)");
    }

    if (!GameConnector::Get().TryConnect()) {
        Logger::Get().LogFile("Game not running or Shared Memory not ready. Waiting...");
    }

    std::thread ffb_thread(FFBThread);
    Logger::Get().LogFile("[GUI] Main Loop Started.");

    while (g_running) {
        GuiLayer::Render(g_engine);

        // Process background save requests from the FFB thread (v0.7.70)
        if (Config::m_needs_save.exchange(false)) {
            Config::Save(g_engine);
        }

        // Maintain a consistent 60Hz message loop even when backgrounded
        // to ensure DirectInput performance and reliability.
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    Config::Save(g_engine);
    if (!headless) {
        Logger::Get().LogFile("Shutting down GUI...");
        GuiLayer::Shutdown(g_engine);
    }
    if (ffb_thread.joinable()) {
        Logger::Get().LogFile("Stopping FFB Thread...");
        g_running = false; // Ensure loop breaks
        ffb_thread.join();
        Logger::Get().LogFile("FFB Thread Stopped.");
    }
    DirectInputFFB::Get().Shutdown();
    Logger::Get().Log("Main Loop Ended. Clean Exit.");
    
    return 0;
    } catch (const std::exception& e) {
        Logger::Get().LogFile("Fatal exception: %s", e.what());
        return 1;
    } catch (...) {
        Logger::Get().LogFile("Fatal unknown exception.");
        return 1;
    }
}

} // namespace LMUFFB

#ifndef LMUFFB_UNIT_TEST
int main(int argc, char* argv[]) noexcept {
    return LMUFFB::lmuffb_app_main(argc, argv);
}
#endif
