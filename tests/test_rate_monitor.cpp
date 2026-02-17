#include "test_ffb_common.h"
#include "../src/RateMonitor.h"
#include <thread>
#include <chrono>

using namespace FFBEngineTests;

TEST_CASE(test_rate_monitor_calculation, "Diagnostics") {
    RateMonitor monitor;
    auto start = std::chrono::steady_clock::now();

    // Initial rate should be 0
    ASSERT_NEAR(monitor.GetRate(), 0.0, 0.01);

    // Record 400 events exactly 1 second apart
    for (int i = 0; i < 400; ++i) {
        monitor.RecordEventAt(start + std::chrono::milliseconds(2 * i));
    }

    // Still 0 because only 798ms passed
    ASSERT_NEAR(monitor.GetRate(), 0.0, 0.01);

    // Add one more event at 1000ms
    monitor.RecordEventAt(start + std::chrono::milliseconds(1000));

    // Should be approx 401 Hz (401 events / 1.0s)
    ASSERT_NEAR(monitor.GetRate(), 401.0, 0.1);

    // Next window
    auto start2 = start + std::chrono::milliseconds(1000);
    for (int i = 1; i <= 100; ++i) {
        monitor.RecordEventAt(start2 + std::chrono::milliseconds(10 * i));
    }

    // Should be approx 100 Hz (100 events / 1.0s)
    ASSERT_NEAR(monitor.GetRate(), 100.0, 0.1);
}

TEST_CASE(test_rate_monitor_realtime, "Diagnostics") {
    RateMonitor monitor;

    // Record events as fast as possible for a while
    auto start = std::chrono::steady_clock::now();
    int count = 0;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1100) {
        monitor.RecordEvent();
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    double rate = monitor.GetRate();
    std::cout << "Measured real-time rate: " << rate << " Hz" << std::endl;

    // We expect something roughly between 500 and 1000 Hz depending on scheduler
    ASSERT_GE(rate, 100.0);
}
