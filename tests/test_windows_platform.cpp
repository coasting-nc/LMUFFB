#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <dinput.h>

// Include headers to test
#include "../src/DirectInputFFB.h"
#include "../src/Config.h"

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

void test_guid_string_conversion() {
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

void test_window_title_extraction() {
    std::cout << "\nTest: Active Window Title (Diagnostics)" << std::endl;
    
    std::string title = DirectInputFFB::GetActiveWindowTitle();
    std::cout << "  Current Window: " << title << std::endl;
    
    // We expect something, even if it's "Unknown" (though on Windows it should work)
    ASSERT_TRUE(!title.empty());
}

void test_config_persistence_guid() {
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

void test_config_always_on_top_persistence() {
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

int main() {
    std::cout << "=== Running Windows Platform Tests ===" << std::endl;

    test_guid_string_conversion();
    test_window_title_extraction();
    test_config_persistence_guid();
    test_config_always_on_top_persistence();

    std::cout << "\n----------------" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
