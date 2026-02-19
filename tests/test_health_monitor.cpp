#include "test_ffb_common.h"
#include "../src/HealthMonitor.h"

using namespace FFBEngineTests;

TEST_CASE(test_health_monitor_logic, "Diagnostics") {
    // 1. Healthy Scenario (Direct Torque)
    {
        HealthStatus status = HealthMonitor::Check(400.0, 100.0, 400.0, 1);
        ASSERT_TRUE(status.is_healthy);
        ASSERT_FALSE(status.loop_low);
        ASSERT_FALSE(status.telem_low);
        ASSERT_FALSE(status.torque_low);
    }

    // 2. Healthy Scenario (Legacy Torque)
    {
        HealthStatus status = HealthMonitor::Check(400.0, 100.0, 100.0, 0);
        ASSERT_TRUE(status.is_healthy);
        ASSERT_FALSE(status.loop_low);
        ASSERT_FALSE(status.telem_low);
        ASSERT_FALSE(status.torque_low);
    }

    // 3. Low Loop Rate
    {
        HealthStatus status = HealthMonitor::Check(300.0, 100.0, 100.0, 0);
        ASSERT_FALSE(status.is_healthy);
        ASSERT_TRUE(status.loop_low);
    }

    // 4. Low Telemetry Rate
    {
        HealthStatus status = HealthMonitor::Check(400.0, 50.0, 100.0, 0);
        ASSERT_FALSE(status.is_healthy);
        ASSERT_TRUE(status.telem_low);
    }

    // 5. Low Torque Rate (Direct - Expected 400, got 100)
    {
        HealthStatus status = HealthMonitor::Check(400.0, 100.0, 100.0, 1);
        ASSERT_FALSE(status.is_healthy);
        ASSERT_TRUE(status.torque_low);
        ASSERT_NEAR(status.expected_torque_rate, 400.0, 0.1);
    }

    // 6. Healthy Torque Rate (Direct - 380Hz is okay)
    {
        HealthStatus status = HealthMonitor::Check(400.0, 100.0, 380.0, 1);
        ASSERT_TRUE(status.is_healthy);
    }

    // 7. Borderline Telemetry (95Hz is okay)
    {
        HealthStatus status = HealthMonitor::Check(400.0, 95.0, 100.0, 0);
        ASSERT_TRUE(status.is_healthy);
    }
}
