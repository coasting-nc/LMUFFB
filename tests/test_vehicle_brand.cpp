#include "test_ffb_common.h"
#include "physics/VehicleUtils.h"

namespace FFBEngineTests {

TEST_CASE(test_vehicle_brand_parsing, "Internal") {
    // Test ParseVehicleBrand which replaced RestApiProvider manufacturer detection

    // Ferrari
    ASSERT_EQ_STR(ParseVehicleBrand("GTE", "Ferrari 488 GTE Evo"), "Ferrari");
    ASSERT_EQ_STR(ParseVehicleBrand("HYPERCAR", "499P #50"), "Ferrari");

    // Porsche
    ASSERT_EQ_STR(ParseVehicleBrand("LMGT3", "911 GT3 R (992)"), "Porsche");
    ASSERT_EQ_STR(ParseVehicleBrand("LMGT3", "Manthey EMA #91"), "Porsche");
    ASSERT_EQ_STR(ParseVehicleBrand("LMGT3", "Proton Competition #60"), "Porsche");

    // Toyota
    ASSERT_EQ_STR(ParseVehicleBrand("HYPERCAR", "GR010 HYBRID"), "Toyota");

    // Cadillac
    ASSERT_EQ_STR(ParseVehicleBrand("HYPERCAR", "Cadillac V-Series.R"), "Cadillac");

    // Unknown
    ASSERT_EQ_STR(ParseVehicleBrand("UNKNOWN", "Generic Race Car"), "Unknown");

    // Class-based fallback
    ASSERT_EQ_STR(ParseVehicleBrand("LMP2", "Oreca 07"), "Oreca");
}

}
