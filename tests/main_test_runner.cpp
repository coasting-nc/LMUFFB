#include <iostream>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <thread>
#include <chrono>
#include <filesystem>
#include "src/Config.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Shared globals required by GuiLayer
std::atomic<bool> g_running(true);
std::recursive_mutex g_engine_mutex;

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

    // Ensure output is visible on Windows before console closes
    std::cout << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Robust cleanup of test artifacts
    auto cleanup = []() {
        std::vector<std::string> to_remove = {
            Config::m_config_path,
            "test_persistence.ini", "test_config_win.ini", "test_config_top.ini",
            "test_config_preset_temp.ini", "test_config_brake.ini", "test_config_sg.ini",
            "test_config_ap.ini", "test_version.ini", "roundtrip.ini",
            "test_clamp.ini", "test_isolation.ini", "test_order.ini",
            "test_legacy.ini", "test_comments.ini", "imgui.ini",
            "config.ini", "test_config_runner.ini", "test_val.ini",
            "test_stability.ini", "tmp_invalid.ini", "test_config.ini",
            "test_preset_persistence.ini", "test_preservation.ini",
            "test_global_save.ini", "test_config_logic_window.ini",
            "test_config_logic_brake.ini", "test_config_logic_legacy.ini",
            "test_config_logic_legacy_slope.ini", "test_config_logic_legacy_slope_min.ini",
            "test_slope_config.ini", "test_slope_minmax.ini", "test_slope_migration.ini",
            "test_config_logic_guid.ini", "test_config_logic_top.ini", "test_config_logic_preset.ini",
            "tmp_unsafe_config_test.ini", "test_export_preset.ini", "collision_test.ini",
            "test_bad_config.ini", "test_version_presets.ini", "test_legacy_presets.ini"
        };

        for (const auto& file : to_remove) {
            try {
                if (!file.empty() && std::filesystem::exists(file)) {
                    std::filesystem::remove(file);
                }
            } catch (...) {}
        }

        try {
            if (std::filesystem::exists("test_logs")) {
                std::filesystem::remove_all("test_logs");
            }
        } catch (...) {}
    };

    cleanup();

    return (total_failed > 0) ? 1 : 0;
}
