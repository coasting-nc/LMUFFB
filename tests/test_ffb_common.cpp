#include "test_ffb_common.h"
#include "../src/io/GameConnector.h"

namespace FFBEngineTests {

// --- Global Test Counters ---
int g_tests_passed = 0;
int g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO = 0;
int g_test_cases_run = 0;
int g_test_cases_passed = 0;
int g_test_cases_failed = 0;
std::vector<TestDuration> g_test_durations;
std::vector<std::string> g_failure_log; // New: collect failure messages
std::string g_current_test_name; // Tracks the currently-running test for assertion messages

// --- Tag Filtering Globals ---
std::vector<std::string> g_tag_filter;
std::vector<std::string> g_name_filter;
std::vector<std::string> g_tag_exclude;
std::vector<std::string> g_category_filter;
bool g_enable_tag_filtering = false;

// --- Helper: Parse Command Line Arguments ---
void ParseTagArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        // --filter=test_name
        if (arg.find("--filter=") == 0) {
            g_enable_tag_filtering = true;
            std::string filter = arg.substr(9);
            g_name_filter.push_back(filter);
        }
        // --tag=Physics,Math
        else if (arg.find("--tag=") == 0) {
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
    data.mDeltaTime = 0.01f;
    data.mElapsedTime = 1.0f;
    
    // Wheel setup (all 4 wheels)
    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mGripFract = 0.0; // Trigger approximation mode
        data.mWheel[i].mTireLoad = 4000.0; // Realistic load
        data.mWheel[i].mStaticUndeflectedRadius = 30; // 0.3m radius
        data.mWheel[i].mRotation = speed * 3.33f; // Match speed (rad/s)
        data.mWheel[i].mLongitudinalGroundVel = speed;
        data.mWheel[i].mLateralPatchVel = slip_angle * speed; // Convert to m/s
        data.mWheel[i].mBrakePressure = 1.0; // Default for tests (v0.6.0)
        data.mWheel[i].mSuspForce = 4000.0; // Spring force
        data.mWheel[i].mTireLoad = 4000.0;  // Direct tire load (v0.7.165 fix)
        data.mWheel[i].mVerticalTireDeflection = 0.001; // Avoid "missing data" warning (v0.6.21)
    }
    
    return data;
}

// --- Helper: Initialize Engine with Test Defaults ---
void InitializeEngine(FFBEngine& engine) {
    std::cout << "--- InitializeEngine Called ---" << std::endl;
    Preset::ApplyDefaultsToEngine(engine);
    // v0.5.12: Force consistent baseline for legacy tests
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;
    engine.m_invert_force = false;
    
    // v0.6.31: Zero out all auxiliary effects for clean physics testing by default.
    // Individual tests can re-enable what they need.
    engine.m_front_axle.steering_shaft_smoothing = 0.0f;
    engine.m_grip_estimation.slip_angle_smoothing = 0.0f;
    engine.m_rear_axle.sop_smoothing_factor = 0.0f; // 0.0 = Instant/No smoothing (v0.7.147)
    engine.m_rear_axle.yaw_accel_smoothing = 0.0f;
    engine.m_gyro_smoothing = 0.0f;
    engine.m_grip_estimation.chassis_inertia_smoothing = 0.0f;
    engine.m_load_forces.long_load_smoothing = 0.0f;
    engine.m_grip_estimation.grip_smoothing_steady = 0.0f;
    engine.m_grip_estimation.grip_smoothing_fast = 0.0f;
    engine.m_grip_estimation.grip_smoothing_sensitivity = 1.0f;
    
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_load_forces.lat_load_effect = 0.0f; // New v0.7.121
    engine.m_rear_axle.sop_yaw_gain = 0.0f;
    engine.m_rear_axle.oversteer_boost = 0.0f;
    engine.m_rear_axle.rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_abs_pulse_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_general.min_force = 0.0f;
    
    // v0.6.25: Disable speed gate by default for legacy tests (avoids muting physics at 0 speed)
    engine.m_speed_gate_lower = -10.0f;
    engine.m_speed_gate_upper = -5.0f;

    // v0.7.67: Fix for Issue #152 Normalization - Ensure consistent scaling for legacy tests
    // v0.7.109: Initialize structural peaks to match target
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 20.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 20.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 20.0);

    // v0.7.109: Ensure toggles are initialized to FALSE to match global defaults
    engine.m_general.dynamic_normalization_enabled = false;
    engine.m_general.auto_load_normalization_enabled = false;
    // v0.7.147: Initialize static load reference to match CreateBasicTestTelemetry default
    engine.m_static_front_load = 4000.0f;

    // Issue #379: Pre-seed derivatives for legacy tests to avoid first-frame zero force
    FFBEngineTestAccess::SetDerivativesSeeded(engine, true);

    // Issue #397: Seed the DSP filters to prevent false "Missing Telemetry" warnings
    TelemInfoV01 seed_data = CreateBasicTestTelemetry(20.0, 0.0);
    // Explicitly set 1.0 grip to prevent friction circle from dropping on first frame
    for(int i=0; i<4; ++i) seed_data.mWheel[i].mGripFract = 1.0;

    // Seeding mechanism (m-1 review note):
    // SetWasAllowed(false) forces the engine to see a false→true transition on the
    // very next calculate_force call (because InitializeEngine passes allowed=true by
    // default). That transition triggers an internal Reset() that initialises all
    // interpolators and diagnostic timers — avoiding stale state from previous tests.
    // After the seed call we restore m_was_allowed to true so real tests start with
    // a clean "already allowed" state and don't trigger an unwanted second reset.
    FFBEngineTestAccess::SetWasAllowed(engine, false); // Force a state transition reset
    engine.calculate_force(&seed_data); // First dummy frame seeds interpolators
    FFBEngineTestAccess::SetWasAllowed(engine, true);
}

// Helper to simulate the passage of time for the FFB Engine's DSP pipeline.
double PumpEngineTime(FFBEngine& engine, TelemInfoV01& data, double time_to_advance_s) {
    double dt = 0.0025; // 400Hz FFB loop tick
    int ticks = (int)std::ceil(time_to_advance_s / dt);
    if (ticks < 1) ticks = 1;
    double last_force = 0.0;

    for (int i = 0; i < ticks; i++) {
        // Only advance the telemetry timestamp every 10ms (100Hz)
        // to accurately simulate how the game feeds data to the app.
        // Also update data.mDeltaTime to match game tick
        if (i % 4 == 0) {
            data.mElapsedTime += 0.01;
            data.mDeltaTime = 0.01;
        }
        last_force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, dt);
    }
    return last_force;
}

void PumpEngineSteadyState(FFBEngine& engine, TelemInfoV01& data) {
    // Run for 3 seconds to ensure filters (LPF, HW, SG) settle fully,
    // especially the slope decay which is slow (rate=2.0, τ ~0.5s).
    //
    // TODO (m-2 review note): Consider adding a faster variant (e.g. 0.5s) for tests
    // that only need the 10ms interpolator ramp or fast LPF to settle, and do NOT
    // involve the slope detection decay. At 400Hz, 3s = 1200 calculate_force calls
    // per PumpEngineSteadyState invocation, which can noticeably slow CI when called
    // multiple times per test (e.g. test_gyro_damping calls it 3 times).
    PumpEngineTime(engine, data, 3.0);
}

// --- Friend Access for Testing ---
void GameConnectorTestAccessor::Reset(::GameConnector& gc) {
    std::lock_guard<std::recursive_mutex> lock(gc.m_mutex);
    gc._DisconnectLocked();
    gc.m_sessionActive.store(false);
    gc.m_inRealtime.store(false);
    gc.m_currentSessionType.store(-1);
    gc.m_currentGamePhase.store(255);
    gc.m_playerControl.store(-2);
    gc.m_pendingMenuCheck = false;
    memset(&gc.m_prevState, 0, sizeof(gc.m_prevState));
    gc.m_prevState.optionsLocation = 255;
    gc.m_prevState.gamePhase = 255;
    gc.m_prevState.session = -1;
    gc.m_prevState.control = -2;
    gc.m_prevState.pitState = 255;
    gc.m_prevState.steeringRange = -1.0f;
    gc.m_prevState.numVehicles = -1;
}

void GameConnectorTestAccessor::SetSharedMem(::GameConnector& gc, SharedMemoryLayout* layout) {
    std::lock_guard<std::recursive_mutex> lock(gc.m_mutex);
    gc.m_pSharedMemLayout = layout;
    gc.m_connected = true;
}

void GameConnectorTestAccessor::SetSessionActive(::GameConnector& gc, bool val) { gc.m_sessionActive.store(val); }
void GameConnectorTestAccessor::SetInRealtime(::GameConnector& gc, bool val) { gc.m_inRealtime.store(val); }
void GameConnectorTestAccessor::SetSessionType(::GameConnector& gc, long val) { gc.m_currentSessionType.store(val); }
void GameConnectorTestAccessor::SetGamePhase(::GameConnector& gc, unsigned char val) { gc.m_currentGamePhase.store(val); }
void GameConnectorTestAccessor::SetPlayerControl(::GameConnector& gc, signed char val) { gc.m_playerControl.store(val); }

void GameConnectorTestAccessor::InjectTransitions(::GameConnector& gc, const SharedMemoryObjectOut& data) {
    gc.CheckTransitions(data);
}

// Orientation Matrix Helper Implementation
void VerifyOrientation(FFBEngine& engine, const OrientationScenario& scenario, float expected_sop_sign, float expected_total_ffb_sign) {
    std::cout << "  [Matrix] Testing Orientation: " << scenario.description << std::endl;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = scenario.lat_accel_x;
    data.mWheel[0].mTireLoad = scenario.fl_load;
    data.mWheel[1].mTireLoad = scenario.fr_load;
    for (int i = 0; i < 60; i++) engine.calculate_force(&data);
    auto snapshots = engine.GetDebugBatch();
    if (snapshots.empty()) { FAIL_TEST("No snapshots available in VerifyOrientation"); return; }
    const auto& snap = snapshots.back();
    bool sop_ok = (expected_sop_sign > 0) ? (snap.sop_force > 0.001f) : (snap.sop_force < -0.001f);
    if (sop_ok) g_tests_passed++; else { FAIL_TEST("SoP orientation mismatch in scenario: " << scenario.description); }
    bool total_ok = (expected_total_ffb_sign > 0) ? (snap.total_output > 0.001f) : (snap.total_output < -0.001f);
    if (total_ok) g_tests_passed++; else { FAIL_TEST("Total FFB orientation mismatch in scenario: " << scenario.description); }
}

bool IsInLog(const std::string& filename, const std::string& pattern) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

int CountInLog(const std::string& filename, const std::string& pattern) {
    std::ifstream file(filename);
    if (!file.is_open()) return 0;
    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        if (line.find(pattern) != std::string::npos) {
            count++;
        }
    }
    return count;
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
    std::vector<std::string> failed_test_names;

    if (!registry.GetTests().empty()) {
        registry.SortByCategory();
        auto& tests = registry.GetTests();
        
        std::cout << "\n--- Auto-Registered Tests (" << tests.size() << ") ---" << std::endl;
        
        std::string current_category;
        for (const auto& test : tests) {
            if (!ShouldRunTest(test.name, test.tags, test.category)) continue;

            if (test.category != current_category) {
                current_category = test.category;
                std::cout << "\n=== " << current_category << " Tests ===" << std::endl;
            }
            
            try {
                int initial_fails = g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO;
                g_current_test_name = test.name; // Make test name available to ASSERT macros

                auto start = std::chrono::high_resolution_clock::now();
                test.func();
                auto end = std::chrono::high_resolution_clock::now();
                double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
                g_test_durations.push_back({test.name, duration_ms});

                g_test_cases_run++;
                if (g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO > initial_fails) {
                    g_test_cases_failed++;
                    failed_test_names.push_back(test.name);
                    FAIL_TEST(test.name << " ("
                              << (g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO - initial_fails) << " assertion(s) failed)");
                } else {
                    g_test_cases_passed++;
                }
            } catch (const std::exception& e) {
                FAIL_TEST(test.name << " threw exception: " << e.what());
                g_test_cases_run++;
                g_test_cases_failed++;
                failed_test_names.push_back(test.name);
            } catch (...) {
                FAIL_TEST(test.name << " threw unknown exception");
                g_test_cases_run++;
                g_test_cases_failed++;
                failed_test_names.push_back(test.name);
            }
        }
    }

    std::cout << "\n--- Physics Engine Test Summary ---" << std::endl;
    std::cout << "Test Cases: " << g_test_cases_passed << "/" << g_test_cases_run << " passed" << std::endl;
    std::cout << "Assertions: " << g_tests_passed << " passed, " << g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO << " failed" << std::endl;

    if (!failed_test_names.empty()) {
        std::cout << "\n!!! list of FAILED test cases !!!" << std::endl;
        for (const auto& name : failed_test_names) {
            std::cout << "  - " << name << std::endl;
        }
        std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;

        if (!g_failure_log.empty()) {
            std::cout << "\n=== DETAILED FAILURE LOG ===" << std::endl;
            for (const auto& log : g_failure_log) {
                std::cout << log << std::endl;
            }
            std::cout << "============================" << std::endl;
        }
    }
}

} // namespace FFBEngineTests
