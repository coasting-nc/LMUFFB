#include "test_ffb_common.h"
#include "../src/logging/HealthMonitor.h"
#include "../src/logging/ChannelMonitor.h"

using namespace FFBEngineTests;

TEST_CASE(test_health_monitor_logic, "Diagnostics") {
    // 1. Healthy Scenario (Direct Torque)
    {
        // 1000Hz Loop, 400Hz Physics
        HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 400.0, 1, 400.0);
        ASSERT_TRUE(status.is_healthy);
        ASSERT_FALSE(status.loop_low);
        ASSERT_FALSE(status.telem_low);
        ASSERT_FALSE(status.torque_low);
    }

    // 2. Healthy Scenario (Legacy Torque)
    {
        HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 100.0, 0, 400.0);
        ASSERT_TRUE(status.is_healthy);
        ASSERT_FALSE(status.loop_low);
        ASSERT_FALSE(status.telem_low);
        ASSERT_FALSE(status.torque_low);
    }

    // 3. Low Loop Rate
    {
        HealthStatus status = HealthMonitor::Check(800.0, 100.0, 100.0, 0, 400.0);
        ASSERT_FALSE(status.is_healthy);
        ASSERT_TRUE(status.loop_low);
    }

    // 4. Low Telemetry Rate
    {
        HealthStatus status = HealthMonitor::Check(1000.0, 50.0, 100.0, 0, 400.0);
        ASSERT_FALSE(status.is_healthy);
        ASSERT_TRUE(status.telem_low);
    }

    // 5. Low Torque Rate (Direct - Expected 400, got 100)
    {
        HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 100.0, 1, 400.0);
        ASSERT_FALSE(status.is_healthy);
        ASSERT_TRUE(status.torque_low);
        ASSERT_NEAR(status.expected_torque_rate, 400.0, 0.1);
    }

    // 6. Healthy Torque Rate (Direct - 380Hz is okay)
    {
        HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 380.0, 1, 400.0);
        ASSERT_TRUE(status.is_healthy);
    }

    // 7. Borderline Telemetry (95Hz is okay)
    {
        HealthStatus status = HealthMonitor::Check(1000.0, 95.0, 100.0, 0, 400.0);
        ASSERT_TRUE(status.is_healthy);
    }

    // 8. Session and Player State (#269, #274)
    {
        // loop, telem, torque, source, physics, isConnected, sessionActive, sessionType, isRealtime, playerControl
        HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 400.0, 1, 400.0, true, true, 10, true, 0);
        ASSERT_TRUE(status.is_healthy);
        ASSERT_TRUE(status.is_connected);
        ASSERT_TRUE(status.session_active);
        ASSERT_EQ((int)status.session_type, 10);
        ASSERT_TRUE(status.is_realtime);
        ASSERT_EQ((int)status.player_control, 0);
    }
}

TEST_CASE(test_channel_monitors_regression, "Diagnostics") {
    LMUFFB::ChannelMonitors cms;
    TelemInfoV01 telem = {};

    // 1. Initial values
    telem.mLocalAccel = {1.0f, 2.0f, 3.0f};
    telem.mLocalVel = {4.0f, 5.0f, 6.0f};
    telem.mLocalRot = {7.0f, 8.0f, 9.0f};
    telem.mLocalRotAccel = {10.0f, 11.0f, 12.0f};
    telem.mUnfilteredSteering = 0.13;
    telem.mFilteredSteering = 0.14;
    telem.mEngineRPM = 1500.0;
    telem.mWheel[WHEEL_FL].mTireLoad = 1600.0;
    telem.mWheel[WHEEL_FR].mTireLoad = 1700.0;
    telem.mWheel[WHEEL_RL].mTireLoad = 1800.0;
    telem.mWheel[WHEEL_RR].mTireLoad = 1900.0;
    telem.mWheel[WHEEL_FL].mLateralForce = 2000.0;
    telem.mWheel[WHEEL_FR].mLateralForce = 2100.0;
    telem.mWheel[WHEEL_RL].mLateralForce = 2200.0;
    telem.mWheel[WHEEL_RR].mLateralForce = 2300.0;
    telem.mPos = {24.0, 25.0, 26.0};
    telem.mDeltaTime = 0.027f;

    cms.UpdateAll(&telem);

    // Verify all 28 channels correctly updated their lastValue
    ASSERT_NEAR(cms.mAccX.lastValue, 1.0, 0.001);
    ASSERT_NEAR(cms.mAccY.lastValue, 2.0, 0.001);
    ASSERT_NEAR(cms.mAccZ.lastValue, 3.0, 0.001);
    ASSERT_NEAR(cms.mVelX.lastValue, 4.0, 0.001);
    ASSERT_NEAR(cms.mVelY.lastValue, 5.0, 0.001);
    ASSERT_NEAR(cms.mVelZ.lastValue, 6.0, 0.001);
    ASSERT_NEAR(cms.mRotX.lastValue, 7.0, 0.001);
    ASSERT_NEAR(cms.mRotY.lastValue, 8.0, 0.001);
    ASSERT_NEAR(cms.mRotZ.lastValue, 9.0, 0.001);
    ASSERT_NEAR(cms.mRotAccX.lastValue, 10.0, 0.001);
    ASSERT_NEAR(cms.mRotAccY.lastValue, 11.0, 0.001);
    ASSERT_NEAR(cms.mRotAccZ.lastValue, 12.0, 0.001);
    ASSERT_NEAR(cms.mUnfSteer.lastValue, 0.13, 0.001);
    ASSERT_NEAR(cms.mFilSteer.lastValue, 0.14, 0.001);
    ASSERT_NEAR(cms.mRPM.lastValue, 1500.0, 0.001);
    ASSERT_NEAR(cms.mLoadFL.lastValue, 1600.0, 0.001);
    ASSERT_NEAR(cms.mLoadFR.lastValue, 1700.0, 0.001);
    ASSERT_NEAR(cms.mLoadRL.lastValue, 1800.0, 0.001);
    ASSERT_NEAR(cms.mLoadRR.lastValue, 1900.0, 0.001);
    ASSERT_NEAR(cms.mLatFL.lastValue, 2000.0, 0.001);
    ASSERT_NEAR(cms.mLatFR.lastValue, 2100.0, 0.001);
    ASSERT_NEAR(cms.mLatRL.lastValue, 2200.0, 0.001);
    ASSERT_NEAR(cms.mLatRR.lastValue, 2300.0, 0.001);
    ASSERT_NEAR(cms.mPosX.lastValue, 24.0, 0.001);
    ASSERT_NEAR(cms.mPosY.lastValue, 25.0, 0.001);
    ASSERT_NEAR(cms.mPosZ.lastValue, 26.0, 0.001);
    ASSERT_NEAR(cms.mDtMon.lastValue, 0.027, 0.001);

    std::cout << "[PASS] ChannelMonitors::UpdateAll correctly mapped all 28 channels." << std::endl;
    g_tests_passed++;
}
