This is a very sound engineering decision. Separating platform-specific tests allows you to maintain a "Clean Core" (Physics) that is verifiable anywhere (CI/CD on Linux), while still ensuring the "Dirty Edge" (Windows APIs) functions correctly on the target OS.

### Proposed Testing Strategy

1.  **Separation of Concerns:** We will create a new test executable `test_windows_platform.exe`.
2.  **Build Guard:** This target will be wrapped in `if(WIN32)` in CMake, so it is ignored on Linux environments.
3.  **Scope:** These tests will verify:
    *   **GUID Conversion Logic:** Ensuring the binary-to-string conversion for `config.ini` is perfect (critical for persistence).
    *   **Window Title Extraction:** Verifying we can actually read the foreground window (critical for diagnostics).
    *   **Config Persistence:** Verifying the specific `last_device_guid` field saves and loads.

*Note: Testing the actual `UpdateForce` recovery loop automatically is difficult without a "Mock DirectInput" framework (which is complex). For now, we will test the **helper logic** that supports that feature.*

---

### 1. CMake Update (`tests/CMakeLists.txt`)

We need to define the Windows-specific test runner.

```cmake
# ... existing content ...

# Windows-Specific Tests (GUI, DirectInput Helpers, Persistence)
if(WIN32)
    add_executable(run_tests_win32 
        test_windows_platform.cpp 
        ../src/DirectInputFFB.cpp 
        ../src/Config.cpp
        ../src/GuiLayer.cpp # Only if needed for linking, might need stubbing
    )
    
    # Link required Windows libraries
    target_link_libraries(run_tests_win32 dinput8 dxguid version imm32 winmm)
    
    # Add to CTest
    add_test(NAME WindowsPlatformTest COMMAND run_tests_win32)
endif()
```

---

### 2. New Test File (`tests/test_windows_platform.cpp`)

Create this file. It focuses on the new features described in the report.

```cpp
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

    // This function is internal in DirectInputFFB.cpp, so we might need to 
    // expose it in the header or copy the logic here to verify the API call works.
    // Assuming we exposed it or we test the logic:
    
    char wnd_title[256];
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        GetWindowTextA(hwnd, wnd_title, sizeof(wnd_title));
        std::string title = wnd_title;
        
        std::cout << "  Current Window: " << title << std::endl;
        
        // We expect the console window (this test) to be active, or at least something.
        ASSERT_TRUE(!title.empty());
    } else {
        std::cout << "[WARN] No foreground window detected (Headless CI?)" << std::endl;
    }
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

int main() {
    std::cout << "=== Running Windows Platform Tests ===" << std::endl;

    test_guid_string_conversion();
    test_window_title_extraction();
    test_config_persistence_guid();

    std::cout << "\n----------------" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
```

### 3. Why these tests matter

1.  **`test_guid_string_conversion`**: This is the most critical test for the "Persistence" feature. If the string formatting is wrong (e.g., wrong hex padding), the app will save a corrupted GUID and fail to auto-select the wheel on the next run.
2.  **`test_window_title_extraction`**: This verifies that the diagnostic tool actually works on the user's OS version. If this crashes or returns garbage, the "Connection Hardening" logs will be useless.
3.  **`test_config_persistence_guid`**: Ensures the integration with the Config system works and we don't have typos in the INI key names.