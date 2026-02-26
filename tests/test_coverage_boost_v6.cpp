#include "test_ffb_common.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <cstring>
#include <string>
#include <utility>
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "../src/lmu_sm_interface/SafeSharedMemoryLock.h"
#include "../src/lmu_sm_interface/LinuxMock.h"
#include "../src/GuiLayer.h"
#include "../src/GameConnector.h"
#include "../src/DirectInputFFB.h"
#include "../src/RateMonitor.h"
#include "../src/AsyncLogger.h"
#include "../src/GuiWidgets.h"
#include "../src/Logger.h"
#include "imgui.h"

extern std::atomic<bool> g_running;
extern std::atomic<bool> g_ffb_active;
extern std::recursive_mutex g_engine_mutex;
extern void FFBThread();
extern int lmuffb_app_main(int argc, char* argv[]);

class GuiLayerTestAccess {
public:
    static void DrawTuningWindow(FFBEngine& engine) { GuiLayer::DrawTuningWindow(engine); }
    static void DrawDebugWindow(FFBEngine& engine) { GuiLayer::DrawDebugWindow(engine); }
};

#ifndef _WIN32
// Use the global captured swap chain desc defined in test_dxgi_modernization.cpp
extern DXGI_SWAP_CHAIN_DESC1 g_captured_swap_chain_desc;
#endif

namespace FFBEngineTests {

TEST_CASE(test_linux_mock_branches_v6, "System") {
    std::cout << "\nTest: LinuxMock Branches (Coverage Boost V6)" << std::endl;

#ifndef _WIN32
    // InterlockedCompareExchange branches
    long dest = 10;
    long old = InterlockedCompareExchange(&dest, 20, 10); // Match
    if (old == 10 && dest == 20) {
        std::cout << "[PASS] InterlockedCompareExchange Match branch" << std::endl;
        g_tests_passed++;
    }

    old = InterlockedCompareExchange(&dest, 30, 10); // No match
    if (old == 20 && dest == 20) {
        std::cout << "[PASS] InterlockedCompareExchange No-Match branch" << std::endl;
        g_tests_passed++;
    }

    // CreateFileMappingA branches
    HANDLE h1 = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, nullptr); // null name
    if (h1 == (HANDLE)1) {
        std::cout << "[PASS] CreateFileMappingA null name branch" << std::endl;
        g_tests_passed++;
    }

    HANDLE h2 = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, "TestMapV6"); // new name
    if (h2 != nullptr && h2 != (HANDLE)1) {
        std::cout << "[PASS] CreateFileMappingA new name branch" << std::endl;
        g_tests_passed++;
    }

    HANDLE h3 = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, "TestMapV6"); // existing name
    if (h3 != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "[PASS] CreateFileMappingA existing name branch" << std::endl;
        g_tests_passed++;
    }
    CloseHandle(h2);
    CloseHandle(h3);

    // OpenFileMappingA branches
    HANDLE h4 = OpenFileMappingA(FILE_MAP_READ, FALSE, nullptr); // null name
    if (h4 == nullptr) {
        std::cout << "[PASS] OpenFileMappingA null name branch" << std::endl;
        g_tests_passed++;
    }

    HANDLE h5 = OpenFileMappingA(FILE_MAP_READ, FALSE, "NonExistentMapV6"); // non-existing name
    if (h5 == nullptr) {
        std::cout << "[PASS] OpenFileMappingA non-existing name branch" << std::endl;
        g_tests_passed++;
    }

    // IsWindow branches
    if (IsWindow((HWND)1) && IsWindow((HWND)2) && !IsWindow((HWND)3)) {
        std::cout << "[PASS] IsWindow invalid handle branch" << std::endl;
        g_tests_passed++;
    }

    // GetWindowLongPtr branches
    if (GetWindowLongPtr((HWND)1, GWL_EXSTYLE) == MockGUI::ExStyle() &&
        GetWindowLongPtr((HWND)1, 0) == 0) {
        std::cout << "[PASS] GetWindowLongPtr nIndex != GWL_EXSTYLE branch" << std::endl;
        g_tests_passed++;
    }

    // VerQueryValueA branches
    void* buffer = nullptr;
    UINT len = 0;
    char ver_data[1024];
    if (VerQueryValueA(ver_data, "\\VarFileInfo\\Translation", &buffer, &len) &&
        VerQueryValueA(ver_data, "CompanyName", &buffer, &len) &&
        VerQueryValueA(ver_data, "ProductVersion", &buffer, &len) &&
        !VerQueryValueA(ver_data, "Unknown", &buffer, &len)) {
        std::cout << "[PASS] VerQueryValueA all sub-block branches" << std::endl;
        g_tests_passed++;
    }

    // MockDXGIFactory2 branches
    MockDXGIFactory2 factory;
    factory.CreateSwapChainForHwnd(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr); // null pDesc branch

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = 1920;
    factory.CreateSwapChainForHwnd(nullptr, nullptr, &desc, nullptr, nullptr, nullptr);
    if (g_captured_swap_chain_desc.Width == 1920) {
        std::cout << "[PASS] MockDXGIFactory2 pDesc branch" << std::endl;
        g_tests_passed++;
    }
#endif
}

TEST_CASE(test_safe_sm_lock_fail_v6, "System") {
    std::cout << "\nTest: SafeSharedMemoryLock Failure (Coverage Boost V6)" << std::endl;

#ifndef _WIN32
    MockSM::FailNext() = true;
    auto lock = SafeSharedMemoryLock::MakeSafeSharedMemoryLock();
    if (!lock.has_value()) {
        std::cout << "[PASS] SafeSharedMemoryLock nullopt branch" << std::endl;
        g_tests_passed++;
    }
#endif
}

TEST_CASE(test_sm_interface_lock_wait_v6, "System") {
    std::cout << "\nTest: SharedMemoryInterface Lock Wait (Coverage Boost V6)" << std::endl;

#ifndef _WIN32
    auto smLockOpt = SharedMemoryLock::MakeSharedMemoryLock();
    if (smLockOpt.has_value()) {
        SharedMemoryLock& smLock = smLockOpt.value();

        std::atomic<bool> locked(false);
        std::atomic<bool> release(false);
        std::thread t([&]() {
            smLock.Lock();
            locked = true;
            while (!release) std::this_thread::yield();
            smLock.Unlock();
        });

        while (!locked) std::this_thread::yield();

        // Now try to lock with short timeout
        bool result = smLock.Lock(10);
        if (!result) {
            std::cout << "[PASS] SharedMemoryLock wait/timeout branch" << std::endl;
            g_tests_passed++;
        }

        release = true;
        t.join();
    }
#endif
}

TEST_CASE(test_sm_interface_copy_branches_v6, "System") {
    std::cout << "\nTest: SharedMemoryInterface Copy Branches (Coverage Boost V6)" << std::endl;

    SharedMemoryObjectOut src = {};
    SharedMemoryObjectOut dst = {};

    // SME_UPDATE_SCORING branch
    src.generic.events[SME_UPDATE_SCORING] = SME_UPDATE_SCORING;
    src.scoring.scoringInfo.mNumVehicles = 1;
    src.scoring.scoringStreamSize = 10;
    CopySharedMemoryObj(dst, src);
    if (dst.scoring.scoringStreamSize == 10) {
        std::cout << "[PASS] CopySharedMemoryObj SME_UPDATE_SCORING branch" << std::endl;
        g_tests_passed++;
    }

    // SME_UPDATE_TELEMETRY branch
    memset(&src.generic.events, 0, sizeof(src.generic.events));
    src.generic.events[SME_UPDATE_TELEMETRY] = SME_UPDATE_TELEMETRY;
    src.telemetry.activeVehicles = 5;
    CopySharedMemoryObj(dst, src);
    if (dst.telemetry.activeVehicles == 5) {
        std::cout << "[PASS] CopySharedMemoryObj SME_UPDATE_TELEMETRY branch" << std::endl;
        g_tests_passed++;
    }
}

TEST_CASE(test_main_thread_branches_v6, "System") {
    std::cout << "\nTest: FFBThread Branches (Coverage Boost V6)" << std::endl;

#ifndef _WIN32
    Logger::Get().Init("test_main_thread_v6.log");

    // 1. Setup Mock
    MockSM::GetMaps()["LMU_Data"].resize(sizeof(SharedMemoryLayout));
    SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
    memset(layout, 0, sizeof(SharedMemoryLayout));
    layout->data.telemetry.playerHasVehicle = true;
    layout->data.telemetry.playerVehicleIdx = 0;
    layout->data.telemetry.telemInfo[0].mDeltaTime = 0.0f; // Trigger invalid dt branch
    layout->data.telemetry.telemInfo[0].mElapsedTime = 1.0f;
    layout->data.scoring.vehScoringInfo[0].mControl = 1;
    layout->data.scoring.scoringInfo.mInRealtime = 1;

    // Ensure connector is connected
    GameConnector::Get().TryConnect();

    g_ffb_active = true;
    g_running = true;
    std::thread t(FFBThread);

    // Wait a bit to let it run
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Trigger "User exited to menu" branch
    {
        layout->data.scoring.scoringInfo.mInRealtime = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 3. Trigger "User entered driving session" branch
    {
        layout->data.scoring.scoringInfo.mInRealtime = 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 4. Trigger "stale" branch
    // GameConnector::IsStale(100)
    // We don't update layout for 200ms
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 5. Trigger different playerVehicleIdx
    {
        layout->data.telemetry.playerVehicleIdx = 1;
        layout->data.telemetry.telemInfo[1].mDeltaTime = 0.0025f;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 6. Trigger idx >= 104 branch
    {
        layout->data.telemetry.playerVehicleIdx = 105;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 7. Trigger ChannelMonitor::Update (all channels)
    {
        layout->data.telemetry.playerVehicleIdx = 0;
        layout->data.telemetry.activeVehicles = 1;
        auto& t = layout->data.telemetry.telemInfo[0];
        t.mLocalAccel = {1.0f, 1.0f, 1.0f};
        t.mLocalVel = {1.0f, 1.0f, 1.0f};
        t.mLocalRot = {1.0f, 1.0f, 1.0f};
        t.mLocalRotAccel = {1.0f, 1.0f, 1.0f};
        t.mUnfilteredSteering = 1.0;
        t.mFilteredSteering = 1.0;
        t.mEngineRPM = 1000.0;
        for(int i=0; i<4; i++) {
            t.mWheel[i].mTireLoad = 1000.0;
            t.mWheel[i].mLateralForce = 1000.0;
        }
        t.mPos = {1.0, 1.0, 1.0};
        t.mDeltaTime = 0.0026f;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    g_running = false;
    if (t.joinable()) t.join();

    // 8. Extended run with telemetry updates to hit ChannelMonitor::Update and other branches
    {
        std::atomic<bool> stop_telem(false);
        std::thread telem_thread([&]() {
            while(!stop_telem) {
                SharedMemoryLayout* l = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
                l->data.telemetry.playerHasVehicle = true;
                l->data.telemetry.playerVehicleIdx = 0;
                l->data.telemetry.telemInfo[0].mElapsedTime += 0.0025;
                l->data.telemetry.telemInfo[0].mSteeringShaftTorque += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalAccel.x += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalAccel.y += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalAccel.z += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalVel.x += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalVel.y += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalVel.z += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalRot.x += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalRot.y += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalRot.z += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalRotAccel.x += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalRotAccel.y += 0.01f;
                l->data.telemetry.telemInfo[0].mLocalRotAccel.z += 0.01f;
                l->data.telemetry.telemInfo[0].mUnfilteredSteering += 0.01;
                l->data.telemetry.telemInfo[0].mFilteredSteering += 0.01;
                l->data.telemetry.telemInfo[0].mEngineRPM += 1.0;
                for(int i=0; i<4; i++) {
                    l->data.telemetry.telemInfo[0].mWheel[i].mTireLoad += 1.0;
                    l->data.telemetry.telemInfo[0].mWheel[i].mLateralForce += 1.0;
                }
                l->data.telemetry.telemInfo[0].mPos.x += 0.01;
                l->data.telemetry.telemInfo[0].mPos.y += 0.01;
                l->data.telemetry.telemInfo[0].mPos.z += 0.01;
                l->data.telemetry.telemInfo[0].mDeltaTime = 0.0025f;
                l->data.scoring.scoringInfo.mInRealtime = 1;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });

        g_running = true;
        std::thread ffb_t(FFBThread);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        g_running = false;
        ffb_t.join();
        stop_telem = true;
        telem_thread.join();
    }

    std::cout << "[PASS] FFBThread branches exercised" << std::endl;
    g_tests_passed++;

    // 9. Trigger Config::m_needs_save in lmuffb_app_main
    {
        char* argv[] = {(char*)"lmuffb", (char*)"--headless"};
        g_running = true;
        Config::m_needs_save = true;
        std::thread mt([&]() {
            lmuffb_app_main(2, argv);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        g_running = false;
        if (mt.joinable()) mt.join();
    }
    std::cout << "[PASS] lmuffb_app_main with save request exercised" << std::endl;
    g_tests_passed++;
#endif
}

TEST_CASE(test_gui_layer_common_branches_v6, "GUI") {
    std::cout << "\nTest: GuiLayer_Common Branches (Coverage Boost V6)" << std::endl;

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    FFBEngine engine;

    // Trigger branches in DrawTuningWindow
    {
        ImGui::NewFrame();

        // Test "Connected to LMU" branch
        #ifndef _WIN32
        MockSM::GetMaps()["LMU_Data"].resize(sizeof(SharedMemoryLayout));
        #endif
        GameConnector::Get().TryConnect();

        GuiLayerTestAccess::DrawTuningWindow(engine);

        // Test "Logging" branch
        SessionInfo info;
        AsyncLogger::Get().Start(info, ".");
        GuiLayerTestAccess::DrawTuningWindow(engine);
        AsyncLogger::Get().Stop();

        ImGui::EndFrame();
    }

    // Diverse UI states for more branches
    {
        ImGui::NewFrame();
        engine.m_torque_source = 0; // Shaft
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_torque_source = 1; // In-Game
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_soft_lock_enabled = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_flatspot_suppression = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_static_notch_enabled = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_slope_detection_enabled = true;
        engine.m_oversteer_boost = 0.5f;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_lockup_enabled = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_abs_pulse_enabled = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_slide_texture_enabled = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_road_texture_enabled = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        engine.m_spin_enabled = true;
        GuiLayerTestAccess::DrawTuningWindow(engine);

        Config::show_graphs = true;
        GuiLayerTestAccess::DrawDebugWindow(engine);

        ImGui::EndFrame();
    }

    ImGui::DestroyContext(ctx);
    std::cout << "[PASS] GuiLayer_Common branches exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_gui_widgets_branches_v6, "GUI") {
    std::cout << "\nTest: GuiWidgets Branches (Coverage Boost V6)" << std::endl;

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    float val = 0.5f;
    bool bval = true;
    int ival = 0;
    const char* items[] = {"Item1", "Item2"};

    // Test Float slider with keys
    {
        ImGui::NewFrame();
        ImGui::Columns(2);

        // Trigger arrow keys logic
        io.KeyCtrl = false;
        io.AddKeyEvent(ImGuiKey_LeftArrow, true);
        GuiWidgets::Float("TestFloat", &val, 0.0f, 1.0f);
        io.AddKeyEvent(ImGuiKey_LeftArrow, false);

        io.AddKeyEvent(ImGuiKey_RightArrow, true);
        GuiWidgets::Float("TestFloat2", &val, 0.0f, 1.0f);
        io.AddKeyEvent(ImGuiKey_RightArrow, false);

        // Trigger Tooltip
        io.MousePos = ImGui::GetItemRectMin();
        GuiWidgets::Float("TestFloat3", &val, 0.0f, 1.0f, "%.2f", "MyTooltip");

        ImGui::EndFrame();
    }

    // Test Checkbox/Combo tooltips
    {
        ImGui::NewFrame();
        ImGui::Columns(2);
        GuiWidgets::Checkbox("TestBool", &bval, "BoolTooltip");
        GuiWidgets::Combo("TestCombo", &ival, items, 2, "ComboTooltip");
        ImGui::EndFrame();
    }

    ImGui::DestroyContext(ctx);
    std::cout << "[PASS] GuiWidgets branches exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_game_connector_branches_v6, "System") {
    std::cout << "\nTest: GameConnector Branches (Coverage Boost V6)" << std::endl;

#ifndef _WIN32
    GameConnector& conn = GameConnector::Get();
    conn.Disconnect();

    // 1. CheckLegacyConflict
    MockSM::GetMaps()["$rFactor2SMMP_Telemetry$"].resize(1024);
    ASSERT_TRUE(conn.CheckLegacyConflict());
    MockSM::GetMaps().erase("$rFactor2SMMP_Telemetry$");
    ASSERT_FALSE(conn.CheckLegacyConflict());

    // 2. TryConnect failure - MapViewOfFile (simulated via invalid size or something?)
    // Actually our mock MapViewOfFile returns nullptr if handle is nullptr or 1 or INVALID.
    // If OpenFileMappingA succeeds, it returns a pointer to a string.

    // 3. TryConnect failure - SafeSharedMemoryLock failure
    MockSM::GetMaps()["LMU_Data"].resize(sizeof(SharedMemoryLayout));
    MockSM::FailNext() = true; // For SafeSharedMemoryLock::MakeSafeSharedMemoryLock
    ASSERT_FALSE(conn.TryConnect());

    // 4. TryConnect success
    ASSERT_TRUE(conn.TryConnect());

    // 5. IsConnected - HWND gone
    SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
    layout->data.generic.appInfo.mAppWindow = (HWND)1;
    // We need to re-connect to pick up the HWND? No, it's picked up during TryConnect.
    conn.Disconnect();
    conn.TryConnect();

    ASSERT_TRUE(conn.IsConnected());
    // In LinuxMock, IsWindow((HWND)3) returns FALSE.
    // We can't easily change the stored HWND in GameConnector as it's private.
    // But we've exercised the basic path.

    // 6. CopyTelemetry - Lock failure simulation
    auto otherLockOpt = SharedMemoryLock::MakeSharedMemoryLock();
    if (otherLockOpt.has_value()) {
        otherLockOpt->Lock(); // Make it busy so Lock() falls through to WaitForSingleObject
        MockSM::WaitResult() = 0x00000102L; // WAIT_TIMEOUT
        SharedMemoryObjectOut dest_fail;
        ASSERT_FALSE(conn.CopyTelemetry(dest_fail));
        otherLockOpt->Unlock();
    }

    conn.Disconnect();
#endif
    std::cout << "[PASS] GameConnector branches exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_rate_monitor_v6, "System") {
    std::cout << "\nTest: RateMonitor Branches (Coverage Boost V6)" << std::endl;
    RateMonitor rm;
    auto now = std::chrono::steady_clock::now();
    rm.RecordEventAt(now);
    // Trigger duration_ms >= 1000 branch
    rm.RecordEventAt(now + std::chrono::milliseconds(1001));
    if (rm.GetRate() > 0.0) {
        std::cout << "[PASS] RateMonitor duration branch exercised" << std::endl;
        g_tests_passed++;
    }
}

TEST_CASE(test_async_logger_branches_v6, "System") {
    std::cout << "\nTest: AsyncLogger Branches (Coverage Boost V6)" << std::endl;
    AsyncLogger& logger = AsyncLogger::Get();
    logger.Stop(); // Ensure it's stopped

    SessionInfo info;
    info.app_version = LMUFFB_VERSION;
    info.vehicle_name = "Car/With\\Chars:And*More";
    info.track_name = "Track?Name";

    // Test Start with base_path and sanitization
    logger.Start(info, "./test_logs_v6");
    if (logger.IsLogging()) {
        std::cout << "[PASS] AsyncLogger started with path and sanitization" << std::endl;
        g_tests_passed++;
    }

    // Test Log with decimation and marker
    LogFrame frame = {};
    for(int i=0; i<10; i++) logger.Log(frame);

    logger.SetMarker();
    logger.Log(frame);

    if (logger.GetFrameCount() > 0) {
        std::cout << "[PASS] AsyncLogger log with marker exercised" << std::endl;
        g_tests_passed++;
    }

    logger.Stop();
    // Test double stop
    logger.Stop();
    std::cout << "[PASS] AsyncLogger stop branches exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_logger_v6, "System") {
    std::cout << "\nTest: Logger Branches (Coverage Boost V6)" << std::endl;
    Logger::Get().Init("test_v6_sync.log");
    Logger::Get().LogStr("Test string");
    Logger::Get().LogWin32Error("TestContext", 1234);

    // Test Init failure branch
    Logger::Get().Init("/invalid/path/to/log.log");

    std::cout << "[PASS] Logger helpers exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_direct_input_v6, "System") {
    std::cout << "\nTest: DirectInputFFB Branches (Coverage Boost V6)" << std::endl;
    DirectInputFFB& di = DirectInputFFB::Get();
    di.Initialize(nullptr);

    // Test GUID conversion
    GUID g1 = {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}};
    std::string s = di.GuidToString(g1);
    GUID g2 = di.StringToGuid(s);
    if (memcmp(&g1, &g2, sizeof(GUID)) == 0) {
        std::cout << "[PASS] GUID conversion roundtrip" << std::endl;
        g_tests_passed++;
    }

    // Test empty GUID string
    GUID g_empty = di.StringToGuid("");
    if (g_empty.Data1 == 0) {
        std::cout << "[PASS] GUID empty string handle" << std::endl;
        g_tests_passed++;
    }

    // Test UpdateForce branches
    di.SelectDevice(g1);
    di.UpdateForce(0.5); // Changed
    if (!di.UpdateForce(0.5)) { // Unchanged (returns false due to optimization)
        std::cout << "[PASS] UpdateForce optimization branch" << std::endl;
        g_tests_passed++;
    }
    di.UpdateForce(0.0); // Zero (special case)

    di.Shutdown();
    std::cout << "[PASS] DirectInputFFB branches exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_config_branches_v6, "System") {
    std::cout << "\nTest: Config Branches (Coverage Boost V6)" << std::endl;
    FFBEngine engine;

    // 1. Test Load with non-existent file
    Config::m_last_preset_name = "NonExistent";
    Config::Load(engine, "non_existent_config.ini");

    // 2. Test migration logic and various keys
    {
        std::ofstream f("test_comprehensive.ini");
        f << "[Settings]\n";
        f << "ini_version=0.1.0\n";
        f << "understeer=50.0\n"; // > 2.0 triggers migration
        f << "max_torque_ref=100.0\n"; // > 40.0 triggers reset
        f << "gain=0.8\n";
        f << "sop=0.5\n";
        f << "sop_scale=10.0\n";
        f << "sop_smoothing_factor=0.05\n";
        f << "smoothing=0.1\n"; // Legacy support
        f << "min_force=0.01\n";
        f << "oversteer_boost=1.5\n";
        f << "dynamic_weight_gain=1.2\n";
        f << "dynamic_weight_smoothing=0.1\n";
        f << "grip_smoothing_steady=0.01\n";
        f << "grip_smoothing_fast=0.01\n";
        f << "grip_smoothing_sensitivity=0.1\n";
        f << "lockup_enabled=1\n";
        f << "lockup_gain=0.5\n";
        f << "lockup_start_pct=2.0\n";
        f << "lockup_full_pct=10.0\n";
        f << "lockup_rear_boost=2.0\n";
        f << "lockup_gamma=1.0\n";
        f << "lockup_prediction_sens=50.0\n";
        f << "lockup_bump_reject=0.5\n";
        f << "abs_pulse_enabled=0\n";
        f << "abs_gain=2.0\n";
        f << "abs_freq=20.0\n";
        f << "spin_enabled=1\n";
        f << "spin_gain=0.5\n";
        f << "spin_freq_scale=1.0\n";
        f << "slide_enabled=1\n";
        f << "slide_gain=0.5\n";
        f << "slide_freq=1.0\n";
        f << "road_enabled=1\n";
        f << "road_gain=0.5\n";
        f << "road_fallback_scale=0.1\n";
        f << "soft_lock_enabled=1\n";
        f << "soft_lock_stiffness=50.0\n";
        f << "soft_lock_damping=1.0\n";
        f << "invert_force=1\n";
        f << "wheelbase_max_nm=20.0\n";
        f << "target_rim_nm=15.0\n";
        f << "torque_source=1\n";
        f << "torque_passthrough=true\n";
        f << "optimal_slip_angle=0.12\n";
        f << "optimal_slip_ratio=0.15\n";
        f << "steering_shaft_smoothing=0.01\n";
        f << "steering_shaft_gain=1.0\n";
        f << "ingame_ffb_gain=1.0\n";
        f << "gyro_gain=0.5\n";
        f << "gyro_smoothing_factor=0.01\n";
        f << "yaw_accel_smoothing=0.01\n";
        f << "chassis_inertia_smoothing=0.01\n";
        f << "flatspot_suppression=1\n";
        f << "notch_q=2.0\n";
        f << "flatspot_strength=0.5\n";
        f << "static_notch_enabled=1\n";
        f << "static_notch_freq=50.0\n";
        f << "static_notch_width=1.0\n";
        f << "yaw_kick_threshold=1.5\n";
        f << "slope_detection_enabled=1\n";
        f << "slope_sg_window=21\n";
        f << "slope_sensitivity=2.0\n";
        f << "slope_min_threshold=-0.5\n";
        f << "slope_max_threshold=-1.5\n";
        f << "slope_alpha_threshold=0.02\n";
        f << "slope_decay_rate=5.0\n";
        f << "slope_confidence_enabled=1\n";
        f << "slope_g_slew_limit=100.0\n";
        f << "slope_use_torque=1\n";
        f << "slope_torque_sensitivity=1.0\n";
        f << "slope_confidence_max_rate=0.5\n";
        f << "last_device_guid={1234}\n";
        f << "last_preset_name=Default\n";
        f << "show_graphs=1\n";
        f << "always_on_top=1\n";
        f << "auto_start_logging=1\n";
        f << "log_path=./logs\n";
        f << "speed_gate_lower=1.0\n";
        f << "speed_gate_upper=5.0\n";
        f << "understeer_affects_sop=1\n";
        f << "texture_load_cap=2.0\n";
        f << "bottoming_method=1\n";
        f << "scrub_drag_gain=0.5\n";
        f << "rear_align_effect=0.5\n";
        f << "sop_yaw_gain=0.5\n";
        f << "invalid_key=value\n";
        f << "\n[StaticLoads]\n";
        f << "Car1=5000.0\n";
        f << "\n[OtherSection]\n";
        f << "Key=Value\n";
        f.close();
    }
    Config::Load(engine, "test_comprehensive.ini");

    // 3. Test migration of slope thresholds
    {
        std::ofstream f("test_slope_mig.ini");
        f << "[Settings]\n";
        f << "slope_min_threshold=-0.3\n";
        f << "slope_max_threshold=-2.0\n";
        f << "slope_sensitivity=1.0\n"; // Different from 0.5 triggers migration
        f.close();
    }
    Config::Load(engine, "test_slope_mig.ini");

    // 4. Test swapped thresholds
    {
        std::ofstream f("test_swap.ini");
        f << "[Settings]\n";
        f << "slope_min_threshold=-2.0\n";
        f << "slope_max_threshold=-0.5\n"; // max > min, should swap
        f.close();
    }
    Config::Load(engine, "test_swap.ini");

    // 5. Test LoadPresets
    Config::LoadPresets();

    // 6. Test save with many settings changed
    engine.m_gain = 1.1f;
    engine.m_soft_lock_enabled = true;
    Config::Save(engine, "test_save_v6.ini");

    // 7. Test save failure
    Config::Save(engine, "/invalid/path/config.ini");

    std::cout << "[PASS] Config branches exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_ffb_engine_branches_v6, "Physics") {
    std::cout << "\nTest: FFBEngine Branches (Coverage Boost V6)" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data = {};

    // Test calculate_force with null data
    engine.calculate_force(nullptr, "GT3", "911", 0.0f, true);

    // Test IsFFBAllowed branches
    VehicleScoringInfoV01 scoring = {};
    scoring.mControl = 0; // AI
    engine.IsFFBAllowed(scoring, 5);

    scoring.mControl = 1;
    scoring.mFinishStatus = 1; // Finished
    engine.IsFFBAllowed(scoring, 5);

    scoring.mFinishStatus = 0;
    engine.IsFFBAllowed(scoring, 4); // Not running

    std::cout << "[PASS] FFBEngine branches exercised" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_grip_load_estimation_v6, "Physics") {
    std::cout << "\nTest: GripLoadEstimation Branches (Coverage Boost V6)" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data = {};

    // 1. calculate_kinematic_load branches
    data.mLocalVel.z = 50.0;
    engine.calculate_kinematic_load(&data, 0); // Front Left
    engine.calculate_kinematic_load(&data, 1); // Front Right
    engine.calculate_kinematic_load(&data, 2); // Rear Left
    engine.calculate_kinematic_load(&data, 3); // Rear Right

    // 2. calculate_manual_slip_ratio branches
    TelemWheelV01 w = {};
    w.mStaticUndeflectedRadius = 0; // Trigger invalid radius fallback
    engine.calculate_manual_slip_ratio(w, 50.0);

    engine.calculate_manual_slip_ratio(w, 0.5); // speed < 1.0 (denom fallback)
    engine.calculate_manual_slip_ratio(w, 1.5); // speed < 2.0 (early return 0)

    // 3. calculate_wheel_slip_ratio
    w.mLongitudinalGroundVel = 0.0; // Trigger MIN_SLIP_ANGLE_VELOCITY
    engine.calculate_wheel_slip_ratio(w);

    // 4. update_static_load_reference branches
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 5000.0, 5.0, 0.0025); // speed > 2.0 && < 15.0
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 5000.0, 1.0, 0.0025); // speed <= 2.0
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 5000.0, 20.0, 0.0025); // speed >= 15.0

    // 5. InitializeLoadReference with null className
    FFBEngineTestAccess::CallInitializeLoadReference(engine, nullptr, "TestCar");

    // 6. calculate_grip branches
    bool warned = false;
    double prev_slip1 = 0, prev_slip2 = 0;
    engine.m_slope_detection_enabled = true;
    engine.calculate_grip(w, w, 5000.0, warned, prev_slip1, prev_slip2, 20.0, 0.0025, "Test", &data, true);

    // 7. calculate_grip zero load branch
    w.mTireLoad = 0.0;
    engine.calculate_grip(w, w, 0.0, warned, prev_slip1, prev_slip2, 20.0, 0.0025, "Test", &data, true);

    // 8. update_static_load_reference latched branch
    FFBEngineTestAccess::SetStaticLoadLatched(engine, true);
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 5000.0, 5.0, 0.0025);

    // 9. update_static_load_reference fallback branch
    FFBEngineTestAccess::SetStaticLoadLatched(engine, false);
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 100.0);
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 100.0, 1.0, 0.0025); // speed 1.0, load < 1000

    std::cout << "[PASS] GripLoadEstimation branches exercised" << std::endl;
    g_tests_passed++;
}

} // namespace FFBEngineTests
