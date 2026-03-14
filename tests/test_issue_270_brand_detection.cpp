#include "test_ffb_common.h"
#include "../src/physics/VehicleUtils.h"
#include <iostream>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_issue_270_brand_detection, "Functional", (std::vector<std::string>{"metadata", "issue270"})) {
    std::cout << "Running test_issue_270_brand_detection..." << std::endl;

    // LMP2 Cases
    ASSERT_EQ_STR(ParseVehicleBrand("LMP2", "Oreca 07"), "Oreca"); 
    
    // Test LMP2 fallback for non-keyword vehicle name
    ASSERT_EQ_STR(ParseVehicleBrand("LMP2", "Gibson GK428"), "Oreca"); 
    ASSERT_EQ_STR(ParseVehicleBrand("LMP2 (WEC Restricted)", "Car #22"), "Oreca");

    // LMP3 Cases
    ASSERT_EQ_STR(ParseVehicleBrand("LMP3", "Ligier JS P325"), "Ligier"); 
    ASSERT_EQ_STR(ParseVehicleBrand("LMP3", "Duqueine D09"), "Duqueine"); 
    ASSERT_EQ_STR(ParseVehicleBrand("LMP3", "Ginetta G61-LT-P3 Evo"), "Ginetta"); 
    
    // Test LMP3 fallbacks
    // If the class is just "LMP3", it's currently Unknown as we don't know which one.
    ASSERT_EQ_STR(ParseVehicleBrand("LMP3", "Some Generic P3"), "Unknown"); 
    // if (cls.find("GINETTA") != std::string::npos) return "Ginetta";
    // if (cls.find("LIGIER") != std::string::npos) return "Ligier";
    // if (cls.find("DUQUEINE") != std::string::npos) return "Duqueine";
    
    // Wait, if cls is just "LMP3", it won't match "LIGIER" unless "LIGIER" is in the class name.
    // LMU often has class names like "LMP3 (Ligier)" or similar.
    
    ASSERT_EQ_STR(ParseVehicleBrand("LMP3 (Ligier)", "Car #1"), "Ligier");
    ASSERT_EQ_STR(ParseVehicleBrand("LMP3 (Duqueine)", "Car #2"), "Duqueine");
    ASSERT_EQ_STR(ParseVehicleBrand("LMP3 (Ginetta)", "Car #3"), "Ginetta");

    // Hypercar verify
    ASSERT_EQ_STR(ParseVehicleBrand("Hypercar", "Ferrari 499P"), "Ferrari");
}

} // namespace FFBEngineTests
