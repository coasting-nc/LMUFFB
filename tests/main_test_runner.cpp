#include <iostream>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <filesystem>
#include "src/Config.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Shared globals required by GuiLayer
std::atomic<bool> g_running(true);
std::mutex g_engine_mutex;

namespace FFBEngineTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    extern int g_test_cases_run;
    extern int g_test_cases_passed;
    extern int g_test_cases_failed;
    void Run();
    void ParseTagArguments(int argc, char* argv[]);
}

int main(int argc, char* argv[]) {
    // Parse tag filtering arguments
    FFBEngineTests::ParseTagArguments(argc, argv);
    
    int total_passed = 0;
    int total_failed = 0;

    // Redirect config to a test-specific file to avoid overwriting user settings
    Config::m_config_path = "test_config_runner.ini";
    std::remove(Config::m_config_path.c_str());
    std::remove("imgui.ini");

    // --- Unified Test Suite Execution ---
    // All tests (including Windows-specific ones if compiled) are now auto-registered
    try {
        FFBEngineTests::Run();
        total_passed = FFBEngineTests::g_tests_passed;
        total_failed = FFBEngineTests::g_tests_failed;
    } catch (const std::exception& e) {
        std::cout << "\n[FATAL] Test Runner encountered unhandled exception: " << e.what() << std::endl;
        total_failed++;
    } catch (...) {
        std::cout << "\n[FATAL] Test Runner encountered unknown exception" << std::endl;
        total_failed++;
    }

    std::cout << "\n==============================================" << std::endl;
    std::cout << "           COMBINED TEST SUMMARY              " << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "  TEST CASES   : " << FFBEngineTests::g_test_cases_passed << "/" << FFBEngineTests::g_test_cases_run << std::endl;
    std::cout << "  ASSERTIONS   : " << total_passed << " passed, " << total_failed << " failed" << std::endl;
    std::cout << "==============================================" << std::endl;

    // Cleanup artifacts
    std::remove(Config::m_config_path.c_str());
    std::remove("test_persistence.ini");
    std::remove("test_config_win.ini");
    std::remove("test_config_top.ini");
    std::remove("test_config_preset_temp.ini");
    std::remove("test_config_brake.ini");
    std::remove("test_config_sg.ini");
    std::remove("test_config_ap.ini");
    std::remove("test_version.ini");
    std::remove("roundtrip.ini");
    std::remove("test_clamp.ini");
    std::remove("test_isolation.ini");
    std::remove("test_order.ini");
    std::remove("test_legacy.ini");
    std::remove("test_comments.ini");
    std::remove("imgui.ini");
    
    try {
        if (std::filesystem::exists("test_logs")) {
            std::filesystem::remove_all("test_logs");
        }
    } catch (...) {}

    return (total_failed > 0) ? 1 : 0;
}
