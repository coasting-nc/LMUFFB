#include "test_ffb_common.h"

namespace FFBEngineTests {

// --- Global Test Counters ---
int g_tests_passed = 0;
int g_tests_failed = 0;
int g_test_cases_run = 0;
int g_test_cases_passed = 0;
int g_test_cases_failed = 0;

// --- Tag Filtering Globals ---
std::vector<std::string> g_tag_filter;
std::vector<std::string> g_tag_exclude;
std::vector<std::string> g_category_filter;
bool g_enable_tag_filtering = false;

// --- Helper: Parse Command Line Arguments ---
void ParseTagArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        // --tag=Physics,Math
        if (arg.find("--tag=") == 0) {
            g_enable_tag_filtering = true;
            std::string tags_str = arg.substr(6);
            std::stringstream ss(tags_str);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                g_tag_filter.push_back(tag);
            }
        }
        // --exclude=Performance
        else if (arg.find("--exclude=") == 0) {
            g_enable_tag_filtering = true;
            std::string tags_str = arg.substr(10);
            std::stringstream ss(tags_str);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                g_tag_exclude.push_back(tag);
            }
        }
        // --category=CorePhysics,SlipGrip
        else if (arg.find("--category=") == 0) {
            g_enable_tag_filtering = true;
            std::string cats_str = arg.substr(11);
            std::stringstream ss(cats_str);
            std::string cat;
            while (std::getline(ss, cat, ',')) {
                g_category_filter.push_back(cat);
            }
        }
        // --help
        else if (arg == "--help" || arg == "-h") {
            std::cout << "\nLMUFFB Test Suite - Tag Filtering\n";
            std::cout << "==================================\n\n";
            std::cout << "Usage: run_combined_tests.exe [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --tag=TAG1,TAG2       Run only tests with specified tags (OR logic)\n";
            std::cout << "  --exclude=TAG1,TAG2   Exclude tests with specified tags\n";
            std::cout << "  --category=CAT1,CAT2  Run only specified test categories\n";
            std::cout << "  --help, -h            Show this help message\n\n";
            std::cout << "Available Tags:\n";
            std::cout << "  Functional: Physics, Math, Integration, Config, Regression, Edge, Performance\n";
            std::cout << "  Component: SoP, Slope, Texture, Grip, Coordinates, Smoothing\n\n";
            std::cout << "Available Categories:\n";
            std::cout << "  CorePhysics, SlipGrip, Understeer, SlopeDetection, Texture,\n";
            std::cout << "  YawGyro, Coordinates, Config, SpeedGate, Internal\n\n";
            std::cout << "Examples:\n";
            std::cout << "  run_combined_tests.exe --tag=Physics\n";
            std::cout << "  run_combined_tests.exe --tag=Physics,Regression\n";
            std::cout << "  run_combined_tests.exe --exclude=Performance\n";
            std::cout << "  run_combined_tests.exe --category=CorePhysics,SlipGrip\n\n";
            std::cout << "For more information, see: docs/dev_docs/test_tagging_system.md\n\n";
            exit(0);
        }
    }
    
    // Print active filters
    if (g_enable_tag_filtering) {
        std::cout << "\n=== Tag Filtering Active ===\n";
        if (!g_tag_filter.empty()) {
            std::cout << "Include Tags: ";
            for (size_t i = 0; i < g_tag_filter.size(); i++) {
                std::cout << g_tag_filter[i];
                if (i < g_tag_filter.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        if (!g_tag_exclude.empty()) {
            std::cout << "Exclude Tags: ";
            for (size_t i = 0; i < g_tag_exclude.size(); i++) {
                std::cout << g_tag_exclude[i];
                if (i < g_tag_exclude.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        if (!g_category_filter.empty()) {
            std::cout << "Categories: ";
            for (size_t i = 0; i < g_category_filter.size(); i++) {
                std::cout << g_category_filter[i];
                if (i < g_category_filter.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "============================\n";
    }
}


// --- Helper: Create Basic Test Telemetry ---
TelemInfoV01 CreateBasicTestTelemetry(double speed, double slip_angle) {
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.0025;
    
    // Time
    data.mDeltaTime = 0.01; // 100Hz
    
    // Velocity
    data.mLocalVel.z = -speed; // Game uses -Z for forward
    
    // Wheel setup (all 4 wheels)
    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mGripFract = 0.0; // Trigger approximation mode
        data.mWheel[i].mTireLoad = 4000.0; // Realistic load
        data.mWheel[i].mStaticUndeflectedRadius = 30; // 0.3m radius
        data.mWheel[i].mRotation = speed * 3.33f; // Match speed (rad/s)
        data.mWheel[i].mLongitudinalGroundVel = speed;
        data.mWheel[i].mLateralPatchVel = slip_angle * speed; // Convert to m/s
        data.mWheel[i].mBrakePressure = 1.0; // Default for tests (v0.6.0)
        data.mWheel[i].mSuspForce = 4000.0; // Grounded (v0.6.0)
        data.mWheel[i].mVerticalTireDeflection = 0.001; // Avoid "missing data" warning (v0.6.21)
    }
    
    return data;
}

// --- Helper: Initialize Engine with Test Defaults ---
void InitializeEngine(FFBEngine& engine) {
    Preset::ApplyDefaultsToEngine(engine);
    // v0.5.12: Force consistent baseline for legacy tests
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_invert_force = false;
    
    // v0.6.31: Zero out all auxiliary effects for clean physics testing by default.
    // Individual tests can re-enable what they need.
    engine.m_steering_shaft_smoothing = 0.0f; 
    engine.m_slip_angle_smoothing = 0.0f;
    engine.m_sop_smoothing_factor = 1.0f; // 1.0 = Instant/No smoothing
    engine.m_yaw_accel_smoothing = 0.0f;
    engine.m_gyro_smoothing = 0.0f;
    engine.m_chassis_inertia_smoothing = 0.0f;
    engine.m_dynamic_weight_smoothing = 0.0f;
    engine.m_grip_smoothing_steady = 0.0f;
    engine.m_grip_smoothing_fast = 0.0f;
    engine.m_grip_smoothing_sensitivity = 1.0f;
    
    engine.m_sop_effect = 0.0f;
    engine.m_sop_yaw_gain = 0.0f;
    engine.m_oversteer_boost = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_abs_pulse_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_min_force = 0.0f;
    
    // v0.6.25: Disable speed gate by default for legacy tests (avoids muting physics at 0 speed)
    engine.m_speed_gate_lower = -10.0f;
    engine.m_speed_gate_upper = -5.0f;

    // v0.7.67: Fix for Issue #152 Normalization - Ensure consistent scaling for legacy tests
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 20.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 20.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 20.0);
}

// ============================================================
// Auto-Registration Implementation
// ============================================================

// Category ordering for consistent output
static const std::vector<std::string> CATEGORY_ORDER = {
    "CorePhysics", "SlopeDetection", "Understeer", "SpeedGate",
    "YawGyro", "Coordinates", "RoadTexture", "Texture",
    "LockupBraking", "Config", "SlipGrip", "Internal",
    "Windows", "Screenshot", "Persistence", "GUI"
};

static int GetCategoryOrder(const std::string& cat) {
    auto it = std::find(CATEGORY_ORDER.begin(), CATEGORY_ORDER.end(), cat);
    if (it != CATEGORY_ORDER.end()) {
        return static_cast<int>(std::distance(CATEGORY_ORDER.begin(), it));
    }
    return 999; // Unknown categories go last
}

TestRegistry& TestRegistry::Instance() {
    static TestRegistry instance;
    return instance;
}

void TestRegistry::Register(const std::string& name, 
                            const std::string& category,
                            const std::vector<std::string>& tags,
                            std::function<void()> func,
                            int order) {
    m_tests.push_back({name, category, tags, func, order});
}

void TestRegistry::SortByCategory() {
    if (m_sorted) return;
    std::stable_sort(m_tests.begin(), m_tests.end(), 
        [](const TestEntry& a, const TestEntry& b) {
            int orderA = GetCategoryOrder(a.category);
            int orderB = GetCategoryOrder(b.category);
            if (orderA != orderB) return orderA < orderB;
            return a.order_hint < b.order_hint;
        });
    m_sorted = true;
}

const std::vector<TestEntry>& TestRegistry::GetTests() const {
    return m_tests;
}

AutoRegister::AutoRegister(const std::string& name, 
                           const std::string& category, 
                           const std::vector<std::string>& tags,
                           std::function<void()> func,
                           int order) {
    TestRegistry::Instance().Register(name, category, tags, func, order);
}

void Run() {
    std::cout << "\n--- FFTEngine Regression Suite ---" << std::endl;
    
    // Auto-Registered Tests
    auto& registry = TestRegistry::Instance();
    if (!registry.GetTests().empty()) {
        registry.SortByCategory();
        auto& tests = registry.GetTests();
        
        std::cout << "\n--- Auto-Registered Tests (" << tests.size() << ") ---" << std::endl;
        
        std::string current_category;
        for (const auto& test : tests) {
            if (test.category != current_category) {
                current_category = test.category;
                std::cout << "\n=== " << current_category << " Tests ===" << std::endl;
            }
            
            if (!ShouldRunTest(test.tags, test.category)) continue;

            try {
                int initial_fails = g_tests_failed;
                test.func();

                g_test_cases_run++;
                if (g_tests_failed > initial_fails) {
                    g_test_cases_failed++;
                } else {
                    g_test_cases_passed++;
                }
            } catch (const std::exception& e) {
                std::cout << "[FAIL] " << test.name << " threw exception: " << e.what() << std::endl;
                g_tests_failed++;
                g_test_cases_run++;
                g_test_cases_failed++;
            } catch (...) {
                std::cout << "[FAIL] " << test.name << " threw unknown exception" << std::endl;
                g_tests_failed++;
                g_test_cases_run++;
                g_test_cases_failed++;
            }
        }
    }

    std::cout << "\n--- Physics Engine Test Summary ---" << std::endl;
    std::cout << "Test Cases: " << g_test_cases_passed << "/" << g_test_cases_run << " passed" << std::endl;
    std::cout << "Assertions: " << g_tests_passed << " passed, " << g_tests_failed << " failed" << std::endl;
}

} // namespace FFBEngineTests