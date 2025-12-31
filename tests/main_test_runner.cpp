#include <iostream>
#include <atomic>
#include <mutex>
#include <cstdio>
#include "src/Config.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Shared globals required by GuiLayer
std::atomic<bool> g_running(true);
std::mutex g_engine_mutex;

// Forward declarations of runners in namespaces
namespace FFBEngineTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    void Run(); 
}
namespace PersistenceTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    void Run(); 
}
namespace PersistenceTests_v0628 { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    void Run(); 
}

#ifdef _WIN32
namespace WindowsPlatformTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    void Run(); 
}
namespace ScreenshotTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    void Run(); 
}
namespace GuiInteractionTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    void Run(); 
}
#endif

int main() {
    int total_passed = 0;
    int total_failed = 0;

    // Redirect config to a test-specific file to avoid overwriting user settings
    Config::m_config_path = "test_config_runner.ini";
    std::remove(Config::m_config_path.c_str());
    std::remove("imgui.ini");

    // --- FFB Engine Tests ---
    // Always run these as they are platform agnostic (mostly)
    try {
        FFBEngineTests::Run();
        total_passed += FFBEngineTests::g_tests_passed;
        total_failed += FFBEngineTests::g_tests_failed;
    } catch (...) {
        total_failed++;
    }

    try {
        PersistenceTests::Run();
        total_passed += PersistenceTests::g_tests_passed;
        total_failed += PersistenceTests::g_tests_failed;
    } catch (...) {
        total_failed++;
    }

    try {
        PersistenceTests_v0628::Run();
        total_passed += PersistenceTests_v0628::g_tests_passed;
        total_failed += PersistenceTests_v0628::g_tests_failed;
    } catch (...) {
        total_failed++;
    }

#ifdef _WIN32
    std::cout << "\n";
    // --- Windows Platform Tests ---
    try {
        WindowsPlatformTests::Run();
        total_passed += WindowsPlatformTests::g_tests_passed;
        total_failed += WindowsPlatformTests::g_tests_failed;
    } catch (const std::exception& e) {
        std::cout << "[FATAL] Windows Platform Tests threw exception: " << e.what() << std::endl;
        total_failed++;
    } catch (...) {
        std::cout << "[FATAL] Windows Platform Tests threw unknown exception" << std::endl;
        total_failed++;
    }

    std::cout << "\n";
    // --- Screenshot Tests ---
    try {
        ScreenshotTests::Run();
        total_passed += ScreenshotTests::g_tests_passed;
        total_failed += ScreenshotTests::g_tests_failed;
    } catch (const std::exception& e) {
        std::cout << "[FATAL] Screenshot Tests threw exception: " << e.what() << std::endl;
        total_failed++;
    } catch (...) {
        std::cout << "[FATAL] Screenshot Tests threw unknown exception" << std::endl;
        total_failed++;
    }
    std::cout << "\n";
    // --- Gui Interaction Tests ---
    try {
        GuiInteractionTests::Run();
        total_passed += GuiInteractionTests::g_tests_passed;
        total_failed += GuiInteractionTests::g_tests_failed;
    } catch (...) {
        total_failed++;
    }
#endif

    std::cout << "\n==============================================" << std::endl;
    std::cout << "           COMBINED TEST SUMMARY              " << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "  TOTAL PASSED : " << total_passed << std::endl;
    std::cout << "  TOTAL FAILED : " << total_failed << std::endl;
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

    return (total_failed > 0) ? 1 : 0;
}
