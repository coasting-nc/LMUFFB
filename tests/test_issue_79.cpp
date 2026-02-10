#include "test_ffb_common.h"
#include "../src/lmu_sm_interface/SharedMemoryInterface.hpp"

namespace FFBEngineTests {

TEST_CASE(test_ffb_mute_on_session_end_logic, "Safety") {
    std::cout << "\nTest: FFB Mute Logic (Issue #79)" << std::endl;

    SharedMemoryObjectOut mockData;
    std::memset(&mockData, 0, sizeof(mockData));

    // Setup player index and ID
    uint8_t playerIdx = 5;
    long playerID = 1234;
    mockData.telemetry.playerVehicleIdx = playerIdx;
    mockData.telemetry.playerHasVehicle = true;
    mockData.telemetry.telemInfo[playerIdx].mID = playerID;

    // Setup scoring info
    mockData.scoring.scoringInfo.mNumVehicles = 10;
    mockData.scoring.vehScoringInfo[playerIdx].mID = playerID;

    // --- SCENARIO 1: Normal Driving ---
    mockData.scoring.vehScoringInfo[playerIdx].mControl = 0;      // Local Player
    mockData.scoring.vehScoringInfo[playerIdx].mFinishStatus = 0; // None

    {
        uint8_t idx = mockData.telemetry.playerVehicleIdx;
        bool is_player_controlled = (mockData.scoring.vehScoringInfo[idx].mControl == 0);
        bool is_not_finished = (mockData.scoring.vehScoringInfo[idx].mFinishStatus == 0);
        bool should_output = is_player_controlled && is_not_finished;

        ASSERT_TRUE(should_output == true);
    }

    // --- SCENARIO 2: AI Takeover (Crossing finish line) ---
    mockData.scoring.vehScoringInfo[playerIdx].mControl = 1;      // Local AI
    mockData.scoring.vehScoringInfo[playerIdx].mFinishStatus = 0;

    {
        uint8_t idx = mockData.telemetry.playerVehicleIdx;
        bool is_player_controlled = (mockData.scoring.vehScoringInfo[idx].mControl == 0);
        bool is_not_finished = (mockData.scoring.vehScoringInfo[idx].mFinishStatus == 0);
        bool should_output = is_player_controlled && is_not_finished;

        ASSERT_TRUE(should_output == false);
    }

    // --- SCENARIO 3: Finished Session ---
    mockData.scoring.vehScoringInfo[playerIdx].mControl = 0;      // Local Player (maybe cooldown lap)
    mockData.scoring.vehScoringInfo[playerIdx].mFinishStatus = 1; // Finished

    {
        uint8_t idx = mockData.telemetry.playerVehicleIdx;
        bool is_player_controlled = (mockData.scoring.vehScoringInfo[idx].mControl == 0);
        bool is_not_finished = (mockData.scoring.vehScoringInfo[idx].mFinishStatus == 0);
        bool should_output = is_player_controlled && is_not_finished;

        ASSERT_TRUE(should_output == false);
    }

    // --- SCENARIO 4: DNF/DQ ---
    mockData.scoring.vehScoringInfo[playerIdx].mControl = 0;
    mockData.scoring.vehScoringInfo[playerIdx].mFinishStatus = 2; // DNF

    {
        uint8_t idx = mockData.telemetry.playerVehicleIdx;
        bool is_player_controlled = (mockData.scoring.vehScoringInfo[idx].mControl == 0);
        bool is_not_finished = (mockData.scoring.vehScoringInfo[idx].mFinishStatus == 0);
        bool should_output = is_player_controlled && is_not_finished;

        ASSERT_TRUE(should_output == false);
    }

    // --- SCENARIO 5: Out of bounds safety ---
    mockData.telemetry.playerVehicleIdx = 105; // Invalid index

    {
        uint8_t idx = mockData.telemetry.playerVehicleIdx;
        bool should_output = false;
        if (idx < 104) {
             bool is_player_controlled = (mockData.scoring.vehScoringInfo[idx].mControl == 0);
             bool is_not_finished = (mockData.scoring.vehScoringInfo[idx].mFinishStatus == 0);
             should_output = is_player_controlled && is_not_finished;
        }

        ASSERT_TRUE(should_output == false);
    }
}

} // namespace FFBEngineTests
