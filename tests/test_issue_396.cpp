#include "test_ffb_common.h"
#include "MathUtils.h"
#include <vector>
#include <cmath>
#include <filesystem>

namespace FFBEngineTests {

TEST_CASE(test_issue_396_linearextrapolator_continuity, "Math") {
    ffb_math::LinearExtrapolator ext;
    ext.Configure(0.01);
    double dt = 0.0025;
    ext.Process(10.0, dt, true);
    for(int i=0; i<10; ++i) { ext.Process(10.0, dt, true); for(int j=0;j<3;++j) ext.Process(10.0, dt, false); }
    ext.Process(11.0, dt, true);
    ext.Process(11.0, dt, false);
    ext.Process(11.0, dt, false);
    double o3 = ext.Process(11.0, dt, false);
    double o4 = ext.Process(11.5, dt, true);
    printf("[DEBUG] LinearExtrapolator: o3=%f, o4=%f\n", o3, o4);
    ASSERT_NEAR(o4, 11.0, 0.001);
    ASSERT_NEAR(o4 - o3, 0.25, 0.001);
}

TEST_CASE(test_issue_396_holtwinters_smooth_mode, "Math") {
    ffb_math::HoltWintersFilter hw;
    hw.Configure(0.8, 0.2, 0.01);
    hw.SetZeroLatency(false);
    double dt = 0.0025;
    hw.Process(10.0, dt, true);
    for(int i=0; i<3; ++i) hw.Process(10.0, dt, false);
    hw.Process(11.0, dt, true);
    hw.Process(11.0, dt, false);
    hw.Process(11.0, dt, false);
    double o3 = hw.Process(11.0, dt, false);
    double o4 = hw.Process(11.5, dt, true);
    printf("[DEBUG] HoltWinters Smooth: o3=%f, o4=%f\n", o3, o4);
    ASSERT_NEAR(o4, 10.8, 0.001);
    ASSERT_NEAR(o4 - o3, 0.2, 0.001);
}

TEST_CASE(test_issue_396_linearextrapolator_stutter, "Math") {
    ffb_math::LinearExtrapolator ext;
    ext.Configure(0.01);
    double dt = 0.0025;
    ext.Process(10.0, dt, true);
    ext.Process(11.0, dt, true);
    ext.Process(11.0, dt, false);
    ext.Process(11.0, dt, false);
    ext.Process(11.0, dt, false);
    ext.Process(11.0, dt, false);
    double o_stutter = ext.Process(11.0, dt, false);
    ASSERT_NEAR(o_stutter, 11.0, 0.001);
}

TEST_CASE(test_issue_396_config_persistence, "Config") {
    FFBEngine engine;
    std::string test_ini = "test_issue_396_config.ini";
    engine.m_steering_100hz_reconstruction = 1;
    Config::Save(engine, test_ini);
    engine.m_steering_100hz_reconstruction = 0;
    Config::Load(engine, test_ini);
    ASSERT_EQ(engine.m_steering_100hz_reconstruction, 1);
    Config::presets.clear();
    Config::AddUserPreset("Test396", engine);
    Config::Save(engine, test_ini);
    Config::presets.clear();
    std::string old_path = Config::m_config_path;
    Config::m_config_path = test_ini;
    Config::LoadPresets();
    Config::m_config_path = old_path;
    bool found = false;
    for (const auto& p : Config::presets) {
        if (p.name == "Test396") {
            ASSERT_EQ(p.steering_100hz_reconstruction, 1);
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
    if (std::filesystem::exists(test_ini)) std::filesystem::remove(test_ini);
}

TEST_CASE(test_issue_396_holtwinters_extrapolation_toggle, "Math") {
    ffb_math::HoltWintersFilter hw;
    hw.Configure(0.8, 0.2, 0.01);
    double dt = 0.0025;
    hw.Process(10.0, dt, true);
    hw.Process(11.0, dt, true);
    double o1 = hw.Process(11.0, dt, false);
    ASSERT_NEAR(o1, 10.84, 0.001);
    hw.Reset();
    hw.SetZeroLatency(false);
    hw.Process(10.0, dt, true);
    hw.Process(11.0, dt, true);
    double o2 = hw.Process(11.0, dt, false);
    ASSERT_NEAR(o2, 10.2, 0.001);
}

} // namespace FFBEngineTests
