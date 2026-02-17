#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_vehicle_class_parsing_keywords, "Internal") {
    FFBEngine engine;
    
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "LMP2 ELMS", ""), (int)FFBEngine::ParsedVehicleClass::LMP2_UNRESTRICTED);
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "GTE Pro", ""), (int)FFBEngine::ParsedVehicleClass::GTE);
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "GT3 Gen 2", ""), (int)FFBEngine::ParsedVehicleClass::GT3);
    
    // Explicit requested branches for coverage:
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "LMP2 WEC", ""), (int)FFBEngine::ParsedVehicleClass::LMP2_RESTRICTED); // Line 730
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "LMP2", ""), (int)FFBEngine::ParsedVehicleClass::LMP2_UNSPECIFIED); // Line 733
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "HYPERCAR", ""), (int)FFBEngine::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "", "488 GTE"), (int)FFBEngine::ParsedVehicleClass::GTE); // Line 769
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "", "M4 GT3"), (int)FFBEngine::ParsedVehicleClass::GT3); // Line 777

    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "Random Car", ""), (int)FFBEngine::ParsedVehicleClass::UNKNOWN);
}

TEST_CASE(test_vehicle_class_case_insensitivity, "Internal") {
    FFBEngine engine;
    
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "gt3", ""), (int)FFBEngine::ParsedVehicleClass::GT3);
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "GT3", ""), (int)FFBEngine::ParsedVehicleClass::GT3);
    ASSERT_EQ((int)FFBEngineTestAccess::CallParseVehicleClass(engine, "Gt3", ""), (int)FFBEngine::ParsedVehicleClass::GT3);
}

TEST_CASE(test_vehicle_default_loads, "Internal") {
    FFBEngine engine;
    
    // Check that all defined classes have a reasonable default load (> 3000N)
    for (int i = 0; i <= (int)FFBEngine::ParsedVehicleClass::GT3; i++) {
        double load = FFBEngineTestAccess::CallGetDefaultLoadForClass(engine, (FFBEngine::ParsedVehicleClass)i);
        ASSERT_GE(load, 4000.0);
    }
}

} // namespace FFBEngineTests
