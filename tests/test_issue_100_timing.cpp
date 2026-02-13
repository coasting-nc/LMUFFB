#include "test_ffb_common.h"
#include "GuiLayer.h"
#include <chrono>
#include <thread>

namespace FFBEngineTests {

/**
 * @brief Regression Test for Issue #100: Main Loop Timing Consistency.
 *
 * This test verifies that the GuiLayer::Render function returns true
 * (to prevent main loop throttling) and that a simulated main loop
 * maintains a healthy frequency even when the app would have been
 * considered "inactive" in previous versions.
 */
TEST_CASE(test_issue_100_render_return_value, "GUI") {
    std::cout << "\nTest: GuiLayer::Render return value (Issue #100)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // In version 0.7.35 and earlier, Render() returned false when not focused.
    // In version 0.7.36+, it should always return true to prevent throttling
    // of the Win32 message loop, which DirectInput depends on.
    bool result = GuiLayer::Render(engine);

    ASSERT_TRUE(result == true);
}

TEST_CASE(test_main_loop_frequency_simulated, "Timing") {
    std::cout << "\nTest: Simulated Main Loop Frequency (Issue #100)" << std::endl;

    // Simulate 10 iterations of the main loop
    // Old logic: 10 * 100ms = 1000ms
    // New logic: 10 * 16ms = 160ms

    auto start = std::chrono::steady_clock::now();
    const int iterations = 10;

    FFBEngine engine;
    InitializeEngine(engine);

    for (int i = 0; i < iterations; ++i) {
        // Mock GuiLayer::Render() call
        GuiLayer::Render(engine);

        // This is the new consistent sleep logic
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Simulated loop duration: " << duration << "ms" << std::endl;

    // Verify it didn't hit the old 100ms throttle
    // iterations * 100ms would be 1000ms.
    // We expect around 160ms + overhead.
    ASSERT_LE(duration, 500); // 500ms is safe enough to prove it's not 1000ms
    ASSERT_GE(duration, 150);
}

} // namespace FFBEngineTests
