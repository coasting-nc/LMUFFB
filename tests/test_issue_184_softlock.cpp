#include "test_ffb_common.h"
#include <atomic>
#include <mutex>
#include <thread>
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "../src/GameConnector.h"

extern std::atomic<bool> g_ffb_active;

namespace FFBEngineTests {

void test_issue_184_softlock_stationary() {
    std::cout << "\nTest: Issue #184 Soft Lock in Menus Repro" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Setup Soft Lock to be strong
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 20.0f;
    engine.m_wheelbase_max_nm = 100.0f;
    engine.m_target_rim_nm = 100.0f;
    engine.m_gain = 1.0f;

    // Normalization setup
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);

    // Mock Telemetry: Steering beyond lock (1.1)
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0, 0.0);
    data.mUnfilteredSteering = 1.1;

    // Simulation of current main.cpp logic (v0.7.117)
    // When in_realtime is false (e.g. Esc menu)
    {
        bool in_realtime = false;
        bool full_allowed = false; // IsFFBAllowed && in_realtime
        double force = 0.0;

        // This represents the call in main.cpp:
        force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed, 0.0025);

        // BUG: current main.cpp has this line which kills Soft Lock in menus:
        if (!in_realtime) force = 0.0;

        std::cout << "  Force with in_realtime=false (Current BUG): " << force << std::endl;
        ASSERT_EQ(force, 0.0); // Verifies the bug exists: no force in menus
    }

    // After fix, the logic should be:
    {
        bool in_realtime = false;
        bool full_allowed = false;
        double force = 0.0;

        // Proposed fix: remove the 'if (!in_realtime) force = 0.0;' in main.cpp
        force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed, 0.0025);

        // No more 'if (!in_realtime) force = 0.0;' here!

        std::cout << "  Force with in_realtime=false (Post-Fix Expectation): " << force << std::endl;
        // Should be non-zero and negative (pushing back from 1.1)
        ASSERT_LT(force, -0.01);
    }
}

void test_issue_184_gameconnector_heartbeat() {
    std::cout << "\nTest: Issue #184 GameConnector Heartbeat" << std::endl;

    bool old_ffb_active = g_ffb_active.load();
    g_ffb_active = false; // Prevent background thread interference

    GameConnector& conn = GameConnector::Get();
    conn.Disconnect(); // Start clean

    // 1. Setup shared memory mock state
    SharedMemoryObjectOut mockSM;
    std::memset(&mockSM, 0, sizeof(mockSM));

    // Crucial for CopySharedMemoryObj
    mockSM.generic.events[SME_UPDATE_TELEMETRY] = SME_UPDATE_TELEMETRY;
    // Leave mAppWindow as NULL to avoid IsWindow() check on Windows
    mockSM.generic.appInfo.mAppWindow = NULL;

    mockSM.telemetry.activeVehicles = 1;
    mockSM.telemetry.playerHasVehicle = true;
    mockSM.telemetry.playerVehicleIdx = 0;
    mockSM.telemetry.telemInfo[0].mElapsedTime = 100.0;
    mockSM.telemetry.telemInfo[0].mUnfilteredSteering = 0.5;

    // Create/Update the mock shared memory file
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemoryLayout), LMU_SHARED_MEMORY_FILE);
    SharedMemoryLayout* pBuf = (SharedMemoryLayout*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryLayout));
    if (pBuf) pBuf->data = mockSM;

    // Connect
    ASSERT_TRUE(conn.TryConnect());

    // Initial Copy - should refresh heartbeat
    conn.CopyTelemetry(mockSM);

    // Check not stale (use 500ms for safety in CI)
    ASSERT_FALSE(conn.IsStale(500));

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    ASSERT_TRUE(conn.IsStale(500));

    // 2. Test input-only heartbeat (Time frozen, steering changed)
    mockSM.telemetry.telemInfo[0].mElapsedTime = 100.0; // Same time
    mockSM.telemetry.telemInfo[0].mUnfilteredSteering = 0.6; // New steering

    // Update the ACTUAL shared memory buffer that GameConnector reads from
    if (pBuf) pBuf->data = mockSM;

    conn.CopyTelemetry(mockSM);
    ASSERT_FALSE(conn.IsStale(500)); // Heartbeat should have refreshed the timer

    // 3. Cleanup
    if (pBuf) UnmapViewOfFile(pBuf);
    if (hMap) CloseHandle(hMap);
    conn.Disconnect();

    g_ffb_active = old_ffb_active;
}

AutoRegister reg_issue_184_softlock("Issue #184 Soft Lock Repro", "Issue184", {"Physics", "Issue184"}, test_issue_184_softlock_stationary);
AutoRegister reg_issue_184_heartbeat("Issue #184 Heartbeat Test", "Issue184", {"Logic", "Issue184"}, test_issue_184_gameconnector_heartbeat);

} // namespace FFBEngineTests
