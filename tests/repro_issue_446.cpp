#include "test_ffb_common.h"
#include "../src/logging/HealthMonitor.h"

using namespace FFBEngineTests;
using namespace LMUFFB::Logging;

TEST_CASE(test_ingame_ffb_missing_detection, "Diagnostics") {
    std::cout << "Test: In-Game FFB Missing Detection" << std::endl;
    // torqueSource=1 (In-Game), torque=0.0 (No signal), isRealtime=true, playerControl=0 (Driving)
    HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 0.0, 1, 400.0, true, true, 10, true, 0);
    ASSERT_TRUE(status.ingame_ffb_missing);
}

TEST_CASE(test_ingame_ffb_present_detection, "Diagnostics") {
    std::cout << "Test: In-Game FFB Present Detection" << std::endl;
    // torqueSource=1 (In-Game), torque=400.0 (Signal), isRealtime=true, playerControl=0 (Driving)
    HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 400.0, 1, 400.0, true, true, 10, true, 0);
    ASSERT_FALSE(status.ingame_ffb_missing);
}

TEST_CASE(test_ingame_ffb_missing_not_driving, "Diagnostics") {
    std::cout << "Test: In-Game FFB Missing (Not Driving)" << std::endl;
    // torqueSource=1 (In-Game), torque=0.0 (No signal), isRealtime=false (Menu)
    HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 0.0, 1, 400.0, true, true, 10, false, 0);
    ASSERT_FALSE(status.ingame_ffb_missing);
}
