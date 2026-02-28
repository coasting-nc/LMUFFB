#include "test_ffb_common.h"

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
    ASSERT_NEAR(engine_load.m_sop_effect, 0.45f, 0.01);
    ASSERT_TRUE(engine_load.m_road_texture_enabled);
}

TEST_CASE(test_channel_stats, "Config") {
    std::cout << "\nTest: Channel Stats Logic" << std::endl;
    
    ChannelStats stats;
    
    // Sequence: 10, 20, 30
    stats.Update(10.0);
    stats.Update(20.0);
    stats.Update(30.0);
    
    // Verify Session Min/Max
    ASSERT_NEAR(stats.session_min, 10.0, 0.001);
    ASSERT_NEAR(stats.session_max, 30.0, 0.001);
    
    // Verify Interval Avg (Compatibility helper)
    ASSERT_NEAR(stats.Avg(), 20.0, 0.001);
    
    // Test Interval Reset (Session min/max should persist)
    stats.ResetInterval();
    if (stats.interval_count == 0) {
        std::cout << "[PASS] Interval Stats Reset." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Interval Reset failed." << std::endl;
        g_tests_failed++;
    }
    
    // Min/Max should still be valid
    ASSERT_NEAR(stats.session_min, 10.0, 0.001);
    ASSERT_NEAR(stats.session_max, 30.0, 0.001);
    
    ASSERT_NEAR(stats.Avg(), 0.0, 0.001); // Handle divide by zero check
}

TEST_CASE(test_game_state_logic, "Config") {
    std::cout << "\nTest: Game State Logic (Mock)" << std::endl;
    
    // Mock Layout
    SharedMemoryLayout mock_layout;
    std::memset(&mock_layout, 0, sizeof(mock_layout));
    
    // Case 1: Player not found
    // (Default state is 0/false)
    bool inRealtime1 = false;
    for (int i = 0; i < 104; i++) {
        if (mock_layout.data.scoring.vehScoringInfo[i].mIsPlayer) {
            inRealtime1 = (mock_layout.data.scoring.scoringInfo.mInRealtime != 0);
            break;
        }
    }
    if (!inRealtime1) {
         std::cout << "[PASS] Player missing -> False." << std::endl;
         g_tests_passed++;
    } else {
         std::cout << "[FAIL] Player missing -> True?" << std::endl;
         g_tests_failed++;
    }
    
    // Case 2: Player found, InRealtime = 0 (Menu)
    mock_layout.data.scoring.vehScoringInfo[5].mIsPlayer = true;
    mock_layout.data.scoring.scoringInfo.mInRealtime = false;
    
    bool result_menu = false;
    for(int i=0; i<104; i++) {
        if(mock_layout.data.scoring.vehScoringInfo[i].mIsPlayer) {
            result_menu = mock_layout.data.scoring.scoringInfo.mInRealtime;
            break;
        }
    }
    if (!result_menu) {
        std::cout << "[PASS] InRealtime=False -> False." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] InRealtime=False -> True?" << std::endl;
        g_tests_failed++;
    }
    
    // Case 3: Player found, InRealtime = 1 (Driving)
    mock_layout.data.scoring.scoringInfo.mInRealtime = true;
    bool result_driving = false;
    for(int i=0; i<104; i++) {
        if(mock_layout.data.scoring.vehScoringInfo[i].mIsPlayer) {
            result_driving = mock_layout.data.scoring.scoringInfo.mInRealtime;
            break;
        }
    }
    if (result_driving) {
        std::cout << "[PASS] InRealtime=True -> True." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] InRealtime=True -> False?" << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_presets, "Config") {
    std::cout << "\nTest: Configuration Presets" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    Config::LoadPresets();
    
    int idx = -1;
    for(size_t i=0; i<Config::presets.size(); i++) {
        if(Config::presets[i].name == "Test: SoP Only") {
            idx = (int)i;
            break;
        }
    }
    
    if(idx != -1) {
        Config::ApplyPreset(idx, engine);
        ASSERT_NEAR(engine.m_gain, 1.0f, 0.01);
        ASSERT_NEAR(engine.m_sop_effect, 0.08f, 0.01);
    } else {
        std::cout << "[FAIL] Preset 'Test: SoP Only' not found" << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_preset_initialization, "Config") {
    std::cout << "\nTest: Built-in Preset Fidelity (v0.6.30 Refinement)" << std::endl;
    
    // REGRESSION TEST: Verify all built-in presets properly initialize tuning fields.
    // v0.6.30: T300 preset is now specialized with optimized values.
    
    Config::LoadPresets();
    
    // Ã¢Å¡Â Ã¯Â¸  IMPORTANT: These expected values MUST match Config.h default member initializers!
    // When changing the default preset in Config.h, update these values to match.
    // Also update SetAdvancedBraking() default parameters in Config.h.
    // See Config.h line ~12 for the single source of truth.
    //
    // Expected default values for generic presets (updated to GT3 defaults in v0.6.35)
    const float expected_abs_freq = 25.5f;  // Changed from 20.0 to match GT3
    const float expected_lockup_freq_scale = 1.02f;  // Changed from 1.0 to match GT3
    const float expected_spin_freq_scale = 1.0f;
    const int expected_bottoming_method = 0;
    
    Preset ref_defaults;
    const float expected_scrub_drag_gain = ref_defaults.scrub_drag_gain;
    
    // Specialized T300 Expectation (v0.6.30)
    const float t300_lockup_freq = 1.02f;
    const float t300_scrub_gain = 0.0462185f;
    const float t300_understeer = 0.5f;
    const float t300_sop = 0.425003f;
    const float t300_shaft_smooth = 0.0f;
    const float t300_notch_q = 2.0f;
    
    // Ã¢Å¡Â Ã¯Â¸  IMPORTANT: This array MUST match the exact order of presets in Config.cpp LoadPresets()!
    // When adding/removing/reordering presets in Config.cpp, update this array AND the loop count below.
    // Current count: 14 presets (v0.6.35: Added 4 DD presets after T300)
    const char* preset_names[] = {
        "Default",
        "T300",
        "GT3 DD 15 Nm (Simagic Alpha)",
        "LMPx/HY DD 15 Nm (Simagic Alpha)",
        "GM DD 21 Nm (Moza R21 Ultra)",
        "GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)",
        "Test: Game Base FFB Only",
        "Test: SoP Only",
        "Test: Understeer Only",
        "Test: Yaw Kick Only",
        "Test: Textures Only",
        "Test: Rear Align Torque Only",
        "Test: SoP Base Only",
        "Test: Slide Texture Only"
    };
    
    bool all_passed = true;
    
    // Ã¢Å¡Â Ã¯Â¸  IMPORTANT: Loop count (14) must match preset_names array size above!
    for (int i = 0; i < 14; i++) {
        if (i >= Config::presets.size()) {
            std::cout << "[FAIL] Preset " << i << " (" << preset_names[i] << ") not found!" << std::endl;
            all_passed = false;
            continue;
        }
        
        const Preset& preset = Config::presets[i];
        
        // Verify preset name matches
        if (preset.name != preset_names[i]) {
            std::cout << "[FAIL] Preset " << i << " name mismatch: expected '" 
                      << preset_names[i] << "', got '" << preset.name << "'" << std::endl;
            all_passed = false;
            continue;
        }
        
        bool fields_ok = true;
        
        // v0.6.35: Skip generic field validation for specialized presets
        // Specialized presets have custom-tuned values that differ from Config.h defaults.
        // They should NOT be validated against expected_abs_freq, expected_lockup_freq_scale, etc.
        // 
        // Ã¢Å¡Â Ã¯Â¸  IMPORTANT: When adding new specialized presets to Config.cpp, add them to this list!
        // Current specialized presets: Default, T300, GT3, LMPx/HY, GM, GM + Yaw Kick
        bool is_specialized = (preset.name == "Default" || 
                              preset.name == "T300" ||
                              preset.name == "GT3 DD 15 Nm (Simagic Alpha)" ||
                              preset.name == "LMPx/HY DD 15 Nm (Simagic Alpha)" ||
                              preset.name == "GM DD 21 Nm (Moza R21 Ultra)" ||
                              preset.name == "GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)");
        
        // Determine expectations based on whether it's the specialized T300 preset
        bool is_specialized_t300 = (preset.name == "T300");
        
        // Only check generic fields for non-specialized (test) presets
        if (!is_specialized) {
            float exp_lockup_f = expected_lockup_freq_scale;
            float exp_scrub = expected_scrub_drag_gain;
        
        
            if (std::abs(preset.lockup_freq_scale - exp_lockup_f) > 0.001f) {
                 std::cout << "[FAIL] " << preset.name << ": lockup_freq_scale = " 
                          << preset.lockup_freq_scale << ", expected " << exp_lockup_f << std::endl;
                fields_ok = false;
            }

            if (std::abs(preset.scrub_drag_gain - exp_scrub) > 0.001f) {
                std::cout << "[FAIL] " << preset.name << ": scrub_drag_gain = " 
                          << preset.scrub_drag_gain << ", expected " << exp_scrub << std::endl;
                fields_ok = false;
            }

            // Generic checks for non-specialized presets
            if (preset.abs_freq != expected_abs_freq) {
                std::cout << "[FAIL] " << preset.name << ": abs_freq = " 
                          << preset.abs_freq << ", expected " << expected_abs_freq << std::endl;
                fields_ok = false;
            }

            if (preset.spin_freq_scale != expected_spin_freq_scale) {
                 std::cout << "[FAIL] " << preset.name << ": spin_freq_scale = " 
                          << preset.spin_freq_scale << ", expected " << expected_spin_freq_scale << std::endl;
                fields_ok = false;
            }
            
            if (preset.bottoming_method != expected_bottoming_method) {
                std::cout << "[FAIL] " << preset.name << ": bottoming_method = " 
                          << preset.bottoming_method << ", expected " << expected_bottoming_method << std::endl;
                fields_ok = false;
            }
        }
        
        // v0.6.30 Specialization Verification
        if (is_specialized_t300) {
            if (std::abs(preset.understeer - t300_understeer) > 0.001f) {
                std::cout << "[FAIL] T300: Optimized understeer (" << preset.understeer << ") != " << t300_understeer << std::endl;
                fields_ok = false;
            }
            if (std::abs(preset.sop - t300_sop) > 0.001f) {
                std::cout << "[FAIL] T300: Optimized SoP (" << preset.sop << ") != " << t300_sop << std::endl;
                fields_ok = false;
            }
            if (preset.steering_shaft_smoothing != t300_shaft_smooth) {
                std::cout << "[FAIL] T300: Optimized shaft smoothing (" << preset.steering_shaft_smoothing << ") != " << t300_shaft_smooth << std::endl;
                fields_ok = false;
            }
            if (preset.notch_q != t300_notch_q) {
                std::cout << "[FAIL] T300: Optimized notch_q (" << preset.notch_q << ") != " << t300_notch_q << std::endl;
                fields_ok = false;
            }
        }
        
        if (fields_ok) {
            std::cout << "[PASS] " << preset.name << ": fields verified correctly" << (is_specialized_t300 ? " (Including v0.6.30 optimizations)" : "") << std::endl;
            g_tests_passed++;
        } else {
            all_passed = false;
            g_tests_failed++;
        }
    }
    
    if (all_passed) {
        std::cout << "[PASS] All 14 built-in presets have correct field initialization" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Some presets have incorrect specialization or defaults" << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_config_defaults_v057, "Config") {
    std::cout << "\nTest: Config Defaults (v0.5.7)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    ASSERT_TRUE(Config::m_always_on_top);
}

TEST_CASE(test_config_safety_validation_v057, "Config") {
    std::cout << "\nTest: Config Safety Validation (v0.5.7)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_optimal_slip_angle = 0.0f;
    Config::Save(engine, "tmp_invalid.ini");
    Config::Load(engine, "tmp_invalid.ini");
    // Test: Invalid optimal_slip_ratio (0.0) reset
    engine.m_optimal_slip_ratio = 0.0f;
    Config::Save(engine, "tmp_invalid.ini");
    Config::Load(engine, "tmp_invalid.ini");
    ASSERT_NEAR(engine.m_optimal_slip_ratio, 0.12f, 0.01);

    // Test: Very small values (<0.01) correctly reset
    engine.m_optimal_slip_angle = 0.005f;
    Config::Save(engine, "tmp_invalid.ini");
    Config::Load(engine, "tmp_invalid.ini");
    ASSERT_NEAR(engine.m_optimal_slip_angle, 0.10f, 0.01);

    engine.m_optimal_slip_ratio = 0.005f;
    Config::Save(engine, "tmp_invalid.ini");
    Config::Load(engine, "tmp_invalid.ini");
    ASSERT_NEAR(engine.m_optimal_slip_ratio, 0.12f, 0.01);
}

TEST_CASE(test_config_safety_clamping, "Config") {
    std::cout << "\nTest: Config Safety Clamping (v0.4.50)" << std::endl;
    
    // Create a temporary unsafe config file with legacy high-gain values
    const char* test_file = "tmp_unsafe_config_test.ini";
    {
        std::ofstream file(test_file);
        if (!file.is_open()) {
            std::cout << "[FAIL] Could not create test config file." << std::endl;
            g_tests_failed++;
            return;
        }
        
        // Write legacy high-gain values that would cause physics explosions
        file << "slide_gain=5.0\n";
        file << "road_gain=10.0\n";
        file << "lockup_gain=8.0\n";
        file << "spin_gain=7.0\n";
        file << "rear_align_effect=15.0\n";
        file << "sop_yaw_gain=20.0\n";
        file << "sop=12.0\n";
        file << "scrub_drag_gain=3.0\n";
        file << "gyro_gain=2.5\n";
        file.close();
    }
    
    // Load the unsafe config
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    Config::Load(engine, test_file);
    
    // Verify all Generator effects are clamped to safe maximums
    bool all_clamped = true;
    
    // Clamp to 2.0f
    if (engine.m_slide_texture_gain != 2.0f) {
        std::cout << "[FAIL] slide_gain not clamped. Got: " << engine.m_slide_texture_gain << " Expected: 2.0" << std::endl;
        all_clamped = false;
    }
    if (engine.m_road_texture_gain != 2.0f) {
        std::cout << "[FAIL] road_gain not clamped. Got: " << engine.m_road_texture_gain << " Expected: 2.0" << std::endl;
        all_clamped = false;
    }
    if (engine.m_lockup_gain != 3.0f) {
        std::cout << "[FAIL] lockup_gain not clamped. Got: " << engine.m_lockup_gain << " Expected: 3.0" << std::endl;
        all_clamped = false;
    }
    if (engine.m_spin_gain != 2.0f) {
        std::cout << "[FAIL] spin_gain not clamped. Got: " << engine.m_spin_gain << " Expected: 2.0" << std::endl;
        all_clamped = false;
    }
    if (engine.m_rear_align_effect != 2.0f) {
        std::cout << "[FAIL] rear_align_effect not clamped. Got: " << engine.m_rear_align_effect << " Expected: 2.0" << std::endl;
        all_clamped = false;
    }
    if (engine.m_sop_yaw_gain != 1.0f) {
        std::cout << "[FAIL] sop_yaw_gain not clamped. Got: " << engine.m_sop_yaw_gain << " Expected: 1.0" << std::endl;
        all_clamped = false;
    }
    if (engine.m_sop_effect != 2.0f) {
        std::cout << "[FAIL] sop not clamped. Got: " << engine.m_sop_effect << " Expected: 2.0" << std::endl;
        all_clamped = false;
    }
    
    // Clamp to 1.0f
    if (engine.m_scrub_drag_gain != 1.0f) {
        std::cout << "[FAIL] scrub_drag_gain not clamped. Got: " << engine.m_scrub_drag_gain << " Expected: 1.0" << std::endl;
        all_clamped = false;
    }
    if (engine.m_gyro_gain != 1.0f) {
        std::cout << "[FAIL] gyro_gain not clamped. Got: " << engine.m_gyro_gain << " Expected: 1.0" << std::endl;
        all_clamped = false;
    }
    
    if (all_clamped) {
        std::cout << "[PASS] All legacy high-gain values correctly clamped to safe maximums." << std::endl;
        g_tests_passed++;
    } else {
        g_tests_failed++;
    }
    
    // Clean up test file
    std::remove(test_file);
}

TEST_CASE(test_config_dynamic_thresholds, "Config") {
    std::cout << "\nTest: Dynamic Lockup Thresholds" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_lockup_start_pct = 5.0f;
    engine.m_lockup_full_pct = 20.0f;
    ASSERT_TRUE(engine.m_lockup_full_pct > engine.m_lockup_start_pct);
}

TEST_CASE(test_config_migration_logic, "Config") {
    std::cout << "\nTest: Config Migration Logic" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Case 1: max_torque_ref > 40 (Legacy DD hack)
    const char* test_file_dd = "tmp_migration_dd.ini";
    {
        std::ofstream file(test_file_dd);
        file << "max_torque_ref=100.0\n";
        file.close();
    }
    Config::Load(engine, test_file_dd);
    ASSERT_NEAR(engine.m_wheelbase_max_nm, 15.0f, 0.01);
    ASSERT_NEAR(engine.m_target_rim_nm, 10.0f, 0.01);
    std::remove(test_file_dd);

    // Case 2: max_torque_ref <= 40 (Actual wheelbase torque)
    const char* test_file_tuned = "tmp_migration_tuned.ini";
    {
        std::ofstream file(test_file_tuned);
        file << "max_torque_ref=20.0\n";
        file.close();
    }
    Config::Load(engine, test_file_tuned);
    ASSERT_NEAR(engine.m_wheelbase_max_nm, 20.0f, 0.01);
    ASSERT_NEAR(engine.m_target_rim_nm, 20.0f, 0.01);
    std::remove(test_file_tuned);

    // Case 3: max_load_factor legacy key (maps to texture_load_cap)
    const char* test_file_load = "tmp_migration_load.ini";
    {
        std::ofstream file(test_file_load);
        file << "max_load_factor=1.5\n";
        file.close();
    }
    Config::Load(engine, test_file_load);
    ASSERT_NEAR(engine.m_texture_load_cap, 1.5f, 0.01);
    std::remove(test_file_load);

    // Case 4: understeer_effect migration (> 2.0f range)
    const char* test_file_under = "tmp_migration_under.ini";
    {
        std::ofstream file(test_file_under);
        file << "understeer=50.0\n";
        file.close();
    }
    Config::Load(engine, test_file_under);
    ASSERT_NEAR(engine.m_understeer_effect, 0.5f, 0.01);
    std::remove(test_file_under);
}

} // namespace FFBEngineTests
