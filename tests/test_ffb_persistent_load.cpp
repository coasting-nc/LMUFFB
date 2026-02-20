#include "test_ffb_common.h"
#include "Config.h"
#include "FFBEngine.h"
#include <fstream>
#include <cstdio>

using namespace FFBEngineTests;

TEST_CASE(Config_StaticLoadParsing, "PersistentLoad") {
    // 1. Create a mock config file with [StaticLoads] section
    const char* test_ini = "test_static_loads.ini";
    {
        std::ofstream file(test_ini);
        file << "[StaticLoads]\n";
        file << "Ferrari 488 GTE=4200.5\n";
        file << "Porsche 911 RSR=4100.0\n";
        file << "\n[Presets]\n";
    }

    // 2. Load the config
    FFBEngine engine;
    Config::m_saved_static_loads.clear();
    Config::Load(engine, test_ini);

    // 3. Verify map content using thread-safe getter
    double val1 = 0.0, val2 = 0.0;
    ASSERT_TRUE(Config::GetSavedStaticLoad("Ferrari 488 GTE", val1));
    ASSERT_NEAR(val1, 4200.5, 0.01);
    ASSERT_TRUE(Config::GetSavedStaticLoad("Porsche 911 RSR", val2));
    ASSERT_NEAR(val2, 4100.0, 0.01);

    std::remove(test_ini);
}

TEST_CASE(Engine_UsesSavedStaticLoad, "PersistentLoad") {
    FFBEngine engine;
    // Manual injection using thread-safe setter
    Config::SetSavedStaticLoad("Porsche 911 RSR", 4100.0);

    // Initialize with a car that has a saved load
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "GTE", "Porsche 911 RSR");

    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 4100.0, 0.01);
    ASSERT_TRUE(FFBEngineTestAccess::GetStaticLoadLatched(engine));
}

TEST_CASE(Engine_SavesNewStaticLoadUponLatching, "PersistentLoad") {
    FFBEngine engine;
    Config::m_saved_static_loads.clear();
    Config::m_needs_save = false;

    // Initialize with a car that has NO saved load
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "LMP2", "Oreca 07");
    ASSERT_FALSE(FFBEngineTestAccess::GetStaticLoadLatched(engine));

    strncpy(engine.m_vehicle_name, "Oreca 07", 63);
    engine.m_vehicle_name[63] = '\0';

    // Simulate reaching latch speed (15.0 m/s) with valid load
    // First, provide some samples to avoid safety fallback to 0.5 * auto_peak
    for (int i = 0; i < 100; ++i) {
        FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 3800.0, 10.0, 0.0025);
    }

    // Now latch it
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 3800.0, 20.0, 0.0025);

    ASSERT_TRUE(FFBEngineTestAccess::GetStaticLoadLatched(engine));
    double saved_val = 0.0;
    ASSERT_TRUE(Config::GetSavedStaticLoad("Oreca 07", saved_val));
    ASSERT_NEAR(saved_val, FFBEngineTestAccess::GetStaticFrontLoad(engine), 1.0);
    ASSERT_TRUE(Config::m_needs_save.load());
}
