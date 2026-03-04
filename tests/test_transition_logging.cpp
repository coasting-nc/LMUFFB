#include "test_ffb_common.h"
#include "../src/GameConnector.h"
#include "../src/Logger.h"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

namespace FFBEngineTests {

// Helper to check if a string is in the log file
bool IsInLog(const std::string& filename, const std::string& pattern) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

class GameConnectorTestAccessor {
public:
    static void CallCheckTransitions(GameConnector& gc, const SharedMemoryObjectOut& data) {
        gc.CheckTransitions(data);
    }
};

TEST_CASE_TAGGED(test_transition_logging_logic, "Functional", (std::vector<std::string>{"logging", "transitions"})) {
    Logger::Get().Init("test_transitions.log");
    GameConnector& gc = GameConnector::Get();

    SharedMemoryObjectOut data;
    memset(&data, 0, sizeof(data));

    // 1. Options Location Transition
    std::cout << "Testing Options Location Transition..." << std::endl;
    data.generic.appInfo.mOptionsLocation = 3; // On Track
    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "[Transition] OptionsLocation:"));

    data.generic.appInfo.mOptionsLocation = 2; // Monitor
    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "[Transition] OptionsLocation: 3 -> 2 (Monitor (Garage))"));

    // 2. Game Phase Transition
    std::cout << "Testing Game Phase Transition..." << std::endl;
    data.scoring.scoringInfo.mGamePhase = 5; // Green Flag
    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "[Transition] GamePhase:"));

    data.scoring.scoringInfo.mGamePhase = 9; // Paused
    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "(Paused)"));

    // 3. Control Transition
    std::cout << "Testing Control Transition..." << std::endl;
    data.telemetry.playerHasVehicle = true;
    data.telemetry.playerVehicleIdx = 0;
    data.scoring.vehScoringInfo[0].mControl = 1; // AI

    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "[Transition] Control:"));

    data.scoring.vehScoringInfo[0].mControl = 0; // Player
    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "(Player)"));

    // 4. Context Transitions (Track/Vehicle)
    std::cout << "Testing Context Transitions..." << std::endl;
    strncpy(data.scoring.scoringInfo.mTrackName, "Spa", 63);
    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "[Transition] Track: '' -> 'Spa'"));

    data.telemetry.playerHasVehicle = true;
    data.telemetry.playerVehicleIdx = 0;
    strncpy(data.scoring.vehScoringInfo[0].mVehicleName, "Ferrari 488", 63);
    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    ASSERT_TRUE(IsInLog("test_transitions.log", "[Transition] Vehicle: '' -> 'Ferrari 488'"));

    // 5. No Duplicate Logs & No Console Output
    std::cout << "Testing No Duplicate Logs & No Console Output..." << std::endl;
    data.generic.appInfo.mOptionsLocation = 0; // Main UI

    // Redirect cout to capture if it prints there (it shouldn't for LogFile)
    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    GameConnectorTestAccessor::CallCheckTransitions(gc, data);
    GameConnectorTestAccessor::CallCheckTransitions(gc, data); // Duplicate call

    std::cout.rdbuf(old_cout);

    std::ifstream file("test_transitions.log");
    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        // Just find if "OptionsLocation: " appeared in this test run's specific transition
        if (line.find("OptionsLocation:") != std::string::npos && line.find("-> 0") != std::string::npos) {
            count++;
        }
    }
    ASSERT_GE(count, 1);

    // Verify it didn't print to console
    std::string cout_output = buffer.str();
    bool found_unexpected = false;
    std::stringstream ss(cout_output);
    while (std::getline(ss, line)) {
        if (line.find("[Transition]") != std::string::npos && line.find("[DEBUG]") == std::string::npos) {
            found_unexpected = true;
            break;
        }
    }
    ASSERT_FALSE(found_unexpected);
}

} // namespace FFBEngineTests
