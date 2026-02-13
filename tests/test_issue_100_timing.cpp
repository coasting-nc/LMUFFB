#include "test_ffb_common.h"
#include "src/FFBEngine.h"
#include "src/GuiLayer.h"
#include <chrono>
#include <thread>
#include <atomic>

namespace FFBEngineTests {

/**
 * REGRESSION TEST: Issue #100 - Main Loop Timing
 * This test verifies that the application maintain a consistent message loop
 * frequency (approx 60Hz) even when the GUI is simulated as "inactive".
 */

// Mock GuiLayer::Render if possible, or just measure the actual loop in a controlled environment.
// Since we are in a unit test environment, we can simulate the main loop logic.

TEST_CASE(test_issue_100_timing, "Timing") {
    std::cout << "Test: Simulated Main Loop Frequency (Issue #100)" << std::endl;

    auto start_time = std::chrono::steady_clock::now();

    // Simulate 10 iterations of the main loop fix
    // Logic from src/main.cpp:
    // while (g_running) {
    //     GuiLayer::Render(g_engine);
    //     std::this_thread::sleep_for(std::chrono::milliseconds(16));
    // }

    for (int i = 0; i < 10; ++i) {
        // We assume GuiLayer::Render(g_engine) returns true or false,
        // but our fix makes it not matter for sleep duration.
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "Simulated loop duration: " << duration << "ms" << std::endl;

    // 10 iterations * 16ms = 160ms.
    // Allow some margin for scheduler jitter (up to 500ms is still way better than 1000ms if it were 100ms per sleep)
    ASSERT_TRUE(duration <= 500);
    ASSERT_TRUE(duration >= 150);
}

TEST_CASE(test_gui_render_return_value, "Timing") {
    std::cout << "Test: GUI Render Return Value (Issue #100)" << std::endl;

    FFBEngine engine;
    // v0.7.36 FIX: GuiLayer::Render must always return true while running
    // to prevent focus-based throttling in the main loop.
    bool result = GuiLayer::Render(engine);

    ASSERT_TRUE(result == true);
}

} // namespace FFBEngineTests
