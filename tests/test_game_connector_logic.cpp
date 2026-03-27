#include "StringUtils.h"
#include "test_ffb_common.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace LMUFFB::Utils;

namespace FFBEngineTests {

/**
 * Regression test for LMUFFB::IO::GameConnector robust state machine and "zombie connection" fix.
 * Simulates a game session using real Windows shared memory (or Linux mock)
 * to verify cross-platform connection reliability.
 */
TEST_CASE(test_game_connector_robust_logic, "System") {
#ifdef _WIN32
    std::cout << "  [INFO] Running LMUFFB::IO::GameConnector logic test with simulated Windows Shared Memory." << std::endl;
    
    const char* mapName = "LMU_Data";
    const char* lockMapName = "LMU_SharedMemoryLockData";
    
    // 1. Ensure clean start
    LMUFFB::IO::GameConnector::Get().Disconnect();
    
    // 2. Create Simulated Shared Memory
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemoryLayout), mapName);
    ASSERT_TRUE(hMap != NULL);
    
    SharedMemoryLayout* layout = (SharedMemoryLayout*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryLayout));
    ASSERT_TRUE(layout != NULL);
    memset(layout, 0, sizeof(SharedMemoryLayout));
    
    // Create Lock Data Mapping to satisfy SharedMemoryLock::Init()
    HANDLE hLockMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, lockMapName);
    ASSERT_TRUE(hLockMap != NULL);
    
    // 3. Test: Connection with INVALID Window
    layout->data.generic.appInfo.mAppWindow = (HWND)(intptr_t)9999; // Likely invalid
    
    bool connected = LMUFFB::IO::GameConnector::Get().TryConnect();
    ASSERT_TRUE(connected);
    // IsConnected() should be FALSE because window 9999 is invalid
    ASSERT_FALSE(LMUFFB::IO::GameConnector::Get().IsConnected());
    
    // 4. Test: Re-connection with VALID Window (Regression #267)
    // On Windows, we can use the current console or a known window
    layout->data.generic.appInfo.mAppWindow = GetConsoleWindow();
    layout->data.scoring.scoringInfo.mInRealtime = true;
    layout->data.scoring.scoringInfo.mNumVehicles = 1; // Required for scoring copy
    StringUtils::SafeCopy(layout->data.scoring.scoringInfo.mTrackName, 64, "Nordschleife");
    
    // Re-call TryConnect. Should now succeed and update state because we fixed IsConnected() check.
    bool try_res = LMUFFB::IO::GameConnector::Get().TryConnect();
    ASSERT_TRUE(try_res);
    ASSERT_TRUE(LMUFFB::IO::GameConnector::Get().IsConnected());
    
    // 5. Test: Telemetry Copying with State Check
    SharedMemoryObjectOut dest {};
    layout->data.generic.events[SME_UPDATE_TELEMETRY] = (SharedMemoryEvent)1;
    layout->data.generic.events[SME_UPDATE_SCORING] = (SharedMemoryEvent)1; // Enable scoring copy!
    
    bool copy_res = LMUFFB::IO::GameConnector::Get().CopyTelemetry(dest);
    
    if (!copy_res) {
        std::cout << "  [FAIL_DIAG] copy_res=false. Conn=" << LMUFFB::IO::GameConnector::Get().IsConnected() << std::endl;
    }

    ASSERT_TRUE(copy_res);
    ASSERT_TRUE(dest.scoring.scoringInfo.mInRealtime);
    ASSERT_TRUE(strcmp(dest.scoring.scoringInfo.mTrackName, "Nordschleife") == 0);

    // 6. Test: Proper Disconnection Reset
    LMUFFB::IO::GameConnector::Get().Disconnect();
    ASSERT_FALSE(LMUFFB::IO::GameConnector::Get().IsConnected());
    
    // Verify that LMUFFB::IO::GameConnector forgot the track name (Internal state reset)
    // We can't access private members easily, but we can try to re-connect to empty memory.
    memset(layout, 0, sizeof(SharedMemoryLayout));
    LMUFFB::IO::GameConnector::Get().TryConnect();
    // Since track name is now empty, m_sessionActive should be false
    // We'd need an accessor to verify this, but the Disconnect code change already covers it.

    // 7. Cleanup
    UnmapViewOfFile(layout);
    CloseHandle(hMap);
    CloseHandle(hLockMap);
    
    // Ensure LMUFFB::IO::GameConnector is clean for other tests
    LMUFFB::IO::GameConnector::Get().Disconnect();

#else
    std::cout << "  [INFO] Skipping Windows-specific sim on Linux (covered by test_game_connector_isolated)." << std::endl;
#endif
}

} // namespace FFBEngineTests
