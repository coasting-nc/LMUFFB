#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <string>
#include "src/core/Config.h"
#include "test_performance_types.h"
#include "src/logging/Logger.h"
#include "src/io/lmu_sm_interface/LmuSharedMemoryWrapper.h"

using namespace LMUFFB;
using namespace LMUFFB::Logging;

#ifdef _WIN32
#include <windows.h>
#endif

namespace LMUFFB {
// Shared globals required by GuiLayer and main.cpp
std::atomic<bool> g_running(true);
std::atomic<bool> g_ffb_active(true);
std::recursive_mutex g_engine_mutex;
FFBEngine g_engine;
SharedMemoryObjectOut g_localData;

// Mock time globals
namespace Utils {
std::chrono::steady_clock::time_point g_mock_time;
bool g_use_mock_time = false;
}
}

namespace FFBEngineTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO; 
    extern int g_test_cases_run;
    extern int g_test_cases_passed;
    extern int g_test_cases_failed;
    extern std::vector<TestDuration> g_test_durations;
    void Run();
    void ParseTagArguments(int argc, char* argv[]);
}

int main(int argc, char* argv[]) noexcept {
    try {
        // Parse tag filtering arguments
        FFBEngineTests::ParseTagArguments(argc, argv);
        
        int total_passed = 0;
        int total_failed = 0;

    // Initialize logger for unit tests
    Logger::Get().Init("test_runner_debug.log");

    // Redirect config to a test-specific file to avoid overwriting user settings
    Config::m_config_path = "test_config_runner.ini";
    if (std::filesystem::exists(Config::m_config_path)) std::filesystem::remove(Config::m_config_path);
    if (std::filesystem::exists("imgui.ini")) std::filesystem::remove("imgui.ini");

    // --- Unified Test Suite Execution ---
    // All tests (including Windows-specific ones if compiled) are now auto-registered
    try {
        FFBEngineTests::Run();
        total_passed = FFBEngineTests::g_tests_passed;
        total_failed = FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO;
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

    if (!FFBEngineTests::g_test_durations.empty()) {
        std::cout << "----------------------------------------------" << std::endl;
        std::cout << "           SLOWEST TESTS (TOP 5)              " << std::endl;
        std::cout << "----------------------------------------------" << std::endl;

        auto slowest = FFBEngineTests::g_test_durations;
        std::sort(slowest.begin(), slowest.end(), [](const auto& a, const auto& b) {
            return a.duration_ms > b.duration_ms;
        });

        int count = 0;
        for (const auto& test : slowest) {
            if (count++ >= 5) break;
            std::cout << "  " << count << ". " << std::left << std::setw(30) << test.name
                      << " : " << std::fixed << std::setprecision(2) << test.duration_ms << " ms" << std::endl;
        }
    }
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
            "test_bad_config.ini", "test_version_presets.ini", "test_legacy_presets.ini",
            "test_comprehensive.ini", "test_save_v6.ini", "test_slope_mig.ini", "test_swap.ini",
            "test_migrate.ini"
        };

        for (const auto& file : to_remove) {
            try {
                if (!file.empty() && std::filesystem::exists(file)) {
                    std::filesystem::remove(file);
                }
            } catch (...) {
                // Ignore cleanup errors for test files
                (void)0;
            }
        }

        try {
            // Clean up all directories starting with "test_logs"
            for (const auto& entry : std::filesystem::directory_iterator(".")) {
                if (entry.is_directory()) {
                    std::string dirname = entry.path().filename().string();
                    if (dirname.find("test_logs") == 0) {
                        try {
                            std::filesystem::remove_all(entry.path());
                        } catch (...) { (void)0; }
                    }
                }
            }
            
            if (std::filesystem::exists("test_gui.csv")) {
                std::filesystem::remove_all("test_gui.csv");
            }
            // Remove any leftover csv, log, and ini artifacts in current dir and logs/ dir
            std::vector<std::string> dirs = {".", "logs"};
            for (const auto& dir : dirs) {
                if (std::filesystem::exists(dir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                        if (!entry.is_regular_file()) continue;

                        std::string filename = entry.path().filename().string();
                        std::string ext = entry.path().extension().string();

                        bool is_lmuffb_log = ((ext == ".csv" || ext == ".bin" || ext == ".log") && filename.find("lmuffb_") == 0);
                        bool is_test_log = (ext == ".log" && filename.find("test_") == 0);
                        bool is_test_ini = (ext == ".ini" && (filename.find("test_") == 0 || filename.find("tmp_") == 0));

                        if (is_lmuffb_log || is_test_log || is_test_ini) {
                            std::filesystem::remove(entry.path());
                        }
                    }
                }
            }
        } catch (...) {
            // Ignore directory cleanup errors
            (void)0;
        }
    };

    // Ensure log file is closed before cleanup
    Logger::Get().Close();
    
    cleanup();

    return (total_failed > 0) ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal exception in test runner: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal unknown exception in test runner." << std::endl;
        return 1;
    }
}
