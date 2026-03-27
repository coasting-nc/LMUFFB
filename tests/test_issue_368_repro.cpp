#include "test_ffb_common.h"
#include "physics/VehicleUtils.h"
#include <iostream>

namespace FFBEngineTests {

TEST_CASE(test_issue_368_porsche_lmgt3_detection, "Internal") {
    // String from user log: "Proton Competition 2025 #60:ELMS"
    const char* className = "GT3";
    const char* vehicleName = "Proton Competition 2025 #60:ELMS";

    const char* brand = Physics::ParseVehicleBrand(className, vehicleName);
    std::cout << "Detected brand for '" << vehicleName << "': " << brand << std::endl;
    ASSERT_EQ_STR(brand, "Porsche");

    // Manthey keyword test
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("GT3", "Manthey EMA #91"), "Porsche");

    // 992 keyword test
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("LMGT3", "GT3 R (992)"), "Porsche");

    // Standard Porsche name
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("GT3", "Porsche 911 GT3 R"), "Porsche");
}

TEST_CASE(test_issue_368_whitespace_robustness, "Internal") {
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("", " Porsche "), "Porsche");
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("", "\t911\n"), "Porsche");
}

}
