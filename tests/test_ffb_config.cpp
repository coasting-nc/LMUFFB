#include "test_ffb_common.h"
#include "../src/PresetRegistry.h"

namespace FFBEngineTests {

TEST_CASE(test_config_persistence, "Config") {
    std::cout << "\nTest: Config Save/Load Persistence" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_gain = 1.23f;
    engine.m_sop_effect = 0.45f;
    engine.m_road_texture_enabled = true;
    Config::Save(engine, "test_config.ini");
    FFBEngine engine_load;
    InitializeEngine(engine_load);
    Config::Load(engine_load, "test_config.ini");
    ASSERT_NEAR(engine_load.m_gain, 1.23f, 0.01);
}

TEST_CASE(test_presets, "Config") {
    std::cout << "\nTest: Configuration Presets" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");
    
    int idx = -1;
    const auto& presets = registry.GetPresets();
    for(size_t i=0; i<presets.size(); i++) {
        if(presets[i].name == "Test: SoP Only") {
            idx = (int)i;
            break;
        }
    }
    
    if(idx != -1) {
        registry.ApplyPreset(idx, engine);
        ASSERT_NEAR(engine.m_gain, 1.0f, 0.01);
        ASSERT_NEAR(engine.m_sop_effect, 0.08f, 0.01);
    } else {
        std::cout << "[FAIL] Preset 'Test: SoP Only' not found" << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_preset_initialization, "Config") {
    std::cout << "\nTest: Built-in Preset Fidelity (v0.6.30 Refinement)" << std::endl;
    
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");
    const auto& presets = registry.GetPresets();
    
    const float expected_abs_freq = 25.5f;
    const float expected_lockup_freq_scale = 1.02f;
    const float expected_spin_freq_scale = 1.0f;
    const int expected_bottoming_method = 0;
    
    Preset ref_defaults;
    const float expected_scrub_drag_gain = ref_defaults.scrub_drag_gain;
    
    const float t300_lockup_freq = 1.02f;
    const float t300_scrub_gain = 0.0462185f;
    const float t300_understeer = 0.5f;
    const float t300_sop = 0.425003f;
    const float t300_shaft_smooth = 0.0f;
    const float t300_notch_q = 2.0f;
    
    const char* preset_names[] = {
        "Default",
        "T300",
        "GT3 DD 15 Nm (Simagic Alpha)",
        "LMPx/HY DD 15 Nm (Simagic Alpha)",
        "GM DD 21 Nm (Moza R21 Ultra)",
        "GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)",
        "Test: Game Base FFB Only",
        "Test: SoP Only",
        "Test: Understeer Only"
    };
    
    bool all_passed = true;
    for (int i = 0; i < 9; i++) {
        if (i >= (int)presets.size()) {
            std::cout << "[FAIL] Preset " << i << " (" << preset_names[i] << ") not found!" << std::endl;
            all_passed = false; continue;
        }
        const Preset& preset = presets[i];
        if (preset.name != preset_names[i]) {
            std::cout << "[FAIL] Preset " << i << " name mismatch: expected '" << preset_names[i] << "', got '" << preset.name << "'" << std::endl;
            all_passed = false; continue;
        }
        
        bool fields_ok = true;
        bool is_specialized = (preset.name == "Default" || preset.name == "T300" || preset.name.find("DD 15 Nm") != std::string::npos || preset.name.find("DD 21 Nm") != std::string::npos);
        
        if (!is_specialized) {
            if (std::abs(preset.lockup_freq_scale - expected_lockup_freq_scale) > 0.001f) fields_ok = false;
            if (std::abs(preset.scrub_drag_gain - expected_scrub_drag_gain) > 0.001f) fields_ok = false;
        }
        
        if (preset.name == "T300") {
            if (std::abs(preset.understeer - t300_understeer) > 0.001f) fields_ok = false;
        }
        
        if (fields_ok) {
            std::cout << "[PASS] " << preset.name << ": fields verified" << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] " << preset.name << ": field mismatch" << std::endl;
            all_passed = false; g_tests_failed++;
        }
    }
}

TEST_CASE(test_config_defaults_v057, "Config") {
    std::cout << "\nTest: Config Defaults (v0.5.7)" << std::endl;
    ASSERT_TRUE(Config::m_always_on_top);
}

} // namespace FFBEngineTests
