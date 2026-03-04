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
#include <optional>
#include <atomic>
#include <mutex>

// Constants

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
#endif

// --- FFB Loop (High Priority 1000Hz) ---
void FFBThread() {
    std::cout << "[FFB] Loop Started." << std::endl;
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

    // Extended monitors for Issue #133
    struct ChannelMonitor {
        RateMonitor monitor;
        double lastValue = -1e18;
        void Update(double newValue) {
            if (newValue != lastValue) {
                monitor.RecordEvent();
                lastValue = newValue;
            }
        }
    };

    ChannelMonitor mAccX, mAccY, mAccZ;
    ChannelMonitor mVelX, mVelY, mVelZ;
    ChannelMonitor mRotX, mRotY, mRotZ;
    ChannelMonitor mRotAccX, mRotAccY, mRotAccZ;
    ChannelMonitor mUnfSteer, mFilSteer;
    ChannelMonitor mRPM;
    ChannelMonitor mLoadFL, mLoadFR, mLoadRL, mLoadRR;
    ChannelMonitor mLatFL, mLatFR, mLatRL, mLatRR;
    ChannelMonitor mPosX, mPosY, mPosZ;
    ChannelMonitor mDtMon;

    // Precise Timing: Target 1000Hz (1000 microseconds)
    const std::chrono::microseconds target_period(1000);
    auto next_tick = std::chrono::steady_clock::now();

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
                in_realtime_phys = GameConnector::Get().CopyTelemetry(g_localData);
                bool is_stale = GameConnector::Get().IsStale(100);

                static bool was_in_menu = true;
                if (was_in_menu && in_realtime_phys) {
                    std::cout << "[Game] User entered driving session." << std::endl;
                    if (Config::m_auto_start_logging && !AsyncLogger::Get().IsLogging()) {
                        SessionInfo info;
                        info.app_version = LMUFFB_VERSION;
                        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
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
                        info.torque_passthrough = g_engine.m_torque_passthrough;
                        AsyncLogger::Get().Start(info, Config::m_log_path);
                    }
                } else if (!was_in_menu && !in_realtime_phys) {
                    std::cout << "[Game] User exited to menu (FFB Muted)." << std::endl;
                    if (Config::m_auto_start_logging && AsyncLogger::Get().IsLogging()) {
                        AsyncLogger::Get().Stop();
                    }
                }
                was_in_menu = !in_realtime_phys;

                if (!is_stale && g_localData.telemetry.playerHasVehicle) {
                    uint8_t idx = g_localData.telemetry.playerVehicleIdx;
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
                        mAccX.Update(pPlayerTelemetry->mLocalAccel.x);
                        mAccY.Update(pPlayerTelemetry->mLocalAccel.y);
                        mAccZ.Update(pPlayerTelemetry->mLocalAccel.z);
                        mVelX.Update(pPlayerTelemetry->mLocalVel.x);
                        mVelY.Update(pPlayerTelemetry->mLocalVel.y);
                        mVelZ.Update(pPlayerTelemetry->mLocalVel.z);
                        mRotX.Update(pPlayerTelemetry->mLocalRot.x);
                        mRotY.Update(pPlayerTelemetry->mLocalRot.y);
                        mRotZ.Update(pPlayerTelemetry->mLocalRot.z);
                        mRotAccX.Update(pPlayerTelemetry->mLocalRotAccel.x);
                        mRotAccY.Update(pPlayerTelemetry->mLocalRotAccel.y);
                        mRotAccZ.Update(pPlayerTelemetry->mLocalRotAccel.z);
                        mUnfSteer.Update(pPlayerTelemetry->mUnfilteredSteering);
                        mFilSteer.Update(pPlayerTelemetry->mFilteredSteering);
                        mRPM.Update(pPlayerTelemetry->mEngineRPM);
                        mLoadFL.Update(pPlayerTelemetry->mWheel[0].mTireLoad);
                        mLoadFR.Update(pPlayerTelemetry->mWheel[1].mTireLoad);
                        mLoadRL.Update(pPlayerTelemetry->mWheel[2].mTireLoad);
                        mLoadRR.Update(pPlayerTelemetry->mWheel[3].mTireLoad);
                        mLatFL.Update(pPlayerTelemetry->mWheel[0].mLateralForce);
                        mLatFR.Update(pPlayerTelemetry->mWheel[1].mLateralForce);
                        mLatRL.Update(pPlayerTelemetry->mWheel[2].mLateralForce);
                        mLatRR.Update(pPlayerTelemetry->mWheel[3].mLateralForce);
                        mPosX.Update(pPlayerTelemetry->mPos.x);
                        mPosY.Update(pPlayerTelemetry->mPos.y);
                        mPosZ.Update(pPlayerTelemetry->mPos.z);
                        mDtMon.Update(pPlayerTelemetry->mDeltaTime);

                        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                        bool full_allowed = g_engine.IsFFBAllowed(scoring, g_localData.scoring.scoringInfo.mGamePhase) && in_realtime_phys;

                        force_physics = g_engine.calculate_force(pPlayerTelemetry, scoring.mVehicleClass, scoring.mVehicleName, g_localData.generic.FFBTorque, full_allowed, 0.0025);
                        if (!in_realtime_phys) force_physics = 0.0;

                        // Safety Layer (v0.7.49): Slew Rate Limiting (400Hz)
                        // Applied before up-sampling to prevent reconstruction artifacts on spikes.
                        bool restricted = !full_allowed || (scoring.mFinishStatus != 0);
                        force_physics = g_engine.ApplySafetySlew(force_physics, 0.0025, restricted);

                        should_output = true;
                    }
                }
            }
            
            if (!should_output) {
                std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                force_physics = g_engine.ApplySafetySlew(0.0, 0.0025, true);
            }
            current_physics_force = force_physics;

            // Warning for low sample rate (Issue #133)
            static auto lastWarningTime = std::chrono::steady_clock::now();
            HealthStatus health;
            {
                std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                double t_rate = (g_engine.m_torque_source == 1) ? genTorqueMonitor.GetRate() : torqueMonitor.GetRate();
                health = HealthMonitor::Check(loopMonitor.GetRate(), telemMonitor.GetRate(), t_rate, g_engine.m_torque_source, physicsMonitor.GetRate());
            }

            if (in_realtime_phys && !health.is_healthy) {
                 auto now = std::chrono::steady_clock::now();
                 if (std::chrono::duration_cast<std::chrono::seconds>(now - lastWarningTime).count() >= 60) {
                     std::string reason = "";
                     if (health.loop_low) reason += "Loop=" + std::to_string((int)health.loop_rate) + "Hz ";
                     if (health.physics_low) reason += "Physics=" + std::to_string((int)health.physics_rate) + "Hz ";
                     if (health.telem_low) reason += "Telemetry=" + std::to_string((int)health.telem_rate) + "Hz ";
                     if (health.torque_low) reason += "Torque=" + std::to_string((int)health.torque_rate) + "Hz (Target " + std::to_string((int)health.expected_torque_rate) + "Hz) ";

                     Logger::Get().Log("Low Sample Rate detected: %s", reason.c_str());
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
        std::this_thread::sleep_until(next_tick);
    }

    std::cout << "[FFB] Loop Stopped." << std::endl;
}

#ifndef _WIN32
void handle_sigterm(int sig) {
    g_running = false;
}
#endif

#ifdef LMUFFB_UNIT_TEST
int lmuffb_app_main(int argc, char* argv[]) noexcept {
#else
int main(int argc, char* argv[]) noexcept {
#endif
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
        DirectInputFFB::Get().Initialize(reinterpret_cast<HWND>(GuiLayer::GetWindowHandle()));
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
    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal exception: %s\n", e.what());
        // Attempt to log if possible, but don't risk more throws
        try { Logger::Get().Log("Fatal exception: %s", e.what()); } catch(...) { (void)0; }
        return 1;
    } catch (...) {
        fprintf(stderr, "Fatal unknown exception.\n");
        try { Logger::Get().Log("Fatal unknown exception."); } catch(...) { (void)0; }
        return 1;
    }
}
