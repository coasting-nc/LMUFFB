#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "../src/Version.h"
#include "test_ffb_common.h"

// Define helper macro for assertions
#ifndef ASSERT_TRUE
#define ASSERT_TRUE(condition) \
    if ((condition)) { \
        std::cout << "[PASS] " << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }
#endif

// Link version.lib for Version Info functions
#pragma comment(lib, "version.lib")

namespace FFBEngineTests {

TEST_CASE(test_executable_metadata, "Security") {
    std::cout << "\nTest: Executable Metadata & Version Info (Security)" << std::endl;

    // 1. Get current executable path
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        std::cout << "[FAIL] GetModuleFileNameA failed: " << GetLastError() << std::endl;
        g_tests_failed++;
        return;
    }
    std::cout << "  Analyzing: " << exePath << std::endl;

    // 2. Get Version Info Size
    DWORD dwHandle;
    DWORD dwSize = GetFileVersionInfoSizeA(exePath, &dwHandle);
    if (dwSize == 0) {
        std::cout << "[FAIL] GetFileVersionInfoSizeA failed (No Version Resource found): " << GetLastError() << std::endl;
        g_tests_failed++;
        return;
    }

    // 3. Get Version Info Data
    std::vector<BYTE> versionData(dwSize);
    if (!GetFileVersionInfoA(exePath, dwHandle, dwSize, versionData.data())) {
        std::cout << "[FAIL] GetFileVersionInfoA failed: " << GetLastError() << std::endl;
        g_tests_failed++;
        return;
    }

    // 4. Query Values
    struct LangAndCodePage {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;
    UINT cbTranslate;

    // Read the list of languages and code pages.
    if (!VerQueryValueA(versionData.data(), "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
        std::cout << "[FAIL] VerQueryValueA (Translation) failed" << std::endl;
        g_tests_failed++;
        return;
    }

    // Use the first language/codepage available
    char subBlock[50];
    sprintf_s(subBlock, "\\StringFileInfo\\%04x%04x\\CompanyName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

    char* companyName = nullptr;
    UINT len = 0;

    // verify CompanyName
    if (VerQueryValueA(versionData.data(), subBlock, (LPVOID*)&companyName, &len)) {
        std::string company(companyName);
        std::cout << "  CompanyName: " << company << std::endl;
        ASSERT_TRUE(company == "lmuFFB");
    } else {
        std::cout << "[FAIL] Could not query CompanyName" << std::endl;
        g_tests_failed++;
    }

    // verify ProductVersion
    sprintf_s(subBlock, "\\StringFileInfo\\%04x%04x\\ProductVersion", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
    char* productVersion = nullptr;
    if (VerQueryValueA(versionData.data(), subBlock, (LPVOID*)&productVersion, &len)) {
        std::string version(productVersion);
        std::cout << "  ProductVersion: " << version << std::endl;
        // Verify it matches LMUFFB_VERSION defined in Version.h
        // Note: Reset resource file might have appended .0 but Version.h is 0.7.26
        // We'll check if it starts with the defined version string
        bool match = (version.find(LMUFFB_VERSION) == 0);
        if (!match) {
             std::cout << "  [WARN] defined version: " << LMUFFB_VERSION << ", resource: " << version << std::endl;
        }
        ASSERT_TRUE(match);
    } else {
        std::cout << "[FAIL] Could not query ProductVersion" << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_is_window_safety, "Security") {
    std::cout << "\nTest: IsWindow Logic Safety (Simulated)" << std::endl;

    // This test verifies that checking IsWindow on NULL or invalid handles is safe
    // (doesn't crash) and returns false, which is the expected behavior for disconnect.

    // 1. NULL Handle
    HWND nullHwnd = NULL;
    BOOL res1 = IsWindow(nullHwnd);
    ASSERT_TRUE(res1 == 0);

    // 2. Invalid Handle (likely)
    // Casting a random pointer to HWND is generally unsafe if dereferenced,
    // but IsWindow is designed to handle this robustly.
    HWND invalidHwnd = (HWND)(uintptr_t)0x12345678;
    BOOL res2 = IsWindow(invalidHwnd);
    ASSERT_TRUE(res2 == 0);

    // 3. Valid Handle (Console Window)
    HWND consoleHwnd = GetConsoleWindow();
    if (consoleHwnd) {
        BOOL res3 = IsWindow(consoleHwnd);
        ASSERT_TRUE(res3 != 0);
    } else {
        std::cout << "  [SKIP] No console window to test valid handle" << std::endl;
    }
}

} // namespace
