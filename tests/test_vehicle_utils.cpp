#include "test_ffb_common.h"
#include "VehicleUtils.h"

using namespace LMUFFB;

namespace FFBEngineTests {

TEST_CASE(test_vehicle_class_parsing_keywords, "Internal") {
    // No engine instance needed
    
    // Primary identification via class name
    ASSERT_EQ((int)Physics::ParseVehicleClass("HYPERCAR", ""), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMH", ""), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMDH", ""), (int)Physics::ParsedVehicleClass::HYPERCAR);
    
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMP2 ELMS", ""), (int)Physics::ParsedVehicleClass::LMP2_UNRESTRICTED);
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMP2", "DERESTRICTED"), (int)Physics::ParsedVehicleClass::LMP2_UNRESTRICTED);
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMP2 WEC", ""), (int)Physics::ParsedVehicleClass::LMP2_RESTRICTED);
    // Issue #225: "LMP2" without suffix in LMU refers to the restricted WEC spec
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMP2", ""), (int)Physics::ParsedVehicleClass::LMP2_RESTRICTED);

    ASSERT_EQ((int)Physics::ParseVehicleClass("LMP3", ""), (int)Physics::ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("GTE", ""), (int)Physics::ParsedVehicleClass::GTE);
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMGT3", ""), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMGT3", ""), (int)Physics::ParsedVehicleClass::LMGT3);

    // Secondary Identification via Vehicle Name Keywords
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "499P"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "GR010"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "963"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "9X8"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "V-SERIES.R"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "SCG 007"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "GLICKENHAUS"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "VANWALL"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "A424"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "SC63"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "VALKYRIE"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "M HYBRID"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "TIPO 6"), (int)Physics::ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "680"), (int)Physics::ParsedVehicleClass::HYPERCAR);

    ASSERT_EQ((int)Physics::ParseVehicleClass("", "ORECA"), (int)Physics::ParsedVehicleClass::LMP2_UNSPECIFIED);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "07"), (int)Physics::ParsedVehicleClass::LMP2_UNSPECIFIED);

    ASSERT_EQ((int)Physics::ParseVehicleClass("", "LIGIER"), (int)Physics::ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "GINETTA"), (int)Physics::ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "DUQUEINE"), (int)Physics::ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "P320"), (int)Physics::ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "P325"), (int)Physics::ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "G61"), (int)Physics::ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "D09"), (int)Physics::ParsedVehicleClass::LMP3);

    ASSERT_EQ((int)Physics::ParseVehicleClass("", "RSR-19"), (int)Physics::ParsedVehicleClass::GTE);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "488 GTE"), (int)Physics::ParsedVehicleClass::GTE);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "C8.R"), (int)Physics::ParsedVehicleClass::GTE);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "VANTAGE AMR"), (int)Physics::ParsedVehicleClass::GTE);

    ASSERT_EQ((int)Physics::ParseVehicleClass("", "LMGT3"), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "296 GT3"), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "M4 GT3"), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "Z06 GT3"), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "HURACAN"), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "RC F"), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "720S"), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("", "MUSTANG"), (int)Physics::ParsedVehicleClass::LMGT3);

    ASSERT_EQ((int)Physics::ParseVehicleClass("Random Car", ""), (int)Physics::ParsedVehicleClass::UNKNOWN);
    ASSERT_EQ((int)Physics::ParseVehicleClass(nullptr, nullptr), (int)Physics::ParsedVehicleClass::UNKNOWN);
}

TEST_CASE(test_issue_368_repro, "Internal") {
    // Case reported in Issue #368 for Porsche LMGT3 (Proton Competition)
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("LMGT3", "Proton Competition 2025 #60:ELMS"), "Porsche");

    // Manthey keyword test
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("LMGT3", "Manthey EMA #91"), "Porsche");

    // 992 keyword test
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("LMGT3", "GT3 R (992)"), "Porsche");

    // Standard Porsche name
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("LMGT3", "Porsche 911 GT3 R"), "Porsche");

    // Robust Trim (Whitespace handling)
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("", " Porsche "), "Porsche");
    ASSERT_EQ_STR(Physics::ParseVehicleBrand("", "\t911\n"), "Porsche");
}

TEST_CASE(test_vehicle_class_case_insensitivity, "Internal") {
    ASSERT_EQ((int)Physics::ParseVehicleClass("gt3", ""), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("LMGT3", ""), (int)Physics::ParsedVehicleClass::LMGT3);
    ASSERT_EQ((int)Physics::ParseVehicleClass("Gt3", ""), (int)Physics::ParsedVehicleClass::LMGT3);
}

TEST_CASE(test_vehicle_default_loads, "Internal") {
    // Check all defined classes
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::HYPERCAR), 9500.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::LMP2_UNRESTRICTED), 8500.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::LMP2_RESTRICTED), 7500.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::LMP2_UNSPECIFIED), 8000.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::LMP3), 5800.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::GTE), 5500.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::LMGT3), 5000.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::LMGT3), 5000.0);
    ASSERT_EQ(Physics::GetDefaultLoadForClass(Physics::ParsedVehicleClass::UNKNOWN), 4500.0);
}

TEST_CASE(test_vehicle_class_to_string, "Internal") {
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::HYPERCAR), "Hypercar");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::LMP2_UNRESTRICTED), "LMP2 Unrestricted");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::LMP2_RESTRICTED), "LMP2 Restricted");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::LMP2_UNSPECIFIED), "LMP2 Unspecified");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::LMP3), "LMP3");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::GTE), "GTE");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::LMGT3), "LMGT3");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::LMGT3), "LMGT3");
    ASSERT_EQ_STR(Physics::VehicleClassToString(Physics::ParsedVehicleClass::UNKNOWN), "Unknown");
}

TEST_CASE(test_motion_ratio_centralization, "Internal") {
    ASSERT_EQ(Physics::GetMotionRatioForClass(Physics::ParsedVehicleClass::HYPERCAR), 0.50);
    ASSERT_EQ(Physics::GetMotionRatioForClass(Physics::ParsedVehicleClass::LMP2_RESTRICTED), 0.50);
    ASSERT_EQ(Physics::GetMotionRatioForClass(Physics::ParsedVehicleClass::LMGT3), 0.65);
    ASSERT_EQ(Physics::GetMotionRatioForClass(Physics::ParsedVehicleClass::GTE), 0.65);
    ASSERT_EQ(Physics::GetMotionRatioForClass(Physics::ParsedVehicleClass::UNKNOWN), 0.55);
}

TEST_CASE(test_unsprung_weight_centralization, "Internal") {
    // Front
    ASSERT_EQ(Physics::GetUnsprungWeightForClass(Physics::ParsedVehicleClass::HYPERCAR, false), 400.0);
    ASSERT_EQ(Physics::GetUnsprungWeightForClass(Physics::ParsedVehicleClass::LMGT3, false), 500.0);
    ASSERT_EQ(Physics::GetUnsprungWeightForClass(Physics::ParsedVehicleClass::UNKNOWN, false), 450.0);

    // Rear
    ASSERT_EQ(Physics::GetUnsprungWeightForClass(Physics::ParsedVehicleClass::HYPERCAR, true), 450.0);
    ASSERT_EQ(Physics::GetUnsprungWeightForClass(Physics::ParsedVehicleClass::LMGT3, true), 550.0);
    ASSERT_EQ(Physics::GetUnsprungWeightForClass(Physics::ParsedVehicleClass::UNKNOWN, true), 500.0);
}

} // namespace FFBEngineTests
