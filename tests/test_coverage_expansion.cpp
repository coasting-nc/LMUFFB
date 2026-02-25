#include "test_ffb_common.h"
#include "../src/Logger.h"
#include "../src/VehicleUtils.h"
#include "../src/lmu_sm_interface/SafeSharedMemoryLock.h"
#include "../src/GameConnector.h"
#include "../src/DirectInputFFB.h"
#include "../src/GuiLayer.h"
#include "../src/GuiPlatform.h"
#include "imgui.h"
#include <thread>
#include <fstream>

#ifndef _WIN32
#include "../src/lmu_sm_interface/LinuxMock.h"
#endif

class GuiLayerTestAccess {
public:
    static void DrawTuningWindow(FFBEngine& engine) { GuiLayer::DrawTuningWindow(engine); }
    static void DrawDebugWindow(FFBEngine& engine) { GuiLayer::DrawDebugWindow(engine); }
};

namespace FFBEngineTests {

TEST_CASE(test_logger_expansion, "Diagnostics") {
    // 1. Init
    Logger::Get().Init("test_expansion.log");

    // 2. Log basic
    Logger::Get().Log("Expansion Test: %d %s", 42, "hello");

    // 3. Log string
    Logger::Get().LogStr("Expansion String Test");

    // 4. Log Win32 error mock
    Logger::Get().LogWin32Error("MockContext", 1234);

    // 5. Verify file exists and has content (basic check)
    std::ifstream file("test_expansion.log");
    ASSERT_TRUE(file.is_open());
    std::string line;
    bool found = false;
    while (std::getline(file, line)) {
        if (line.find("Expansion Test") != std::string::npos) found = true;
    }
    ASSERT_TRUE(found);
    file.close();
    std::remove("test_expansion.log"); // Clean up
}

TEST_CASE(test_vehicle_utils_expansion, "Physics") {
    // Missing Line 46 in VehicleUtils.cpp: ORECA in name but no LMP2 in class
    ParsedVehicleClass pvc = ParseVehicleClass("", "ORECA 07");
    ASSERT_EQ((int)pvc, (int)ParsedVehicleClass::LMP2_UNSPECIFIED);

    // Other missing branches
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::UNKNOWN), "Unknown");
    ASSERT_NEAR(GetDefaultLoadForClass(ParsedVehicleClass::UNKNOWN), 4500.0, 0.1);
}

TEST_CASE(test_shared_memory_lock_expansion, "System") {
    auto lockOpt = SharedMemoryLock::MakeSharedMemoryLock();
    ASSERT_TRUE(lockOpt.has_value());

    SharedMemoryLock& lock = lockOpt.value();

    // Move constructor
    SharedMemoryLock lock2 = std::move(lock);

    // Move assignment
    auto lock3Opt = SharedMemoryLock::MakeSharedMemoryLock();
    ASSERT_TRUE(lock3Opt.has_value());
    SharedMemoryLock& lock3 = lock3Opt.value();
    lock3 = std::move(lock2);

    // Lock/Unlock
    ASSERT_TRUE(lock3.Lock(10));
    lock3.Unlock();

    // Reset
    lock3.Reset();

    // SafeSharedMemoryLock move assignment (Line 31)
    auto safeOpt = SafeSharedMemoryLock::MakeSafeSharedMemoryLock();
    ASSERT_TRUE(safeOpt.has_value());
    SafeSharedMemoryLock safe2 = std::move(safeOpt.value());
    SafeSharedMemoryLock safe3(std::move(safe2)); // Move construct
    safe2 = std::move(safe3); // Move assign
    ASSERT_TRUE(safe2.Lock(10));
    safe2.Unlock();
}

TEST_CASE(test_game_connector_expansion, "System") {
    GameConnector& conn = GameConnector::Get();

    // Disconnect if already connected from other tests to start clean
    conn.Disconnect();
    ASSERT_FALSE(conn.IsConnected());

    // Test legacy conflict
    #ifndef _WIN32
    MockSM::GetMaps()["$rFactor2SMMP_Telemetry$"].resize(1024);
    ASSERT_TRUE(conn.CheckLegacyConflict());
    MockSM::GetMaps().erase("$rFactor2SMMP_Telemetry$");
    #endif

    // Test connection success with HWND
    #ifndef _WIN32
    if (MockSM::GetMaps().count("LMU_Data")) {
        SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
        layout->data.generic.appInfo.mAppWindow = (HWND)1;
    }
    #endif

    ASSERT_TRUE(conn.TryConnect());
    ASSERT_TRUE(conn.IsConnected());

    // Test staleness
    ASSERT_FALSE(conn.IsStale(10000));

    // Test Telemetry copy failure (Lock timeout)
    #ifndef _WIN32
    auto& maps = MockSM::GetMaps();
    if (maps.count("LMU_SharedMemoryLockData")) {
        // LONG is used in SharedMemoryLock::LockData
        LONG* lockData = (LONG*)maps["LMU_SharedMemoryLockData"].data();
        lockData[1] = 1; // busy = 1

        SharedMemoryObjectOut dest;
        ASSERT_FALSE(conn.CopyTelemetry(dest));

        lockData[1] = 0; // release
    }
    #endif
}

TEST_CASE(test_config_expansion, "System") {
    FFBEngine engine;

    // Import failure (Line 780)
    ASSERT_FALSE(Config::ImportPreset("non_existent_preset.ini", engine));

    // Save failure (Line 1265)
    Config::Save(engine, "/proc/invalid_path_lmu");

    // Migration logic (Lines 946+)
    {
        std::ofstream ofs("old_version.ini");
        ofs << "ini_version=0.6.0\n";
        ofs << "max_torque_ref=20.0\n";
        ofs << "understeer=2.5\n"; // > 2.0 triggers legacy scaling
        ofs.close();
        Config::Load(engine, "old_version.ini");
        std::remove("old_version.ini"); // Clean up
    }

    // Duplicate Preset name counter (Line 1087)
    Config::presets.clear();
    Preset p;
    p.name = "TestPreset";
    Config::presets.push_back(p);

    Config::DuplicatePreset(0, engine);
    ASSERT_EQ_STR(Config::presets[1].name.c_str(), "TestPreset (Copy)");

    // To trigger Line 1087 (counter loop):
    Preset p2;
    p2.name = "Manual (Copy)";
    Config::presets.push_back(p2);

    Preset p3;
    p3.name = "Manual";
    Config::presets.push_back(p3);
    int idx = (int)Config::presets.size() - 1;
    Config::DuplicatePreset(idx, engine);
    // It tries "Manual (Copy)", fails, then tries "Manual (Copy) 1"
    ASSERT_EQ_STR(Config::presets.back().name.c_str(), "Manual (Copy) 1");

    // Presets expansion
    Config::presets.clear();
    Preset p1_b; p1_b.name = "Builtin"; p1_b.is_builtin = true;
    Config::presets.push_back(p1_b);
    Preset p2_u; p2_u.name = "User"; p2_u.is_builtin = false;
    Config::presets.push_back(p2_u);

    Config::DeletePreset(0, engine); // Fails (builtin)
    ASSERT_EQ((int)Config::presets.size(), 2);
    Config::DeletePreset(1, engine); // Success
    ASSERT_EQ((int)Config::presets.size(), 1);
    Config::DeletePreset(-1, engine);
    ASSERT_FALSE(Config::IsEngineDirtyRelativeToPreset(-1, engine));
}

TEST_CASE(test_direct_input_mock_expansion, "System") {
    DirectInputFFB& di = DirectInputFFB::Get();

    // These calls exercise the #else blocks on Linux
    ASSERT_TRUE(di.Initialize(NULL));

    auto devices = di.EnumerateDevices();
    ASSERT_GT((int)devices.size(), 0);

    GUID dummy = {0};
    ASSERT_TRUE(di.SelectDevice(dummy));
    ASSERT_TRUE(di.IsActive());
    ASSERT_TRUE(di.IsExclusive());

    ASSERT_TRUE(di.UpdateForce(0.5));
    ASSERT_TRUE(di.UpdateForce(0.0));

    di.ReleaseDevice();
    ASSERT_FALSE(di.IsActive());

    di.Shutdown();

    // Coverage for GuidToString and StringToGuid
    GUID g1 = { 0x12345678, 0x1234, 0x5678, { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 } };
    std::string s1 = di.GuidToString(g1);
    GUID g2 = di.StringToGuid(s1);
    ASSERT_TRUE(memcmp(&g1, &g2, sizeof(GUID)) == 0);

    ASSERT_TRUE(di.StringToGuid("").Data1 == 0);
}

TEST_CASE(test_sm_lock_concurrency, "System") {
    auto lockOpt = SharedMemoryLock::MakeSharedMemoryLock();
    ASSERT_TRUE(lockOpt.has_value());
    SharedMemoryLock& lock = lockOpt.value();

    lock.Lock();

    std::thread t([&]() {
        // This should hit the wait path
        if (lock.Lock(1000)) {
            lock.Unlock();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    lock.Unlock();
    t.join();
}

TEST_CASE(test_sm_interface_expansion, "System") {
    SharedMemoryObjectOut src = {};
    SharedMemoryObjectOut dst = {};

    // Test all event branches in CopySharedMemoryObj
    src.generic.events[SME_UPDATE_SCORING] = SME_UPDATE_SCORING;
    src.scoring.scoringInfo.mNumVehicles = 1;
    src.scoring.scoringStreamSize = 10;

    src.generic.events[SME_UPDATE_TELEMETRY] = SME_UPDATE_TELEMETRY;
    src.telemetry.activeVehicles = 1;

    src.generic.events[SME_ENTER] = SME_ENTER;

    CopySharedMemoryObj(dst, src);
    ASSERT_EQ(dst.telemetry.activeVehicles, 1);
    ASSERT_EQ(dst.scoring.scoringStreamSize, 10);

    src.generic.events[SME_ENTER] = (SharedMemoryEvent)0;
    src.generic.events[SME_EXIT] = SME_EXIT;
    CopySharedMemoryObj(dst, src);

    src.generic.events[SME_EXIT] = (SharedMemoryEvent)0;
    src.generic.events[SME_SET_ENVIRONMENT] = SME_SET_ENVIRONMENT;
    CopySharedMemoryObj(dst, src);
}

TEST_CASE(test_gui_platform_expansion, "GUI") {
    SetWindowAlwaysOnTopPlatform(true);
    ASSERT_TRUE(GetGuiPlatform().GetAlwaysOnTopMock());
    SetWindowAlwaysOnTopPlatform(false);
    ASSERT_FALSE(GetGuiPlatform().GetAlwaysOnTopMock());

    ResizeWindowPlatform(100, 100, 800, 600);
    SaveCurrentWindowGeometryPlatform(true);
    SaveCurrentWindowGeometryPlatform(false);

    std::string path;
    SavePresetFileDialogPlatform(path, "test.ini");
    OpenPresetFileDialogPlatform(path);

    ASSERT_TRUE(GetGuiPlatform().GetWindowHandle() == nullptr);

    // Mock branches
    #ifndef _WIN32
    ASSERT_FALSE(OpenFileMappingA(0, 0, nullptr));
    ASSERT_TRUE(IsWindow((HWND)1));
    void* buf; UINT len;
    ASSERT_FALSE(VerQueryValueA(nullptr, "Invalid", &buf, &len));
    #endif
}

TEST_CASE(test_gui_layer_rendering_expansion, "GUI") {
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1920, 1080);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    FFBEngine engine;
    Config::show_graphs = true;

    // Add some presets to trigger branches
    Config::presets.clear();
    Preset p1; p1.name = "Test";
    Config::presets.push_back(p1);
    Config::m_last_preset_name = "Test";

    ImGui::NewFrame();

    GuiLayerTestAccess::DrawTuningWindow(engine);
    GuiLayerTestAccess::DrawDebugWindow(engine);

    ImGui::EndFrame();
    ImGui::DestroyContext(ctx);

    ASSERT_TRUE(true);
}

} // namespace FFBEngineTests
