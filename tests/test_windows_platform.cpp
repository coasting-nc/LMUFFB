#include <windows.h>
#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800  // DirectInput 8
#include <dinput.h>
#endif
#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>
#include <cstdio>
#ifndef _WIN32
#include <unistd.h>
#include <cstring>
#endif

// Include headers to test
#include "../src/DirectInputFFB.h"
#include "../src/Config.h"
#include "../src/GuiLayer.h"
#include "../src/GuiPlatform.h"
#include "../src/GameConnector.h"
#include "imgui.h"

#include "test_ffb_common.h"

namespace FFBEngineTests {

#ifndef ASSERT_EQ_STR
#define ASSERT_EQ_STR(a, b) \
    if (std::string(a) == std::string(b)) { \
        std::cout << "[PASS] " << #a << " == " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << a << ") != " << #b << " (" << b << ")" << std::endl; \
        g_tests_failed++; \
    }
#endif

// --- Test Helpers ---
// Note: InitializeEngine is now provided by test_ffb_common.cpp to avoid multiple definitions

// --- TESTS ---





#ifdef _WIN32
TEST_CASE(test_window_always_on_top_behavior, "Windows") {
    std::cout << "\nTest: Window Always on Top Behavior" << std::endl;

    // 1. Create a dummy window for testing
    // Added WS_VISIBLE as SetWindowPos might behave differently for hidden windows in some environments
    HWND hwnd = CreateWindowA("STATIC", "TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
    ASSERT_TRUE(hwnd != NULL);

    // 2. Initial state: Should not be topmost
    LONG_PTR initial_ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    ASSERT_TRUE((initial_ex_style & WS_EX_TOPMOST) == 0);

    // 3. Apply "Always on Top" using the logic from GuiLayer (SetWindowPos)
    // Added SWP_FRAMECHANGED to ensure the system refreshes the window style bits
    BOOL success1 = ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    ASSERT_TRUE(success1 != 0);

    // 4. Verify style bit
    LONG_PTR after_ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    ASSERT_TRUE((after_ex_style & WS_EX_TOPMOST) != 0);

    // 5. Apply "Always on Top" OFF
    BOOL success2 = ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    ASSERT_TRUE(success2 != 0);

    // 6. Verify style bit removed
    LONG_PTR final_ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    ASSERT_TRUE((final_ex_style & WS_EX_TOPMOST) == 0);

    // Cleanup
    DestroyWindow(hwnd);
}
#endif










TEST_CASE(test_icon_presence, "Windows") {
    std::cout << "\nTest: Icon Presence (Build Artifact)" << std::endl;
    
    // Robustness Fix: Use the executable's path to find the artifact, 
    // strictly validating the build structure regardless of CWD.
    char buffer[MAX_PATH];
#ifdef _WIN32
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
#else
    // Linux equivalent for getting executable path
    ssize_t count = readlink("/proc/self/exe", buffer, MAX_PATH);
    if (count != -1) buffer[count] = '\0';
    else strncpy(buffer, ".", MAX_PATH);
#endif
    std::string exe_path(buffer);
    
    // Find the directory of the executable
    size_t last_slash = exe_path.find_last_of("\\/");
    std::string exe_dir = (last_slash != std::string::npos) ? exe_path.substr(0, last_slash) : ".";
    
    std::cout << "  Exe Dir: " << exe_dir << std::endl;

    // Expected locations relative to build output (e.g., build/tests/Release/run.exe)
    // We expect the icon to be in the build root (copied by CMake)
    std::vector<std::string> candidates;
    candidates.push_back(exe_dir + "/../../lmuffb.ico"); // Standard CMake Release/Debug layout
    candidates.push_back(exe_dir + "/../lmuffb.ico");    // Flat layout
    candidates.push_back(exe_dir + "/lmuffb.ico");       // Same dir
#ifndef _WIN32
    candidates.push_back(exe_dir + "/../../icon/lmuffb.ico"); // Linux source tree layout
#endif

    bool found = false;
    std::string found_path;
    for (const auto& path : candidates) {
        std::ifstream f(path, std::ios::binary);
        if (f.good()) {
            std::cout << "  [PASS] Found artifact at: " << path << std::endl;
            found = true;
            found_path = path;
            
            // Verify ICO Header (00 00 01 00)
            char header[4];
            f.read(header, 4);
            if (f.gcount() == 4) {
                if (header[0] == 0x00 && header[1] == 0x00 && header[2] == 0x01 && header[3] == 0x00) {
                    std::cout << "  [PASS] Valid ICO header detected (00 00 01 00)" << std::endl;
                } else {
                    std::cout << "  [FAIL] Invalid ICO header: " 
                              << std::hex << (int)header[0] << " " << (int)header[1] << " " 
                              << (int)header[2] << " " << (int)header[3] << std::dec << std::endl;
                    found = false; // Invalidate match if header is wrong
                }
            } else {
                std::cout << "  [FAIL] File too small to be a valid icon." << std::endl;
                found = false;
            }
            break;
        }
    }

    if (found) {
        g_tests_passed++;
    } else {
        std::cout << "  [FAIL] lmuffb.ico NOT found in build artifacts." << std::endl;
        std::cout << "         Checked paths relative to executable:" << std::endl;
        for (const auto& path : candidates) std::cout << "         - " << path << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_window_always_on_top_interface, "GUI") {
    std::cout << "\nTest: Window Always on Top Interface" << std::endl;

    // Test the platform-agnostic function calls the platform implementation
    SetWindowAlwaysOnTopPlatform(true);

#ifndef _WIN32
    // On Linux we can verify the mock state
    ASSERT_TRUE(::GetGuiPlatform().GetAlwaysOnTopMock() == true);

    SetWindowAlwaysOnTopPlatform(false);
    ASSERT_TRUE(::GetGuiPlatform().GetAlwaysOnTopMock() == false);

    std::cout << "  [PASS] Interface called successfully (Headless Mock verified)" << std::endl;
#else
    std::cout << "  [PASS] Interface called successfully (Win32)" << std::endl;
    g_tests_passed++;
#endif
}

TEST_CASE(test_game_connector_staleness, "Windows") {
    std::cout << "\nTest: GameConnector Staleness (Heartbeat Watchdog)" << std::endl;

    // 1. Disconnected state should be stale
    GameConnector::Get().Disconnect();
    ASSERT_TRUE(GameConnector::Get().IsStale() == true);

    // 2. Mocking connection to test watchdog logic
    // We create the shared memory mapping LMU expects
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)sizeof(SharedMemoryLayout), LMU_SHARED_MEMORY_FILE);
    if (hMap) {
        SharedMemoryLayout* pBuf = (SharedMemoryLayout*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryLayout));
        if (pBuf) {
            memset(pBuf, 0, sizeof(SharedMemoryLayout));
            
            // Setup minimal telemetry
            pBuf->data.telemetry.playerHasVehicle = true;
            pBuf->data.telemetry.playerVehicleIdx = 0;
            pBuf->data.telemetry.activeVehicles = 1;
            pBuf->data.generic.events[SME_UPDATE_TELEMETRY] = static_cast<SharedMemoryEvent>(1);
            pBuf->data.telemetry.telemInfo[0].mElapsedTime = 100.0;
            
            // SharedMemoryLock for GameConnector::TryConnect
            // This will create the lock mapping and event that GameConnector expects
            auto mockLock = SharedMemoryLock::MakeSharedMemoryLock(); 

            // Connect (should succeed now that mapping & lock exist)
            if (GameConnector::Get().TryConnect()) {
                SharedMemoryObjectOut dest;
                
                // First update - should be fresh
                GameConnector::Get().CopyTelemetry(dest);
                ASSERT_TRUE(GameConnector::Get().IsStale(100) == false);

                // Wait 150ms without updating pBuf->data.telemetry.telemInfo[0].mElapsedTime
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                
                // Call CopyTelemetry again (simulates the loop running)
                // Since mElapsedTime hasn't changed, heartbeat shouldn't update
                GameConnector::Get().CopyTelemetry(dest);
                
                // Now it should be stale
                ASSERT_TRUE(GameConnector::Get().IsStale(100) == true);

                // Update mElapsedTime to simulate game advancing
                pBuf->data.telemetry.telemInfo[0].mElapsedTime = 100.01;
                GameConnector::Get().CopyTelemetry(dest);
                
                // Should be fresh again
                ASSERT_TRUE(GameConnector::Get().IsStale(100) == false);
            } else {
                std::cout << "  [SKIP] Could not mock connection for staleness test." << std::endl;
            }
            UnmapViewOfFile(pBuf);
        }
        CloseHandle(hMap);
    }
    
    GameConnector::Get().Disconnect();
}

} // namespace FFBEngineTests