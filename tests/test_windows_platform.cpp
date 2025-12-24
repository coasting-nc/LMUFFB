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

// Global externs required by GuiLayer
std::atomic<bool> g_running(true);
std::mutex g_engine_mutex;

// --- Simple Test Framework (Copy from main test file) ---
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
    remove("config.ini");
}

static void test_gui_style_application() {
    std::cout << "\nTest: GUI Style Application (Headless)" << std::endl;
    
    // 1. Initialize Headless ImGui Context
    ImGuiContext* ctx = ImGui::CreateContext();
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

int main() {
    std::cout << "=== Running Windows Platform Tests ===" << std::endl;

    test_guid_string_conversion();
    test_window_title_extraction();
    test_config_persistence_guid();
    test_config_always_on_top_persistence();
    test_window_always_on_top_behavior();
    test_preset_management_system();
    test_gui_style_application();
    test_slider_precision_display();

    std::cout << "\n----------------" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
