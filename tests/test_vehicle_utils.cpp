#include "test_ffb_common.h"
#include "VehicleUtils.h"

namespace FFBEngineTests {

TEST_CASE(test_vehicle_class_parsing_keywords, "Internal") {
    // No engine instance needed
    
    // Primary identification via class name
    ASSERT_EQ((int)ParseVehicleClass("HYPERCAR", ""), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("LMH", ""), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("LMDH", ""), (int)ParsedVehicleClass::HYPERCAR);
    
    ASSERT_EQ((int)ParseVehicleClass("LMP2 ELMS", ""), (int)ParsedVehicleClass::LMP2_UNRESTRICTED);
    ASSERT_EQ((int)ParseVehicleClass("LMP2", "DERESTRICTED"), (int)ParsedVehicleClass::LMP2_UNRESTRICTED);
    ASSERT_EQ((int)ParseVehicleClass("LMP2 WEC", ""), (int)ParsedVehicleClass::LMP2_RESTRICTED);
    ASSERT_EQ((int)ParseVehicleClass("LMP2", ""), (int)ParsedVehicleClass::LMP2_UNSPECIFIED);

    ASSERT_EQ((int)ParseVehicleClass("LMP3", ""), (int)ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)ParseVehicleClass("GTE", ""), (int)ParsedVehicleClass::GTE);
    ASSERT_EQ((int)ParseVehicleClass("GT3", ""), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("LMGT3", ""), (int)ParsedVehicleClass::GT3);

    // Secondary Identification via Vehicle Name Keywords
    ASSERT_EQ((int)ParseVehicleClass("", "499P"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "GR010"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "963"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "9X8"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "V-SERIES.R"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "SCG 007"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "GLICKENHAUS"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "VANWALL"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "A424"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "SC63"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "VALKYRIE"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "M HYBRID"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "TIPO 6"), (int)ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ((int)ParseVehicleClass("", "680"), (int)ParsedVehicleClass::HYPERCAR);

    ASSERT_EQ((int)ParseVehicleClass("", "ORECA"), (int)ParsedVehicleClass::LMP2_UNSPECIFIED);
    ASSERT_EQ((int)ParseVehicleClass("", "07"), (int)ParsedVehicleClass::LMP2_UNSPECIFIED);

    ASSERT_EQ((int)ParseVehicleClass("", "LIGIER"), (int)ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)ParseVehicleClass("", "GINETTA"), (int)ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)ParseVehicleClass("", "DUQUEINE"), (int)ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)ParseVehicleClass("", "P320"), (int)ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)ParseVehicleClass("", "P325"), (int)ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)ParseVehicleClass("", "G61"), (int)ParsedVehicleClass::LMP3);
    ASSERT_EQ((int)ParseVehicleClass("", "D09"), (int)ParsedVehicleClass::LMP3);

    ASSERT_EQ((int)ParseVehicleClass("", "RSR-19"), (int)ParsedVehicleClass::GTE);
    ASSERT_EQ((int)ParseVehicleClass("", "488 GTE"), (int)ParsedVehicleClass::GTE);
    ASSERT_EQ((int)ParseVehicleClass("", "C8.R"), (int)ParsedVehicleClass::GTE);
    ASSERT_EQ((int)ParseVehicleClass("", "VANTAGE AMR"), (int)ParsedVehicleClass::GTE);

    ASSERT_EQ((int)ParseVehicleClass("", "LMGT3"), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("", "296 GT3"), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("", "M4 GT3"), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("", "Z06 GT3"), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("", "HURACAN"), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("", "RC F"), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("", "720S"), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("", "MUSTANG"), (int)ParsedVehicleClass::GT3);

    ASSERT_EQ((int)ParseVehicleClass("Random Car", ""), (int)ParsedVehicleClass::UNKNOWN);
    ASSERT_EQ((int)ParseVehicleClass(nullptr, nullptr), (int)ParsedVehicleClass::UNKNOWN);
}

TEST_CASE(test_vehicle_class_case_insensitivity, "Internal") {
    ASSERT_EQ((int)ParseVehicleClass("gt3", ""), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("GT3", ""), (int)ParsedVehicleClass::GT3);
    ASSERT_EQ((int)ParseVehicleClass("Gt3", ""), (int)ParsedVehicleClass::GT3);
}

TEST_CASE(test_vehicle_default_loads, "Internal") {
    // Check all defined classes
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::HYPERCAR), 9500.0);
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::LMP2_UNRESTRICTED), 8500.0);
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::LMP2_RESTRICTED), 7500.0);
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::LMP2_UNSPECIFIED), 8000.0);
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::LMP3), 5800.0);
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::GTE), 5500.0);
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::GT3), 4800.0);
    ASSERT_EQ(GetDefaultLoadForClass(ParsedVehicleClass::UNKNOWN), 4500.0);
}

TEST_CASE(test_vehicle_class_to_string, "Internal") {
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::HYPERCAR), "Hypercar");
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::LMP2_UNRESTRICTED), "LMP2 Unrestricted");
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::LMP2_RESTRICTED), "LMP2 Restricted");
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::LMP2_UNSPECIFIED), "LMP2 Unspecified");
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::LMP3), "LMP3");
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::GTE), "GTE");
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::GT3), "GT3");
    ASSERT_EQ_STR(VehicleClassToString(ParsedVehicleClass::UNKNOWN), "Unknown");
}

} // namespace FFBEngineTests
