#include <iostream>
#include <atomic>
#include <mutex>

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
#endif

int main() {
    int total_passed = 0;
    int total_failed = 0;

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
#endif

    std::cout << "\n==============================================" << std::endl;
    std::cout << "           COMBINED TEST SUMMARY              " << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "  TOTAL PASSED : " << total_passed << std::endl;
    std::cout << "  TOTAL FAILED : " << total_failed << std::endl;
    std::cout << "==============================================" << std::endl;

    return (total_failed > 0) ? 1 : 0;
}
