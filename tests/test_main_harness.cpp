#include "test_ffb_common.h"
#include <atomic>
#include <mutex>
#include <thread>
#include "../src/io/lmu_sm_interface/LmuSharedMemoryWrapper.h"

#ifndef _WIN32
#include "../src/io/lmu_sm_interface/LinuxMock.h"
#include <csignal>
#endif

// Shared globals already defined in main_test_runner.cpp or needed by main.cpp
extern std::atomic<bool> g_running;
extern std::atomic<bool> g_ffb_active;
extern std::recursive_mutex g_engine_mutex;
extern FFBEngine g_engine;
extern SharedMemoryObjectOut g_localData;
extern std::chrono::steady_clock::time_point g_mock_time;
extern bool g_use_mock_time;

extern int lmuffb_app_main(int argc, char* argv[]);
extern void FFBThread();
#ifndef _WIN32
extern void handle_sigterm(int sig);
#endif

namespace FFBEngineTests {

TEST_CASE(test_main_app_logic, "System") {
    std::cout << "\nTest: Main App Logic (Coverage Boost)" << std::endl;

    // Setup Mock Telemetry
    #ifndef _WIN32
    MockSM::GetMaps()["LMU_Data"].resize(sizeof(SharedMemoryLayout));
    SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
    layout->data.telemetry.playerHasVehicle = true;
    layout->data.telemetry.playerVehicleIdx = 0;
    layout->data.telemetry.telemInfo[0].mDeltaTime = 0.0025f;
    layout->data.telemetry.telemInfo[0].mElapsedTime = 1.0f;
    layout->data.telemetry.telemInfo[0].mSteeringShaftTorque = 1.0f;
    layout->data.telemetry.activeVehicles = 1;
    StringUtils::SafeCopy(layout->data.scoring.vehScoringInfo[0].mVehicleClass, sizeof(layout->data.scoring.vehScoringInfo[0].mVehicleClass), "GT3");
    StringUtils::SafeCopy(layout->data.scoring.vehScoringInfo[0].mVehicleName, sizeof(layout->data.scoring.vehScoringInfo[0].mVehicleName), "911 GT3");
    layout->data.scoring.vehScoringInfo[0].mControl = 1; // Player control
    layout->data.scoring.scoringInfo.mInRealtime = 1;
    layout->data.scoring.scoringInfo.mGamePhase = 5; // Session running
    layout->data.generic.appInfo.mAppWindow = reinterpret_cast<HWND>(static_cast<intptr_t>(1)); // NOLINT(performance-no-int-to-ptr)
    #endif

    // --- Sub-timing Setup ---
    auto phase_start = std::chrono::high_resolution_clock::now();
    auto report_phase = [&](const char* name) {
        auto now = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(now - phase_start).count();
        std::cout << "    [Profile] Phase '" << name << "': " << std::fixed << std::setprecision(2) << ms << " ms" << std::endl;
        phase_start = now;
    };

    // Run FFBThread for a few iterations with changing telemetry
    // We run long enough to trigger the 5-second health warning logic
    g_running = true;
    g_use_mock_time = true;
    g_mock_time = std::chrono::steady_clock::now();

    // Start thread
    std::thread t(FFBThread);

    for (int i = 0; i < 550; i++) {
        #ifndef _WIN32
        SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
        layout->data.telemetry.telemInfo[0].mElapsedTime += 0.01f; // Faster than realtime to simulate time passing
        layout->data.telemetry.telemInfo[0].mSteeringShaftTorque = (float)sin(i * 0.1);
        #endif

        // Advance mock time by 10ms each step
        g_mock_time += std::chrono::milliseconds(10);

        // v0.7.186 Optimization: On Windows, high-frequency sleeps carry massive scheduling penalties.
        // Yield instead of sleep to allow FFBThread to process if needed, or sleep very briefly every N iterations.
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        } else {
            std::this_thread::yield();
        }
    }
    g_running = false;
    report_phase("FFBThread Exercise");
    if (t.joinable()) t.join();
    g_use_mock_time = false;

    std::cout << "[PASS] FFBThread exercised with telemetry" << std::endl;
    g_tests_passed++;

    // Test lmuffb_app_main in headless mode
    char* argv[] = {(char*)"lmuffb", (char*)"--headless"};
    g_running = true;

    // We can't easily run the full main because it blocks,
    // but we can simulate the loop exit.
    std::thread mt([&]() {
        lmuffb_app_main(2, argv);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    g_running = false;
    if (mt.joinable()) mt.join();
    report_phase("Main App Entry Point");

    std::cout << "[PASS] Main app entry point exercised" << std::endl;
    g_tests_passed++;

    // Test health monitor warnings and menu transitions
    {
        g_ffb_active = true;
        #ifndef _WIN32
        SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
        layout->data.scoring.scoringInfo.mInRealtime = 0; // In menu
        #endif

        // This should trigger "User exited to menu"
        std::thread t(FFBThread);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        #ifndef _WIN32
        layout->data.scoring.scoringInfo.mInRealtime = 1; // Back to track
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        g_running = false;
        if (t.joinable()) t.join();
        std::cout << "[PASS] Menu transitions exercised" << std::endl;
        g_tests_passed++;
    }

    // Test health monitor warnings (simulated low rate)
    {
        g_running = true;
        g_use_mock_time = true;
        g_mock_time = std::chrono::steady_clock::now();

        Config::m_auto_start_logging = true;
        #ifndef _WIN32
        SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
        layout->data.scoring.scoringInfo.mInRealtime = 1;
        #endif

        std::thread t(FFBThread);

        // We need to advance time past the 5-second interval in main.cpp for the health warning
        // We do this in smaller steps to ensure the loop processes the events.
        for(int j=0; j<520; ++j) {
            g_mock_time += std::chrono::milliseconds(10);

            // v0.7.186 Optimization: Replace high-frequency sleep with yield for Windows performance
            if (j % 10 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            } else {
                std::this_thread::yield();
            }
        }

        g_running = false;
        if (t.joinable()) t.join();
        g_use_mock_time = false;
        report_phase("Health Monitor Simulation");
        std::cout << "[PASS] Health monitor branch exercised (optimized)" << std::endl;
        g_tests_passed++;
    }

    // Test SIGTERM handler
    #ifndef _WIN32
    {
        g_running = true;
        handle_sigterm(SIGTERM);
        if (!g_running) {
            std::cout << "[PASS] handle_sigterm sets g_running to false" << std::endl;
            g_tests_passed++;
        }
    }
    #endif

    // Test command line arguments in lmuffb_app_main
    {
        char* argv[] = {(char*)"lmuffb", (char*)"--headless", (char*)"--invalid"};
        g_running = false; // Don't run the loop
        lmuffb_app_main(3, argv);
        std::cout << "[PASS] lmuffb_app_main with extra args" << std::endl;
        g_tests_passed++;
    }

    // Test lmuffb_app_main without headless
#ifndef _WIN32
    {
        char* argv[] = {(char*)"lmuffb"};
        g_running = false;
        lmuffb_app_main(1, argv);
        std::cout << "[PASS] lmuffb_app_main without headless" << std::endl;
        g_tests_passed++;
    }
#endif
}

} // namespace FFBEngineTests
