#include "test_ffb_common.h"
#include "../src/io/GameConnector.h"
#include <thread>
#include <chrono>

#ifndef _WIN32
#include "../src/io/lmu_sm_interface/LinuxMock.h"
#endif

namespace LMUFFB {
extern std::atomic<bool> g_running;
}

namespace FFBEngineTests {

/**
 * Isolated test case for LMUFFB::IO::GameConnector state machine and shared memory locking.
 * This test is sensitive to global mock state and background thread contention, 
 * so it is intentionally excluded from the large unity build chunk to run in its own process.
 */
TEST_CASE(test_game_connector_branch_boost_isolated, "System") {
    // 1. Force background threads to stop to avoid MockSM lock contention in unity build context
    LMUFFB::g_running = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    LMUFFB::IO::GameConnector& conn = LMUFFB::IO::GameConnector::Get();
    conn.Disconnect();

    #ifndef _WIN32
    MockSM::ResetAll(); // Full reset for clean state
    
    MockSM::GetMaps()["LMU_Data"].resize(sizeof(SharedMemoryLayout));
    SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
    memset(layout, 0, sizeof(SharedMemoryLayout)); // Zero init
    
    layout->data.generic.events[SME_UPDATE_TELEMETRY] = static_cast<SharedMemoryEvent>(1); // Enable telemetry copying
    layout->data.generic.events[SME_UPDATE_SCORING] = static_cast<SharedMemoryEvent>(1); // Enable scoring copying
    layout->data.generic.appInfo.mAppWindow = reinterpret_cast<HWND>(static_cast<intptr_t>(3)); // Invalid window
    layout->data.scoring.scoringInfo.mInRealtime = true; // REQUIRED for CopyTelemetry to return true
    layout->data.scoring.scoringInfo.mGamePhase = 5; // Running
    strncpy(layout->data.scoring.scoringInfo.mTrackName, "Monza", 63);

    conn.TryConnect();
    ASSERT_FALSE(conn.IsConnected());

    layout->data.generic.appInfo.mAppWindow = reinterpret_cast<HWND>(static_cast<intptr_t>(1)); // Valid window
    conn.TryConnect();
    ASSERT_TRUE(conn.IsConnected());
    
    layout->data.telemetry.playerHasVehicle = false;
    SharedMemoryObjectOut dest {};

    bool copy_res = conn.CopyTelemetry(dest);
    
    // Create detailed diagnostic message for the failure log
    std::stringstream diag;
    diag << "Conn: " << (conn.IsConnected() ? "YES" : "NO");
    diag << " | Realtime: " << (layout->data.scoring.scoringInfo.mInRealtime ? "T" : "F");

    auto& maps = MockSM::GetMaps();
    if (maps.count("LMU_SharedMemoryLockData")) {
        long* ldata = (long*)maps["LMU_SharedMemoryLockData"].data();
        diag << " | Lock: waiters=" << ldata[0] << " busy=" << ldata[1];
    }
    diag << " | Err: " << MockSM::LastError() << " | Wait: " << MockSM::WaitResult();

    ASSERT_TRUE_MSG(copy_res, diag.str());
    ASSERT_FALSE(dest.telemetry.playerHasVehicle);
    #endif
}

} // namespace FFBEngineTests
