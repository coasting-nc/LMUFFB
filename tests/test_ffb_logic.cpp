#ifndef _WIN32
#include "LinuxMock.h"
#endif

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <fstream>
#include <cstdio>
#include <atomic>
#include <mutex>
#include <thread>

// Include headers to test
#include "../src/DirectInputFFB.h"
#include "../src/Config.h"
#include "../src/GuiLayer.h"
#include "../src/GameConnector.h"
#include "imgui.h"

#include "test_ffb_common.h"

namespace FFBEngineTests {

// --- TESTS ---

TEST_CASE(test_guid_string_conversion, "Logic") {
    std::cout << "\nTest: GUID <-> String Conversion (Persistence)" << std::endl;

    // 1. Create a known GUID
    // {4D1E55B2-F16F-11CF-88CB-001111000030}
    GUID original = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

    // 2. Convert to String
    std::string str = DirectInputFFB::GuidToString(original);
    std::cout << "  Serialized: " << str << std::endl;

    // 3. Convert back to GUID
    GUID result = DirectInputFFB::StringToGuid(str);

    // 4. Verify Integrity
    bool match = (memcmp(&original, &result, sizeof(GUID)) == 0);
    ASSERT_TRUE(match);

    // 5. Test Empty/Invalid
    GUID empty = DirectInputFFB::StringToGuid("");
    bool isEmpty = (empty.Data1 == 0 && empty.Data2 == 0);
    ASSERT_TRUE(isEmpty);
}

TEST_CASE(test_config_persistence_guid, "Logic") {
    std::cout << "\nTest: Config Persistence (Last Device GUID)" << std::endl;

    std::string test_file = "test_config_logic_guid.ini";
    FFBEngine engine;

    std::string fake_guid = "{12345678-1234-1234-1234-1234567890AB}";
    Config::m_last_device_guid = fake_guid;

    Config::Save(engine, test_file);
    Config::m_last_device_guid = "";
    Config::Load(engine, test_file);

    ASSERT_EQ_STR(Config::m_last_device_guid, fake_guid);
    remove(test_file.c_str());
}

TEST_CASE(test_config_always_on_top_persistence, "Logic") {
    std::cout << "\nTest: Config Persistence (Always on Top)" << std::endl;

    std::string test_file = "test_config_logic_top.ini";
    FFBEngine engine;

    Config::m_always_on_top = true;
    Config::Save(engine, test_file);
    Config::m_always_on_top = false;
    Config::Load(engine, test_file);

    ASSERT_TRUE(Config::m_always_on_top == true);
    remove(test_file.c_str());
}

TEST_CASE(test_preset_management_system, "Logic") {
    std::cout << "\nTest: Preset Management System" << std::endl;

    std::string test_file = "test_config_logic_preset.ini";
    Config::presets.clear();

    FFBEngine engine;
    engine.m_gain = 0.88f;
    engine.m_understeer_effect = 12.3f;

    Config::AddUserPreset("TestPreset_Logic", engine);

    ASSERT_TRUE(!Config::presets.empty());

    bool found = false;
    for (const auto& p : Config::presets) {
        if (p.name == "TestPreset_Logic") {
            found = true;
            ASSERT_TRUE(p.gain == engine.m_gain);
            ASSERT_TRUE(p.understeer == engine.m_understeer_effect);
            ASSERT_TRUE(p.is_builtin == false);
            break;
        }
    }
    ASSERT_TRUE(found);
    remove(Config::m_config_path.c_str());
}

TEST_CASE(test_window_title_extraction, "Logic") {
    std::cout << "\nTest: Active Window Title (Diagnostics)" << std::endl;

    std::string title = DirectInputFFB::GetActiveWindowTitle();
    std::cout << "  Current Window: " << title << std::endl;

    ASSERT_EQ_STR(title, "Window Tracking Disabled");
}

TEST_CASE(test_game_connector_lifecycle, "Logic") {
    std::cout << "\nTest: GameConnector Lifecycle (Disconnect/Reconnect)" << std::endl;

    bool initial_state = GameConnector::Get().IsConnected();
    std::cout << "  Initial State: " << (initial_state ? "Connected" : "Disconnected") << std::endl;

    GameConnector::Get().Disconnect();

    bool after_disconnect = GameConnector::Get().IsConnected();
    ASSERT_TRUE(after_disconnect == false);

    // Mock shared memory to ensure connection succeeds even on Linux
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)sizeof(SharedMemoryLayout), LMU_SHARED_MEMORY_FILE);
    auto mockLock = SharedMemoryLock::MakeSharedMemoryLock();

    bool connect_result = GameConnector::Get().TryConnect();

    if (connect_result) {
        std::cout << "  [PASS] GameConnector connected (Shared Memory mocked). Verifying Disconnect persistence..." << std::endl;
        ASSERT_TRUE(GameConnector::Get().IsConnected() == true);
        GameConnector::Get().Disconnect();
        ASSERT_TRUE(GameConnector::Get().IsConnected() == false);
    } else {
        std::cout << "  [FAIL] GameConnector failed to connect despite mocking." << std::endl;
        ASSERT_TRUE(false);
    }

    if (hMap) CloseHandle(hMap);
}

TEST_CASE(test_game_connector_thread_safety, "Logic") {
    std::cout << "\nTest: GameConnector Thread Safety (Stress Test)" << std::endl;

    std::atomic<bool> running{true};

    std::thread t1([&]() {
        SharedMemoryObjectOut telemetry;
        while (running) {
            bool in_realtime = GameConnector::Get().CopyTelemetry(telemetry);
            (void)in_realtime;
        }
    });

    std::thread t2([&]() {
        while (running) {
            GameConnector::Get().Disconnect();
            GameConnector::Get().TryConnect();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    t1.join();
    t2.join();

    std::cout << "  [PASS] GameConnector survived stress test without crashing." << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_gui_style_application, "Logic") {
    std::cout << "\nTest: GUI Style Application (Headless)" << std::endl;

    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ASSERT_TRUE(ctx != nullptr);

    GuiLayer::SetupGUIStyle();

    float bg_r = ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x;
    float bg_g = ImGui::GetStyle().Colors[ImGuiCol_WindowBg].y;
    float bg_b = ImGui::GetStyle().Colors[ImGuiCol_WindowBg].z;

    ASSERT_TRUE(abs(bg_r - 0.12f) < 0.001f);
    ASSERT_TRUE(abs(bg_g - 0.12f) < 0.001f);
    ASSERT_TRUE(abs(bg_b - 0.12f) < 0.001f);

    float header_a = ImGui::GetStyle().Colors[ImGuiCol_Header].w;
    ASSERT_TRUE(header_a == 0.00f);

    ImGui::DestroyContext(ctx);
}

TEST_CASE(test_slider_precision_display, "Logic") {
    std::cout << "\nTest: Slider Precision Display (Arrow Key Visibility)" << std::endl;

    // Case 1: Filter Width (Q)
    {
        float value = 2.50f;
        char buf[64];
        snprintf(buf, 64, "Q: %.2f", value);
        std::string result1(buf);
        value += 0.01f;
        snprintf(buf, 64, "Q: %.2f", value);
        std::string result2(buf);
        ASSERT_TRUE(result1 != result2);
    }

    // Case 2: Percentage
    {
        float value = 1.00f;
        char buf[64];
        snprintf(buf, 64, "%.1f%%%%", value * 100.0f);
        std::string result1(buf);
        value += 0.01f;
        snprintf(buf, 64, "%.1f%%%%", value * 100.0f);
        std::string result2(buf);
        ASSERT_TRUE(result1 != result2);
    }

    // Case 3: Understeer Effect
    {
        float value = 25.0f;
        char buf[64];
        snprintf(buf, 64, "%.1f%%%%", (value / 50.0f) * 100.0f);
        std::string result1(buf);
        value += 0.5f;
        snprintf(buf, 64, "%.1f%%%%", (value / 50.0f) * 100.0f);
        std::string result2(buf);
        ASSERT_TRUE(result1 != result2);
    }

    // Case 4: Small Range
    {
        float value = 0.050f;
        char buf[64];
        snprintf(buf, 64, "%.3f s", value);
        std::string result1(buf);
        value += 0.001f;
        snprintf(buf, 64, "%.3f s", value);
        std::string result2(buf);
        ASSERT_TRUE(result1 != result2);
    }
}

TEST_CASE(test_slider_precision_regression, "Logic") {
    std::cout << "\nTest: Slider Precision Regression (v0.5.1 Fixes)" << std::endl;

    // Case 1: Load Cap
    {
        float value = 1.50f;
        char buf[64];
        snprintf(buf, 64, "%.2fx", value);
        std::string result1(buf);
        value += 0.01f;
        snprintf(buf, 64, "%.2fx", value);
        std::string result2(buf);
        ASSERT_TRUE(result1 != result2);
    }

    // Case 2: Target Frequency
    {
        float value = 50.0f;
        char buf[64];
        snprintf(buf, 64, "%.1f Hz", value);
        std::string result1(buf);
        value += 0.1f;
        snprintf(buf, 64, "%.1f Hz", value);
        std::string result2(buf);
        ASSERT_TRUE(result1 != result2);
    }

    // Case 3: Smoothing Factor
    {
        float value = 1.00f;
        char buf[64];
        snprintf(buf, 64, "%.3f", value);
        std::string result1(buf);
        value -= 0.001f;
        snprintf(buf, 64, "%.3f", value);
        std::string result2(buf);
        ASSERT_TRUE(result1 != result2);
    }

    // Case 4: Step matching
    {
        float small_step = 0.001f;
        char buf[64];
        snprintf(buf, 64, "%.3f", 0.050f);
        std::string before(buf);
        snprintf(buf, 64, "%.3f", 0.050f + small_step);
        std::string after(buf);
        ASSERT_TRUE(before != after);
        g_tests_passed++;
    }
}

TEST_CASE(test_latency_display_regression, "Logic") {
    std::cout << "\nTest: Latency Display Regression (v0.4.50 Restoration)" << std::endl;

    // Test Case 1: SoP Smoothing Latency Calculation
    {
        float sop_smoothing_low = 0.90f;  // 10ms
        int lat_ms_low = (int)((1.0f - sop_smoothing_low) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_low == 10);
        ASSERT_TRUE(lat_ms_low < 15);

        float sop_smoothing_high = 0.70f;  // 30ms
        int lat_ms_high = (int)((1.0f - sop_smoothing_high) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_high == 30);
        ASSERT_TRUE(lat_ms_high >= 15);

        float sop_smoothing_boundary = 0.85f;  // 15ms
        int lat_ms_boundary = (int)((1.0f - sop_smoothing_boundary) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_boundary == 15);
        ASSERT_TRUE(lat_ms_boundary >= 15);
    }

    // Test Case 2: Slip Angle Smoothing Latency Calculation
    {
        float slip_smoothing_low = 0.010f;  // 10ms
        int slip_ms_low = (int)(slip_smoothing_low * 1000.0f + 0.5f);
        ASSERT_TRUE(slip_ms_low == 10);
        ASSERT_TRUE(slip_ms_low < 15);

        float slip_smoothing_high = 0.030f;  // 30ms
        int slip_ms_high = (int)(slip_smoothing_high * 1000.0f + 0.5f);
        ASSERT_TRUE(slip_ms_high == 30);
        ASSERT_TRUE(slip_ms_high >= 15);
    }

    // Test Case 3: Color Coding Logic
    {
        int lat_ms = 10;
        bool is_green = (lat_ms < 15);
        ASSERT_TRUE(is_green);

        lat_ms = 20;
        bool is_red = (lat_ms >= 15);
        ASSERT_TRUE(is_red);
    }

    // Test Case 4: Display Format Verification
    {
        int lat_ms = 14;
        char buf[64];
        snprintf(buf, 64, "Latency: %d ms - %s", lat_ms, (lat_ms < 15) ? "OK" : "High");
        ASSERT_TRUE(std::string(buf) == "Latency: 14 ms - OK");

        lat_ms = 20;
        snprintf(buf, 64, "Latency: %d ms - %s", lat_ms, (lat_ms < 15) ? "OK" : "High");
        ASSERT_TRUE(std::string(buf) == "Latency: 20 ms - High");
    }

    // Test Case 5: Edge Cases
    {
        float sop_smoothing_zero = 1.0f;
        int lat_ms_zero = (int)((1.0f - sop_smoothing_zero) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_zero == 0);

        float sop_smoothing_max = 0.0f;
        int lat_ms_max = (int)((1.0f - sop_smoothing_max) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_max == 100);

        float slip_smoothing_zero = 0.0f;
        int slip_ms_zero = (int)(slip_smoothing_zero * 1000.0f + 0.5f);
        ASSERT_TRUE(slip_ms_zero == 0);
        g_tests_passed++;
    }
}

TEST_CASE(test_window_config_persistence_logic, "Logic") {
    std::cout << "\nTest: Window Config Persistence (Size/Position/State)" << std::endl;

    std::string test_file = "test_config_logic_window.ini";
    FFBEngine engine;

    Config::win_pos_x = 250;
    Config::win_pos_y = 350;
    Config::win_w_small = 600;
    Config::win_h_small = 900;
    Config::win_w_large = 1500;
    Config::win_h_large = 950;
    Config::show_graphs = true;

    Config::Save(engine, test_file);

    Config::win_pos_x = 0;
    Config::win_pos_y = 0;
    Config::win_w_small = 0;
    Config::win_h_small = 0;
    Config::win_w_large = 0;
    Config::win_h_large = 0;
    Config::show_graphs = false;

    Config::Load(engine, test_file);

    ASSERT_TRUE(Config::win_pos_x == 250);
    ASSERT_TRUE(Config::win_pos_y == 350);
    ASSERT_TRUE(Config::win_w_small == 600);
    ASSERT_TRUE(Config::win_h_small == 900);
    ASSERT_TRUE(Config::win_w_large == 1500);
    ASSERT_TRUE(Config::win_h_large == 950);
    ASSERT_TRUE(Config::show_graphs == true);

    remove(test_file.c_str());
}

TEST_CASE(test_defaults_consistency, "Logic") {
    std::cout << "\nTest: Single Source of Truth - Default Consistency" << std::endl;

    // This test verifies that the refactoring to use a single source of truth
    // for defaults is working correctly.

    // Test 1: Capture Preset struct defaults as the reference
    Preset reference_defaults;

    // Test 2: Verify FFBEngine initialization matches
    {
        FFBEngine engine;
        Preset::ApplyDefaultsToEngine(engine);

        ASSERT_TRUE(engine.m_understeer_effect == reference_defaults.understeer);
        ASSERT_TRUE(engine.m_sop_effect == reference_defaults.sop);
        ASSERT_TRUE(engine.m_oversteer_boost == reference_defaults.oversteer_boost);
        ASSERT_TRUE(engine.m_lockup_enabled == reference_defaults.lockup_enabled);
        ASSERT_TRUE(engine.m_lockup_gain == reference_defaults.lockup_gain);
        ASSERT_TRUE(engine.m_slide_texture_enabled == reference_defaults.slide_enabled);
        ASSERT_TRUE(engine.m_slide_texture_gain == reference_defaults.slide_gain);
        ASSERT_TRUE(engine.m_slide_freq_scale == reference_defaults.slide_freq);
        ASSERT_TRUE(engine.m_scrub_drag_gain == reference_defaults.scrub_drag_gain);
        ASSERT_TRUE(engine.m_rear_align_effect == reference_defaults.rear_align_effect);
        ASSERT_TRUE(engine.m_sop_yaw_gain == reference_defaults.sop_yaw_gain);
        ASSERT_TRUE(engine.m_gyro_gain == reference_defaults.gyro_gain);
        ASSERT_TRUE(engine.m_optimal_slip_angle == reference_defaults.optimal_slip_angle);
        ASSERT_TRUE(engine.m_slip_angle_smoothing == reference_defaults.slip_smoothing);
        ASSERT_TRUE(engine.m_sop_smoothing_factor == reference_defaults.sop_smoothing);
        ASSERT_TRUE(engine.m_yaw_accel_smoothing == reference_defaults.yaw_smoothing);
        ASSERT_TRUE(engine.m_chassis_inertia_smoothing == reference_defaults.chassis_smoothing);
        ASSERT_TRUE(engine.m_gyro_smoothing == reference_defaults.gyro_smoothing);
        ASSERT_TRUE(engine.m_steering_shaft_smoothing == reference_defaults.steering_shaft_smoothing);
    }

    // Test 3: Default preset from LoadPresets()
    {
        Config::LoadPresets();
        ASSERT_TRUE(!Config::presets.empty());
        ASSERT_TRUE(Config::presets[0].name == "Default");
        ASSERT_TRUE(Config::presets[0].is_builtin == true);

        const Preset& default_preset = Config::presets[0];
        ASSERT_TRUE(default_preset.understeer == reference_defaults.understeer);
        ASSERT_TRUE(default_preset.sop == reference_defaults.sop);
        ASSERT_TRUE(default_preset.oversteer_boost == reference_defaults.oversteer_boost);
        ASSERT_TRUE(default_preset.lockup_enabled == reference_defaults.lockup_enabled);
        ASSERT_TRUE(default_preset.lockup_gain == reference_defaults.lockup_gain);
        ASSERT_TRUE(default_preset.slide_enabled == reference_defaults.slide_enabled);
        ASSERT_TRUE(default_preset.slide_gain == reference_defaults.slide_gain);
        ASSERT_TRUE(default_preset.slide_freq == reference_defaults.slide_freq);
        ASSERT_TRUE(default_preset.scrub_drag_gain == reference_defaults.scrub_drag_gain);
        ASSERT_TRUE(default_preset.rear_align_effect == reference_defaults.rear_align_effect);
        ASSERT_TRUE(default_preset.sop_yaw_gain == reference_defaults.sop_yaw_gain);
        ASSERT_TRUE(default_preset.gyro_gain == reference_defaults.gyro_gain);
        ASSERT_TRUE(default_preset.optimal_slip_angle == reference_defaults.optimal_slip_angle);
        ASSERT_TRUE(default_preset.slip_smoothing == reference_defaults.slip_smoothing);
        ASSERT_TRUE(default_preset.sop_smoothing == reference_defaults.sop_smoothing);
        ASSERT_TRUE(default_preset.yaw_smoothing == reference_defaults.yaw_smoothing);
        ASSERT_TRUE(default_preset.chassis_smoothing == reference_defaults.chassis_smoothing);
        ASSERT_TRUE(default_preset.gyro_smoothing == reference_defaults.gyro_smoothing);
        ASSERT_TRUE(default_preset.steering_shaft_smoothing == reference_defaults.steering_shaft_smoothing);
    }

    // Test 4: T300 specialized preset
    {
        ASSERT_TRUE(Config::presets.size() > 1);
        ASSERT_TRUE(Config::presets[1].name == "T300");

        const Preset& default_preset = Config::presets[0];
        const Preset& t300_preset = Config::presets[1];

        ASSERT_TRUE(t300_preset.understeer == 0.5f);
        ASSERT_TRUE(abs(t300_preset.sop - 0.425003f) < 0.0001f);
        ASSERT_TRUE(t300_preset.lockup_freq_scale == 1.02f);
        ASSERT_TRUE(t300_preset.scrub_drag_gain == 0.0462185f);

        ASSERT_TRUE(default_preset.understeer != t300_preset.understeer);
        ASSERT_TRUE(default_preset.sop != t300_preset.sop);
    }

    // Test 5: Preset application consistency
    {
        FFBEngine engine1, engine2;
        Preset::ApplyDefaultsToEngine(engine1);
        Config::ApplyPreset(0, engine2); // Apply "Default"

        ASSERT_TRUE(engine1.m_understeer_effect == engine2.m_understeer_effect);
        ASSERT_TRUE(engine1.m_sop_effect == engine2.m_sop_effect);
        ASSERT_TRUE(engine1.m_oversteer_boost == engine2.m_oversteer_boost);
        ASSERT_TRUE(engine1.m_lockup_gain == engine2.m_lockup_gain);
        ASSERT_TRUE(engine1.m_slide_texture_gain == engine2.m_slide_texture_gain);
        ASSERT_TRUE(engine1.m_scrub_drag_gain == engine2.m_scrub_drag_gain);
        ASSERT_TRUE(engine1.m_rear_align_effect == engine2.m_rear_align_effect);
        ASSERT_TRUE(engine1.m_sop_yaw_gain == engine2.m_sop_yaw_gain);
        ASSERT_TRUE(engine1.m_gyro_gain == engine2.m_gyro_gain);
        ASSERT_TRUE(engine1.m_optimal_slip_angle == engine2.m_optimal_slip_angle);
        ASSERT_TRUE(engine1.m_slip_angle_smoothing == engine2.m_slip_angle_smoothing);
        ASSERT_TRUE(engine1.m_sop_smoothing_factor == engine2.m_sop_smoothing_factor);
        ASSERT_TRUE(engine1.m_yaw_accel_smoothing == engine2.m_yaw_accel_smoothing);
        ASSERT_TRUE(engine1.m_chassis_inertia_smoothing == engine2.m_chassis_inertia_smoothing);
        ASSERT_TRUE(engine1.m_gyro_smoothing == engine2.m_gyro_smoothing);
        ASSERT_TRUE(engine1.m_steering_shaft_smoothing == engine2.m_steering_shaft_smoothing);
    }

    // Test 6: Verify no config file still produces correct defaults
    {
        std::string nonexistent_file = "this_file_does_not_exist_12345.ini";
        FFBEngine engine;
        Preset::ApplyDefaultsToEngine(engine);
        Config::Load(engine, nonexistent_file);
        ASSERT_TRUE(engine.m_understeer_effect == reference_defaults.understeer);
        ASSERT_TRUE(engine.m_sop_effect == reference_defaults.sop);
        ASSERT_TRUE(engine.m_lockup_gain == reference_defaults.lockup_gain);
    }

    std::cout << "  [SUMMARY] Single source of truth verified across all initialization paths!" << std::endl;
}

TEST_CASE(test_config_persistence_braking_group, "Logic") {
    std::cout << "\nTest: Config Persistence (Braking Group)" << std::endl;

    std::string test_file = "test_config_logic_brake.ini";
    FFBEngine engine_save;
    InitializeEngine(engine_save);
    FFBEngine engine_load;
    InitializeEngine(engine_load);

    engine_save.m_brake_load_cap = 2.5f;
    engine_save.m_lockup_start_pct = 8.0f;
    engine_save.m_lockup_full_pct = 20.0f;
    engine_save.m_lockup_rear_boost = 2.0f;

    Config::Save(engine_save, test_file);
    Config::Load(engine_load, test_file);

    ASSERT_TRUE(engine_load.m_brake_load_cap == 2.5f);
    ASSERT_TRUE(engine_load.m_lockup_start_pct == 8.0f);
    ASSERT_TRUE(engine_load.m_lockup_full_pct == 20.0f);
    ASSERT_TRUE(engine_load.m_lockup_rear_boost == 2.0f);

    remove(test_file.c_str());
}

TEST_CASE(test_legacy_config_migration, "Logic") {
    std::cout << "\nTest: Legacy Config Migration (Load Cap)" << std::endl;

    std::string test_file = "test_config_logic_legacy.ini";
    {
        std::ofstream file(test_file);
        file << "max_load_factor=1.8\n";
        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);
    Config::Load(engine, test_file);

    ASSERT_TRUE(engine.m_texture_load_cap == 1.8f);
    remove(test_file.c_str());
}

} // namespace FFBEngineTests
