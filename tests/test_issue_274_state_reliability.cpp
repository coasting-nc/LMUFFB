#include "test_ffb_common.h"
#include "../src/GameConnector.h"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include <cstring>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_issue_274_robust_session_fallback, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));

    // Scenario: No SME events, but track name is present in buffer.
    StringUtils::SafeCopy(layout.data.scoring.scoringInfo.mTrackName, sizeof(layout.data.scoring.scoringInfo.mTrackName), "Monza");
    layout.data.scoring.scoringInfo.mInRealtime = 0;

    GameConnectorTestAccessor::SetSharedMem(gc, &layout);
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);

    // Polling should have picked up that a session is active.
    ASSERT_TRUE(gc.IsSessionActive());
}

TEST_CASE_TAGGED(test_issue_274_robust_realtime_fallback, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));

    // Scenario: No SME events, but mInRealtime is true in buffer.
    StringUtils::SafeCopy(layout.data.scoring.scoringInfo.mTrackName, sizeof(layout.data.scoring.scoringInfo.mTrackName), "Spa");
    layout.data.scoring.scoringInfo.mInRealtime = 1;

    GameConnectorTestAccessor::SetSharedMem(gc, &layout);
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);

    // Polling should have picked up that we are in realtime.
    ASSERT_TRUE(gc.IsInRealtime());
}

TEST_CASE_TAGGED(test_issue_274_robust_menu_fallback, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));

    // Scenario: Session was active, but now buffer is empty (e.g. game crashed or reset).
    GameConnectorTestAccessor::SetSessionActive(gc, true);
    GameConnectorTestAccessor::SetInRealtime(gc, true);

    layout.data.scoring.scoringInfo.mTrackName[0] = '\0';
    layout.data.scoring.scoringInfo.mInRealtime = 0;

    GameConnectorTestAccessor::SetSharedMem(gc, &layout);
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);

    // Polling should have picked up that we are back in menu.
    ASSERT_FALSE(gc.IsSessionActive());
    ASSERT_FALSE(gc.IsInRealtime());
}

} // namespace FFBEngineTests
