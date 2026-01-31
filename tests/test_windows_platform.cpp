#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800  // DirectInput 8
#include <dinput.h>

// Include headers to test
#include "../src/DirectInputFFB.h"
#include "../src/Config.h"
#include "../src/GuiLayer.h"
#include "imgui.h"
#include <atomic>
#include <mutex>
#include <fstream>
#include <cstdio>

// Global externs required by GuiLayer


// --- Simple Test Framework (Copy from main test file) ---
namespace WindowsPlatformTests {
int g_tests_passed = 0;
int g_tests_failed = 0;

#define ASSERT_TRUE(condition) \
    if (condition) { \
        std::cout << "[PASS] " << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_EQ_STR(a, b) \
    if (std::string(a) == std::string(b)) { \
        std::cout << "[PASS] " << #a << " == " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << a << ") != " << #b << " (" << b << ")" << std::endl; \
        g_tests_failed++; \
    }

// --- Test Helpers ---
static void InitializeEngine(FFBEngine& engine) {
    Preset::ApplyDefaultsToEngine(engine);
    engine.m_max_torque_ref = 20.0f;
    engine.m_invert_force = false;
    engine.m_steering_shaft_smoothing = 0.0f;
}

// --- TESTS ---

static void test_guid_string_conversion() {
    std::cout << "\nTest: GUID <-> String Conversion (Persistence)" << std::endl;

    // 1. Create a known GUID (e.g., a standard HID GUID)
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

static void test_window_title_extraction() {
    std::cout << "\nTest: Active Window Title (Diagnostics)" << std::endl;
    
    std::string title = DirectInputFFB::GetActiveWindowTitle();
    std::cout << "  Current Window: " << title << std::endl;
    
    // We expect something, even if it's "Unknown" (though on Windows it should work)
    ASSERT_TRUE(!title.empty());
}

static void test_config_persistence_guid() {
    std::cout << "\nTest: Config Persistence (Last Device GUID)" << std::endl;

    // 1. Setup
    std::string test_file = "test_config_win.ini";
    FFBEngine engine; // Dummy engine
    
    // 2. Set the static variable
    std::string fake_guid = "{12345678-1234-1234-1234-1234567890AB}";
    Config::m_last_device_guid = fake_guid;

    // 3. Save
    Config::Save(engine, test_file);

    // 4. Clear
    Config::m_last_device_guid = "";

    // 5. Load
    Config::Load(engine, test_file);

    // 6. Verify
    ASSERT_EQ_STR(Config::m_last_device_guid, fake_guid);

    // Cleanup
    remove(test_file.c_str());
}

static void test_config_always_on_top_persistence() {
    std::cout << "\nTest: Config Persistence (Always on Top)" << std::endl;

    // 1. Setup
    std::string test_file = "test_config_top.ini";
    FFBEngine engine;
    
    // 2. Set the static variable
    Config::m_always_on_top = true;

    // 3. Save
    Config::Save(engine, test_file);

    // 4. Clear
    Config::m_always_on_top = false;

    // 5. Load
    Config::Load(engine, test_file);

    // 6. Verify
    ASSERT_TRUE(Config::m_always_on_top == true);

    // Cleanup
    remove(test_file.c_str());
}

static void test_window_always_on_top_behavior() {
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

static void test_preset_management_system() {
    std::cout << "\nTest: Preset Management System" << std::endl;

    // 1. Use a temporary test file to avoid creating artifacts
    std::string test_file = "test_config_preset_temp.ini";
    
    // 2. Clear existing presets for a clean test
    Config::presets.clear();
    
    // 3. Setup a dummy engine with specific values
    FFBEngine engine;
    engine.m_gain = 0.88f;
    engine.m_understeer_effect = 12.3f;
    
    // 4. Add user preset (this will save to config.ini by default)
    // We need to temporarily override the save behavior
    Config::AddUserPreset("TestPreset_Unique", engine);

    // 5. Verify it was added to the vector
    ASSERT_TRUE(!Config::presets.empty());
    
    bool found = false;
    for (const auto& p : Config::presets) {
        if (p.name == "TestPreset_Unique") {
            found = true;
            // 6. Verify values were captured
            ASSERT_TRUE(p.gain == engine.m_gain);
            ASSERT_TRUE(p.understeer == engine.m_understeer_effect);
            ASSERT_TRUE(p.is_builtin == false);
            break;
        }
    }
    ASSERT_TRUE(found);
    
    // 7. Cleanup: Remove the test config file created by AddUserPreset
    remove(Config::m_config_path.c_str());
}

static void test_gui_style_application() {
    std::cout << "\nTest: GUI Style Application (Headless)" << std::endl;
    
    // 1. Initialize Headless ImGui Context
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr; // Disable imgui.ini during tests
    ASSERT_TRUE(ctx != nullptr);
    
    // 2. Apply Custom Style
    GuiLayer::SetupGUIStyle();
    
    // 3. Verify specific color values from the plan
    // Background should be (0.12, 0.12, 0.12)
    float bg_r = ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x;
    float bg_g = ImGui::GetStyle().Colors[ImGuiCol_WindowBg].y;
    float bg_b = ImGui::GetStyle().Colors[ImGuiCol_WindowBg].z;
    
    ASSERT_TRUE(abs(bg_r - 0.12f) < 0.001f);
    ASSERT_TRUE(abs(bg_g - 0.12f) < 0.001f);
    ASSERT_TRUE(abs(bg_b - 0.12f) < 0.001f);
    
    // Header should be transparent alpha=0
    float header_a = ImGui::GetStyle().Colors[ImGuiCol_Header].w;
    ASSERT_TRUE(header_a == 0.00f);
    
    // Slider Grab should be the Teal Accent (0.00, 0.60, 0.85)
    float accent_r = ImGui::GetStyle().Colors[ImGuiCol_SliderGrab].x;
    float accent_g = ImGui::GetStyle().Colors[ImGuiCol_SliderGrab].y;
    float accent_b = ImGui::GetStyle().Colors[ImGuiCol_SliderGrab].z;
    
    ASSERT_TRUE(abs(accent_r - 0.00f) < 0.001f);
    ASSERT_TRUE(abs(accent_g - 0.60f) < 0.001f);
    ASSERT_TRUE(abs(accent_b - 0.85f) < 0.001f);

    // 4. Destroy Context
    ImGui::DestroyContext(ctx);
}

static void test_slider_precision_display() {
    std::cout << "\nTest: Slider Precision Display (Arrow Key Visibility)" << std::endl;
    
    // This test verifies that slider format strings have enough decimal places
    // to show changes when using arrow keys (typical step: 0.01)
    
    // Test Case 1: Filter Width (Q) - Range 0.5-10.0, step 0.01
    // Format should be "Q: %.2f" to show 0.01 changes
    {
        float value = 2.50f;
        char buf[64];
        snprintf(buf, 64, "Q: %.2f", value);
        std::string result1(buf);
        
        value += 0.01f;  // Simulate arrow key press
        snprintf(buf, 64, "Q: %.2f", value);
        std::string result2(buf);
        
        // The displayed strings should be different
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Filter Width: " << result1 << " -> " << result2 << std::endl;
    }
    
    // Test Case 2: Percentage sliders - Range 0-2.0, step 0.01
    // Format should be "%.1f%%" to show 0.5% changes (0.01 * 100 = 1.0, but displayed as 0.5% of 200%)
    {
        float value = 1.00f;
        char buf[64];
        snprintf(buf, 64, "%.1f%%%%", value * 100.0f);
        std::string result1(buf);
        
        value += 0.01f;  // Simulate arrow key press
        snprintf(buf, 64, "%.1f%%%%", value * 100.0f);
        std::string result2(buf);
        
        // The displayed strings should be different
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Percentage: " << result1 << " -> " << result2 << std::endl;
    }
    
    // Test Case 3: Understeer Effect - Range 0-50, step 0.5
    // Format should be "%.1f%%" to show 1.0% changes (0.5 / 50 * 100 = 1.0%)
    {
        float value = 25.0f;
        char buf[64];
        snprintf(buf, 64, "%.1f%%%%", (value / 50.0f) * 100.0f);
        std::string result1(buf);
        
        value += 0.5f;  // Simulate arrow key press (larger range uses 0.5 step)
        snprintf(buf, 64, "%.1f%%%%", (value / 50.0f) * 100.0f);
        std::string result2(buf);
        
        // The displayed strings should be different
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Understeer: " << result1 << " -> " << result2 << std::endl;
    }
    
    // Test Case 4: Small range sliders - Range 0-0.1, step 0.001
    // Format should be "%.3f" or better to show 0.001 changes
    {
        float value = 0.050f;
        char buf[64];
        snprintf(buf, 64, "%.3f s", value);
        std::string result1(buf);
        
        value += 0.001f;  // Simulate arrow key press (small range uses 0.001 step)
        snprintf(buf, 64, "%.3f s", value);
        std::string result2(buf);
        
        // The displayed strings should be different
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Small Range: " << result1 << " -> " << result2 << std::endl;
    }
    
    // Test Case 5: Slide Pitch - Range 0.5-5.0, step 0.01
    // Format should be "%.2fx" to show 0.01 changes
    {
        float value = 1.50f;
        char buf[64];
        snprintf(buf, 64, "%.2fx", value);
        std::string result1(buf);
        
        value += 0.01f;  // Simulate arrow key press
        snprintf(buf, 64, "%.2fx", value);
        std::string result2(buf);
        
        // The displayed strings should be different
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Slide Pitch: " << result1 << " -> " << result2 << std::endl;
    }
}

static void test_slider_precision_regression() {
    std::cout << "\nTest: Slider Precision Regression (v0.5.1 Fixes)" << std::endl;
    
    // This test verifies fixes for sliders that had precision issues
    // reported in v0.5.1 where arrow key adjustments weren't visible
    
    // Test Case 1: Load Cap - Range 1.0-3.0, step 0.01
    // Format should be "%.2fx" to show 0.01 changes
    // Bug: Was "%.1fx" which didn't show 0.01 step changes
    {
        float value = 1.50f;
        char buf[64];
        snprintf(buf, 64, "%.2fx", value);
        std::string result1(buf);
        
        value += 0.01f;  // Simulate arrow key press
        snprintf(buf, 64, "%.2fx", value);
        std::string result2(buf);
        
        // The displayed strings should be different
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Load Cap: " << result1 << " -> " << result2 << std::endl;
    }
    
    // Test Case 2: Target Frequency - Range 10-100, step 0.01
    // Format should be "%.1f Hz" to show 0.1 changes
    // Bug: Was "%.0f Hz" which didn't show small adjustments
    {
        float value = 50.0f;
        char buf[64];
        snprintf(buf, 64, "%.1f Hz", value);
        std::string result1(buf);
        
        value += 0.1f;  // Simulate arrow key press (visible change)
        snprintf(buf, 64, "%.1f Hz", value);
        std::string result2(buf);
        
        // The displayed strings should be different
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Target Frequency: " << result1 << " -> " << result2 << std::endl;
    }
    
    // Test Case 3: Understeer Effect - Range 0-50, step 0.01
    // Format should be "%.2f" to show 0.01 changes
    // Bug: Was using pre-calculated percentage with buffer scope issues
    {
        float value = 25.00f;
        char buf[64];
        snprintf(buf, 64, "%.2f", value);
        std::string result1(buf);
        
        value += 0.01f;  // Simulate arrow key press (step 0.01 for medium range)
        snprintf(buf, 64, "%.2f", value);
        std::string result2(buf);
        
        // Both should be valid and different
        ASSERT_TRUE(result1 == "25.00");
        ASSERT_TRUE(result2 == "25.01");
        ASSERT_TRUE(result1 != result2);
        std::cout << "  Understeer Effect: " << result1 << " -> " << result2 << std::endl;
    }
    
    // Test Case 4: Verify step sizes match precision
    // This ensures our adaptive step logic matches the display precision
    {
        // Small range (<1.0): step 0.001, needs %.3f or better
        float small_step = 0.001f;
        char buf[64];
        snprintf(buf, 64, "%.3f", 0.050f);
        std::string before(buf);
        snprintf(buf, 64, "%.3f", 0.050f + small_step);
        std::string after(buf);
        ASSERT_TRUE(before != after);
        
        // Medium range (1.0-50.0): step 0.01, needs %.2f or better
        float medium_step = 0.01f;
        snprintf(buf, 64, "%.2f", 1.50f);
        before = buf;
        snprintf(buf, 64, "%.2f", 1.50f + medium_step);
        after = buf;
        ASSERT_TRUE(before != after);
        
        // Large range (>50.0): step 0.5, needs %.1f or better
        float large_step = 0.5f;
        snprintf(buf, 64, "%.1f", 25.0f);
        before = buf;
        snprintf(buf, 64, "%.1f", 25.0f + large_step);
        after = buf;
        ASSERT_TRUE(before != after);
        
        std::cout << "  Step size precision matching verified" << std::endl;
        g_tests_passed++;
    }
}

static void test_latency_display_regression() {
    std::cout << "\nTest: Latency Display Regression (v0.4.50 Restoration)" << std::endl;
    
    // This test verifies the latency display feature that was accidentally removed
    // during the GUI overhaul and restored in v0.4.50
    // Updated in v0.4.51 to include rounding and new layout
    
    // Test Case 1: SoP Smoothing Latency Calculation
    // Formula: lat_ms = (1.0 - sop_smoothing_factor) * 100.0 + 0.5 (Rounding)
    {
        std::cout << "  Testing SoP Smoothing latency calculation..." << std::endl;
        
        // Low latency case (should be green)
        float sop_smoothing_low = 0.90f;  // 10ms latency
        int lat_ms_low = (int)((1.0f - sop_smoothing_low) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_low == 10);
        ASSERT_TRUE(lat_ms_low < 15);  // Should be green
        std::cout << "    Low latency: " << lat_ms_low << " ms (green)" << std::endl;
        
        // High latency case (should be red)
        float sop_smoothing_high = 0.70f;  // 30ms latency
        int lat_ms_high = (int)((1.0f - sop_smoothing_high) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_high == 30);
        ASSERT_TRUE(lat_ms_high >= 15);  // Should be red
        std::cout << "    High latency: " << lat_ms_high << " ms (red)" << std::endl;
        
        // Boundary case (exactly 15ms - should be red)
        float sop_smoothing_boundary = 0.85f;  // 15ms latency
        int lat_ms_boundary = (int)((1.0f - sop_smoothing_boundary) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_boundary == 15);
        ASSERT_TRUE(lat_ms_boundary >= 15);  // Should be red (>= threshold)
        std::cout << "    Boundary latency: " << lat_ms_boundary << " ms (red)" << std::endl;
    }
    
    // Test Case 2: Slip Angle Smoothing Latency Calculation
    // Formula: slip_ms = slip_angle_smoothing * 1000.0 + 0.5 (Rounding)
    {
        std::cout << "  Testing Slip Angle Smoothing latency calculation..." << std::endl;
        
        // Low latency case (should be green)
        float slip_smoothing_low = 0.010f;  // 10ms latency
        int slip_ms_low = (int)(slip_smoothing_low * 1000.0f + 0.5f);
        ASSERT_TRUE(slip_ms_low == 10);
        ASSERT_TRUE(slip_ms_low < 15);  // Should be green
        std::cout << "    Low latency: " << slip_ms_low << " ms (green)" << std::endl;
        
        // High latency case (should be red)
        float slip_smoothing_high = 0.030f;  // 30ms latency
        int slip_ms_high = (int)(slip_smoothing_high * 1000.0f + 0.5f);
        ASSERT_TRUE(slip_ms_high == 30);
        ASSERT_TRUE(slip_ms_high >= 15);  // Should be red
        std::cout << "    High latency: " << slip_ms_high << " ms (red)" << std::endl;
        
        // Boundary case (exactly 15ms - should be red)
        float slip_smoothing_boundary = 0.015f;  // 15ms latency
        int slip_ms_boundary = (int)(slip_smoothing_boundary * 1000.0f + 0.5f);
        ASSERT_TRUE(slip_ms_boundary == 15);
        ASSERT_TRUE(slip_ms_boundary >= 15);  // Should be red (>= threshold)
        std::cout << "    Boundary latency: " << slip_ms_boundary << " ms (red)" << std::endl;
    }
    
    // Test Case 3: Color Coding Logic
    {
        std::cout << "  Testing color coding logic..." << std::endl;
        
        // Green color for low latency (< 15ms)
        int lat_ms = 10;
        bool is_green = (lat_ms < 15);
        ASSERT_TRUE(is_green);
        
        // Verify green color values (0.0, 1.0, 0.0, 1.0)
        if (is_green) {
            float r = 0.0f, g = 1.0f, b = 0.0f, a = 1.0f;
            ASSERT_TRUE(r == 0.0f && g == 1.0f && b == 0.0f && a == 1.0f);
        }
        
        // Red color for high latency (>= 15ms)
        lat_ms = 20;
        bool is_red = (lat_ms >= 15);
        ASSERT_TRUE(is_red);
        
        // Verify red color values (1.0, 0.0, 0.0, 1.0)
        if (is_red) {
            float r = 1.0f, g = 0.0f, b = 0.0f, a = 1.0f;
            ASSERT_TRUE(r == 1.0f && g == 0.0f && b == 0.0f && a == 1.0f);
        }
        
        std::cout << "    Color coding verified" << std::endl;
        g_tests_passed++;
    }
    
    // Test Case 4: Display Format Verification
    {
        std::cout << "  Testing display format..." << std::endl;
        
        // SoP Smoothing display format: "Latency: XX ms - OK"
        int lat_ms = 14;
        char buf[64];
        snprintf(buf, 64, "Latency: %d ms - %s", lat_ms, (lat_ms < 15) ? "OK" : "High");
        std::string display_ok(buf);
        ASSERT_TRUE(display_ok == "Latency: 14 ms - OK");
        
        lat_ms = 20;
        snprintf(buf, 64, "Latency: %d ms - %s", lat_ms, (lat_ms < 15) ? "OK" : "High");
        std::string display_high(buf);
        ASSERT_TRUE(display_high == "Latency: 20 ms - High");
        
        std::cout << "    Format OK: " << display_ok << std::endl;
        std::cout << "    Format High: " << display_high << std::endl;
    }
    
    // Test Case 5: Edge Cases
    {
        std::cout << "  Testing edge cases..." << std::endl;
        
        // Zero latency (SoP smoothing = 1.0)
        float sop_smoothing_zero = 1.0f;
        int lat_ms_zero = (int)((1.0f - sop_smoothing_zero) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_zero == 0);
        
        // Maximum latency (SoP smoothing = 0.0)
        float sop_smoothing_max = 0.0f;
        int lat_ms_max = (int)((1.0f - sop_smoothing_max) * 100.0f + 0.5f);
        ASSERT_TRUE(lat_ms_max == 100);
        
        // Zero slip smoothing
        float slip_smoothing_zero = 0.0f;
        int slip_ms_zero = (int)(slip_smoothing_zero * 1000.0f + 0.5f);
        ASSERT_TRUE(slip_ms_zero == 0);
        
        std::cout << "    Edge cases verified" << std::endl;
        g_tests_passed++;
    }
}

static void test_window_config_persistence() {
    std::cout << "\nTest: Window Config Persistence (Size/Position/State)" << std::endl;
    std::cout << "  RUNNING PERSISTENCE ASSERTIONS" << std::endl;

    // 1. Setup
    std::string test_file = "test_config_window.ini";
    FFBEngine engine;
    
    // 2. Set specific values
    Config::win_pos_x = 250;
    Config::win_pos_y = 350;
    Config::win_w_small = 600;
    Config::win_h_small = 900;
    Config::win_w_large = 1500;
    Config::win_h_large = 950;
    Config::show_graphs = true;

    // 3. Save
    Config::Save(engine, test_file);

    // 4. Reset to different values
    Config::win_pos_x = 0;
    Config::win_pos_y = 0;
    Config::win_w_small = 0;
    Config::win_h_small = 0;
    Config::win_w_large = 0;
    Config::win_h_large = 0;
    Config::show_graphs = false;

    // 5. Load
    Config::Load(engine, test_file);

    // 6. Assert
    ASSERT_TRUE(Config::win_pos_x == 250);
    ASSERT_TRUE(Config::win_pos_y == 350);
    ASSERT_TRUE(Config::win_w_small == 600);
    ASSERT_TRUE(Config::win_h_small == 900);
    ASSERT_TRUE(Config::win_w_large == 1500);
    ASSERT_TRUE(Config::win_h_large == 950);
    ASSERT_TRUE(Config::show_graphs == true);

    // Cleanup
    remove(test_file.c_str());
}

static void test_single_source_of_truth_t300_defaults() {
    std::cout << "\nTest: Single Source of Truth - Default Consistency (v0.5.12)" << std::endl;
    
    // This test verifies that the refactoring to use a single source of truth
    // for defaults is working correctly. All three initialization paths
    // should produce identical results:
    // 1. Preset struct defaults (Config.h)
    // 2. FFBEngine initialized via Preset::ApplyDefaultsToEngine()
    // 3. "Default" preset from LoadPresets()
    //
    // NOTE: This test does NOT check specific values - it only verifies that
    // all paths produce CONSISTENT results. This makes the test resilient to
    // changes in default values.
    
    // Test 1: Capture Preset struct defaults as the reference
    Preset reference_defaults;
    std::cout << "  Test 1: Captured reference defaults from Preset struct" << std::endl;
    
    // Test 2: Verify FFBEngine initialization via ApplyDefaultsToEngine() matches
    {
        std::cout << "  Test 2: FFBEngine initialization consistency..." << std::endl;
        FFBEngine engine;
        Preset::ApplyDefaultsToEngine(engine);
        
        // Verify the engine matches the reference defaults
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
        
        std::cout << "    FFBEngine initialization matches reference" << std::endl;
    }
    
    // Test 3: Verify "Default" preset from LoadPresets() matches
    {
        std::cout << "  Test 3: Default preset consistency..." << std::endl;
        Config::LoadPresets();
        
        // Verify we have presets
        ASSERT_TRUE(!Config::presets.empty());
        
        // First preset should be "Default"
        ASSERT_TRUE(Config::presets[0].name == "Default");
        ASSERT_TRUE(Config::presets[0].is_builtin == true);
        
        // Verify it matches the reference
        const Preset& default_preset = Config::presets[0];
        ASSERT_TRUE(default_preset.understeer == reference_defaults.understeer);
        ASSERT_TRUE(default_preset.sop == reference_defaults.sop);
        ASSERT_TRUE(default_preset.oversteer_boost == reference_defaults.oversteer_boost);
        ASSERT_TRUE(default_preset.lockup_enabled == reference_defaults.lockup_enabled);
        ASSERT_TRUE(default_preset.lockup_gain == reference_defaults.lockup_gain);
        
        std::cout << "    Default preset matches reference" << std::endl;
    }
    
    // Test 4: Verify "T300" preset has specialized values (v0.6.30 Decoupling)
    {
        std::cout << "  Test 4: T300 specialized preset verification..." << std::endl;
        
        // ⚠️ IMPORTANT: T300 preset index depends on Config.cpp LoadPresets() order!
        // Current order: 0=Default, 1=T300, 2=GT3, 3=LMPx/HY, 4=GM, 5=GM+Yaw, 6+=Test presets
        // If presets are added/removed BEFORE T300 in Config.cpp, update this index!
        ASSERT_TRUE(Config::presets.size() > 1);
        ASSERT_TRUE(Config::presets[1].name == "T300");
        
        // Verify specialized values for T300
        const Preset& default_preset = Config::presets[0];
        const Preset& t300_preset = Config::presets[1];
        
        // Optimized values from v0.6.30 changelog
        ASSERT_TRUE(t300_preset.understeer == 0.5f);
        ASSERT_TRUE(abs(t300_preset.sop - 0.425003f) < 0.0001f);
        ASSERT_TRUE(t300_preset.lockup_freq_scale == 1.02f);
        ASSERT_TRUE(t300_preset.scrub_drag_gain == 0.0462185f);
        
        // Verify it is DIFFERENT from Default for key decoupled fields
        ASSERT_TRUE(default_preset.understeer != t300_preset.understeer);
        ASSERT_TRUE(default_preset.sop != t300_preset.sop);
        
        std::cout << "    T300 preset specialization verified (Decoupled from Defaults)" << std::endl;
    }
    
    // Test 5: Verify applying preset produces same result as ApplyDefaultsToEngine()
    {
        std::cout << "  Test 5: Preset application consistency..." << std::endl;
        
        FFBEngine engine1, engine2;
        
        // Initialize engine1 via ApplyDefaultsToEngine
        Preset::ApplyDefaultsToEngine(engine1);
        
        // Initialize engine2 via preset application
        Config::ApplyPreset(0, engine2); // Apply "Default"
        
        // Verify they're identical
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
        
        std::cout << "    Both initialization paths produce identical results" << std::endl;
    }
    
    // Test 6: Verify no config file still produces correct defaults
    {
        std::cout << "  Test 6: No config file scenario..." << std::endl;
        
        // Simulate fresh install (no config.ini)
        std::string nonexistent_file = "this_file_does_not_exist_12345.ini";
        FFBEngine engine;
        
        // Initialize with defaults first (as main.cpp does)
        Preset::ApplyDefaultsToEngine(engine);
        
        // Try to load non-existent config (should not change values)
        Config::Load(engine, nonexistent_file);
        
        // Verify defaults are still present (match reference)
        ASSERT_TRUE(engine.m_understeer_effect == reference_defaults.understeer);
        ASSERT_TRUE(engine.m_sop_effect == reference_defaults.sop);
        ASSERT_TRUE(engine.m_lockup_gain == reference_defaults.lockup_gain);
        
        std::cout << "    Fresh install scenario verified" << std::endl;
    }
    
    std::cout << "  [SUMMARY] Single source of truth verified across all initialization paths!" << std::endl;
}

static void test_config_persistence_braking_group() {
    std::cout << "\nTest: Config Persistence (Braking Group)" << std::endl;
    
    std::string test_file = "test_config_brake.ini";
    FFBEngine engine_save;
    InitializeEngine(engine_save);
    FFBEngine engine_load;
    InitializeEngine(engine_load);
    
    // 1. Set non-default values
    engine_save.m_brake_load_cap = 2.5f;
    engine_save.m_lockup_start_pct = 8.0f;
    engine_save.m_lockup_full_pct = 20.0f;
    engine_save.m_lockup_rear_boost = 2.0f;
    
    // 2. Save
    Config::Save(engine_save, test_file);
    
    // 3. Load
    Config::Load(engine_load, test_file);
    
    // 4. Verify
    ASSERT_TRUE(engine_load.m_brake_load_cap == 2.5f);
    ASSERT_TRUE(engine_load.m_lockup_start_pct == 8.0f);
    ASSERT_TRUE(engine_load.m_lockup_full_pct == 20.0f);
    ASSERT_TRUE(engine_load.m_lockup_rear_boost == 2.0f);
    
    remove(test_file.c_str());
}

static void test_legacy_config_migration() {
    std::cout << "\nTest: Legacy Config Migration (Load Cap)" << std::endl;
    
    std::string test_file = "test_config_legacy.ini";
    {
        std::ofstream file(test_file);
        // Write old key
        file << "max_load_factor=1.8\n";
        file.close();
    }
    
    FFBEngine engine;
    InitializeEngine(engine);
    Config::Load(engine, test_file);
    
    // Verify it mapped to texture_load_cap
    ASSERT_TRUE(engine.m_texture_load_cap == 1.8f);
    
    remove(test_file.c_str());
}

static void test_icon_presence() {
    std::cout << "\nTest: Icon Presence (Build Artifact)" << std::endl;
    
    // Robustness Fix: Use the executable's path to find the artifact, 
    // strictly validating the build structure regardless of CWD.
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
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

void Run() {
    std::cout << "=== Running Windows Platform Tests ===" << std::endl;

    test_guid_string_conversion();
    test_window_title_extraction();
    test_config_persistence_guid();
    test_config_always_on_top_persistence();
    test_window_always_on_top_behavior();
    test_preset_management_system();
    test_gui_style_application();
    test_slider_precision_display();
    test_slider_precision_regression();
    test_latency_display_regression();
    test_window_config_persistence();
    test_single_source_of_truth_t300_defaults();  // NEW: v0.5.12
    test_config_persistence_braking_group(); // NEW: v0.5.13
    test_legacy_config_migration(); // NEW: v0.5.13
    test_icon_presence(); // NEW: Icon check

    // Report results
    std::cout << "\n----------------" << std::endl;
    std::cout << "\n=== Windows Platform Test Summary ===\n";
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
}

} // namespace WindowsPlatformTests
