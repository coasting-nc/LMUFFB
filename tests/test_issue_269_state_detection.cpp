#include "test_ffb_common.h"
#include "../src/io/GameConnector.h"
#include "../src/ffb/FFBEngine.h"
#include "../src/io/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include <cstring>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_issue_269_control_state_detection, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));

    layout.data.telemetry.playerHasVehicle = true;
    layout.data.telemetry.playerVehicleIdx = 0;
    layout.data.scoring.vehScoringInfo[0].mControl = 1; // AI

    GameConnectorTestAccessor::SetSharedMem(gc, &layout);

    // Manual trigger of initialization (normally in TryConnect)
    if (layout.data.telemetry.playerHasVehicle) {
        uint8_t idx = layout.data.telemetry.playerVehicleIdx;
        if (idx < 104) {
            GameConnectorTestAccessor::SetPlayerControl(gc, layout.data.scoring.vehScoringInfo[idx].mControl);
        }
    }

    signed char ctrl = gc.GetPlayerControl();
    ASSERT_EQ((int)ctrl, 1);

    // Transition to Player
    layout.data.scoring.vehScoringInfo[0].mControl = 0; // Player
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);
    ctrl = gc.GetPlayerControl();
    ASSERT_EQ((int)ctrl, 0);

    // Transition to Nobody
    layout.data.scoring.vehScoringInfo[0].mControl = -1;
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);
    ctrl = gc.GetPlayerControl();
    ASSERT_EQ((int)ctrl, -1);
}

TEST_CASE_TAGGED(test_issue_269_soft_lock_preservation, "Functional", (std::vector<std::string>{"ffb_logic"})) {
    FFBEngine engine;
    TelemInfoV01 telem;
    memset(&telem, 0, sizeof(telem));

    telem.mPhysicalSteeringWheelRange = 900.0f * (3.14159f / 180.0f);
    telem.mUnfilteredSteering = 1.1; // Beyond soft lock
    telem.mDeltaTime = 0.0025;

    // Test 1: Allowed = true
    double force_allowed = engine.calculate_force(&telem, "GT3", "Ferrari", 0.0f, true);
    ASSERT_NE(force_allowed, 0.0);

    // Test 2: Allowed = false (e.g. in menu or AI control)
    // We expect physics-based FFB to be 0, but Soft Lock force to be present.
    double force_muted = engine.calculate_force(&telem, "GT3", "Ferrari", 0.0f, false);

    // Soft lock force should be negative if steering is positive
    ASSERT_NE(force_muted, 0.0);
    // Invert force is true by default in engine, so if steering is 1.1, soft lock is positive Nm,
    // but calculation depends on wheelbase_max_nm etc.
    // FFBEngine::calculate_soft_lock sets ctx.soft_lock_force.
    // If allowed=false, output_force is zeroed, but ctx.soft_lock_force is preserved.
}

} // namespace FFBEngineTests
