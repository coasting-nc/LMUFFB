#include "test_ffb_common.h"
#include <iostream>
#include <filesystem>

namespace FFBEngineTests {

/**
 * Test for Issue #316: Safety section in GUI
 */
void test_safety_gui_defaults() {
    std::cout << "\nTest: Issue #316 Safety GUI Defaults" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Verify initial values match expectations from Issue #314
    ASSERT_NEAR(engine.m_safety_window_duration, 2.0f, 0.001);
    ASSERT_NEAR(engine.m_safety_gain_reduction, 0.3f, 0.001);
    ASSERT_NEAR(engine.m_safety_smoothing_tau, 0.2f, 0.001);
    ASSERT_NEAR(engine.m_spike_detection_threshold, 500.0f, 0.001);
    ASSERT_NEAR(engine.m_immediate_spike_threshold, 1500.0f, 0.001);
    ASSERT_NEAR(engine.m_safety_slew_full_scale_time_s, 1.0f, 0.001);
    ASSERT_TRUE(engine.m_stutter_safety_enabled);
    ASSERT_NEAR(engine.m_stutter_threshold, 1.5f, 0.001);
}

void test_safety_preset_application() {
    std::cout << "\nTest: Issue #316 Safety Preset Application" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    Preset p("HighProtection");
    p.safety_window_duration = 5.0f;
    p.safety_gain_reduction = 0.1f;
    p.safety_smoothing_tau = 0.5f;
    p.spike_detection_threshold = 200.0f;
    p.immediate_spike_threshold = 500.0f;
    p.safety_slew_full_scale_time_s = 2.0f;
    p.stutter_safety_enabled = false;
    p.stutter_threshold = 3.0f;

    p.Apply(engine);

    ASSERT_NEAR(engine.m_safety_window_duration, 5.0f, 0.001);
    ASSERT_NEAR(engine.m_safety_gain_reduction, 0.1f, 0.001);
    ASSERT_NEAR(engine.m_safety_smoothing_tau, 0.5f, 0.001);
    ASSERT_NEAR(engine.m_spike_detection_threshold, 200.0f, 0.001);
    ASSERT_NEAR(engine.m_immediate_spike_threshold, 500.0f, 0.001);
    ASSERT_NEAR(engine.m_safety_slew_full_scale_time_s, 2.0f, 0.001);
    ASSERT_FALSE(engine.m_stutter_safety_enabled);
    ASSERT_NEAR(engine.m_stutter_threshold, 3.0f, 0.001);
}

void test_safety_config_persistence() {
    std::cout << "\nTest: Issue #316 Safety Config Persistence" << std::endl;

    std::string test_ini = "test_safety_316.ini";
    if (std::filesystem::exists(test_ini)) std::filesystem::remove(test_ini);

    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_safety_window_duration = 3.3f;
    engine.m_safety_gain_reduction = 0.44f;
    engine.m_safety_smoothing_tau = 0.123f;
    engine.m_spike_detection_threshold = 666.0f;
    engine.m_immediate_spike_threshold = 1234.0f;
    engine.m_safety_slew_full_scale_time_s = 0.88f;
    engine.m_stutter_safety_enabled = false;
    engine.m_stutter_threshold = 2.22f;

    Config::Save(engine, test_ini);

    FFBEngine loaded_engine;
    Config::Load(loaded_engine, test_ini);

    ASSERT_NEAR(loaded_engine.m_safety_window_duration, 3.3f, 0.001);
    ASSERT_NEAR(loaded_engine.m_safety_gain_reduction, 0.44f, 0.001);
    ASSERT_NEAR(loaded_engine.m_safety_smoothing_tau, 0.123f, 0.001);
    ASSERT_NEAR(loaded_engine.m_spike_detection_threshold, 666.0f, 0.001);
    ASSERT_NEAR(loaded_engine.m_immediate_spike_threshold, 1234.0f, 0.001);
    ASSERT_NEAR(loaded_engine.m_safety_slew_full_scale_time_s, 0.88f, 0.001);
    ASSERT_FALSE(loaded_engine.m_stutter_safety_enabled);
    ASSERT_NEAR(loaded_engine.m_stutter_threshold, 2.22f, 0.001);

    std::filesystem::remove(test_ini);
}

void test_safety_validation_clamping() {
    std::cout << "\nTest: Issue #316 Safety Validation Clamping" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    Preset p("BrokenSafety");
    p.safety_window_duration = -1.0f;
    p.safety_gain_reduction = 2.0f; // Max 1.0
    p.safety_smoothing_tau = 0.0f;  // Min 0.001
    p.spike_detection_threshold = -10.0f; // Min 1.0
    p.immediate_spike_threshold = 0.0f;   // Min 1.0
    p.safety_slew_full_scale_time_s = 0.0f; // Min 0.01
    p.stutter_threshold = 0.0f; // Min 1.01

    p.Apply(engine);

    ASSERT_GE(engine.m_safety_window_duration, 0.0f);
    ASSERT_LE(engine.m_safety_gain_reduction, 1.0f);
    ASSERT_GE(engine.m_safety_smoothing_tau, 0.001f);
    ASSERT_GE(engine.m_spike_detection_threshold, 1.0f);
    ASSERT_GE(engine.m_immediate_spike_threshold, 1.0f);
    ASSERT_GE(engine.m_safety_slew_full_scale_time_s, 0.01f);
    ASSERT_GE(engine.m_stutter_threshold, 1.01f);
}

AutoRegister reg_issue_316_safety_gui("Issue #316 Safety GUI", "Issue316", {"Safety", "GUI", "Config"}, []() {
    test_safety_gui_defaults();
    test_safety_preset_application();
    test_safety_config_persistence();
    test_safety_validation_clamping();
});

} // namespace FFBEngineTests
