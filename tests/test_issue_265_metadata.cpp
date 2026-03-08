#include "test_ffb_common.h"
#include "../src/FFBEngine.h"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include <cstring>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_issue_265_metadata_sync, "Functional", (std::vector<std::string>{"logging", "metadata"})) {
    std::cout << "Running test_issue_265_metadata_sync..." << std::endl;
    FFBEngine engine;
    SharedMemoryObjectOut data;
    memset(&data, 0, sizeof(data));

    // Setup initial state: Player has a vehicle
    data.telemetry.playerHasVehicle = true;
    data.telemetry.playerVehicleIdx = 0;
    StringUtils::SafeCopy(data.scoring.vehScoringInfo[0].mVehicleName, sizeof(data.scoring.vehScoringInfo[0].mVehicleName), "Ferrari 499P");
    StringUtils::SafeCopy(data.scoring.vehScoringInfo[0].mVehicleClass, sizeof(data.scoring.vehScoringInfo[0].mVehicleClass), "Hypercar");
    StringUtils::SafeCopy(data.scoring.scoringInfo.mTrackName, sizeof(data.scoring.scoringInfo.mTrackName), "Le Mans");

    // Call UpdateMetadata
    engine.UpdateMetadata(data);

    // Verify engine state
    ASSERT_EQ_STR(engine.m_vehicle_name, "Ferrari 499P");
    ASSERT_EQ_STR(engine.m_track_name, "Le Mans");
    ASSERT_EQ_STR(engine.m_current_class_name, "Hypercar");

    // Change vehicle
    StringUtils::SafeCopy(data.scoring.vehScoringInfo[0].mVehicleName, sizeof(data.scoring.vehScoringInfo[0].mVehicleName), "Porsche 963");
    StringUtils::SafeCopy(data.scoring.vehScoringInfo[0].mVehicleClass, sizeof(data.scoring.vehScoringInfo[0].mVehicleClass), "Hypercar");
    StringUtils::SafeCopy(data.scoring.scoringInfo.mTrackName, sizeof(data.scoring.scoringInfo.mTrackName), "Spa");

    engine.UpdateMetadata(data);

    ASSERT_EQ_STR(engine.m_vehicle_name, "Porsche 963");
    ASSERT_EQ_STR(engine.m_track_name, "Spa");
    ASSERT_EQ_STR(engine.m_current_class_name, "Hypercar");

    // Test case where player doesn't have a vehicle yet (e.g. joining session)
    data.telemetry.playerHasVehicle = false;
    StringUtils::SafeCopy(data.scoring.scoringInfo.mTrackName, sizeof(data.scoring.scoringInfo.mTrackName), "Monza");

    engine.UpdateMetadata(data);

    // Track should update, vehicle should remain what it was (or stay "Porsche 963")
    ASSERT_EQ_STR(engine.m_track_name, "Monza");
    ASSERT_EQ_STR(engine.m_vehicle_name, "Porsche 963");
}

} // namespace FFBEngineTests
