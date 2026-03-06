#include "test_ffb_common.h"
#include "../src/GameConnector.h"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include <cstring>
#include <chrono>
#include <thread>

namespace FFBEngineTests {

// Accessor to reach private members of GameConnector
class GameConnectorTestAccessor {
public:
    static void Reset(GameConnector& gc) {
        std::lock_guard<std::recursive_mutex> lock(gc.m_mutex);
        gc._DisconnectLocked();
        gc.m_sessionActive.store(false);
        gc.m_inRealtime.store(false);
        gc.m_currentSessionType.store(-1);
        gc.m_currentGamePhase.store(255);
        memset(&gc.m_prevState, 0, sizeof(gc.m_prevState));
        gc.m_prevState.optionsLocation = 255;
        gc.m_prevState.gamePhase = 255;
        gc.m_prevState.session = -1;
        gc.m_prevState.control = -2;
        gc.m_prevState.pitState = 255;
        gc.m_prevState.steeringRange = -1.0f;
    }

    static void SetSharedMem(GameConnector& gc, SharedMemoryLayout* layout) {
        std::lock_guard<std::recursive_mutex> lock(gc.m_mutex);
        gc.m_pSharedMemLayout = layout;
        gc.m_connected = true;
    }

    static void SetSessionActive(GameConnector& gc, bool val) { gc.m_sessionActive.store(val); }
    static void SetInRealtime(GameConnector& gc, bool val) { gc.m_inRealtime.store(val); }
    static void SetSessionType(GameConnector& gc, long val) { gc.m_currentSessionType.store(val); }
    static void SetGamePhase(GameConnector& gc, unsigned char val) { gc.m_currentGamePhase.store(val); }

    static void InjectTransitions(GameConnector& gc, const SharedMemoryObjectOut& data) {
        gc.CheckTransitions(data);
    }
};

TEST_CASE_TAGGED(test_issue_267_initial_connection_menu, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));

    // In menu: no track name, no realtime
    layout.data.scoring.scoringInfo.mTrackName[0] = '\0';
    layout.data.scoring.scoringInfo.mInRealtime = 0;
    layout.data.scoring.scoringInfo.mSession = 0;
    layout.data.scoring.scoringInfo.mGamePhase = 0;

    GameConnectorTestAccessor::SetSharedMem(gc, &layout);

    // Manual trigger of initialization (normally in TryConnect)
    GameConnectorTestAccessor::SetSessionActive(gc, layout.data.scoring.scoringInfo.mTrackName[0] != '\0');
    GameConnectorTestAccessor::SetInRealtime(gc, layout.data.scoring.scoringInfo.mInRealtime != 0);
    GameConnectorTestAccessor::SetSessionType(gc, layout.data.scoring.scoringInfo.mSession);
    GameConnectorTestAccessor::SetGamePhase(gc, layout.data.scoring.scoringInfo.mGamePhase);

    ASSERT_FALSE(gc.IsSessionActive());
    ASSERT_FALSE(gc.IsInRealtime());
    ASSERT_EQ(gc.GetSessionType(), 0);
    ASSERT_EQ(gc.GetGamePhase(), 0);
}

TEST_CASE_TAGGED(test_issue_267_initial_connection_cockpit, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));

    // In cockpit: track name present, in realtime
    strncpy(layout.data.scoring.scoringInfo.mTrackName, "Le Mans", 63);
    layout.data.scoring.scoringInfo.mInRealtime = 1;
    layout.data.scoring.scoringInfo.mSession = 10; // Race
    layout.data.scoring.scoringInfo.mGamePhase = 5; // Green Flag

    GameConnectorTestAccessor::SetSharedMem(gc, &layout);

    // Manual trigger of initialization
    GameConnectorTestAccessor::SetSessionActive(gc, layout.data.scoring.scoringInfo.mTrackName[0] != '\0');
    GameConnectorTestAccessor::SetInRealtime(gc, layout.data.scoring.scoringInfo.mInRealtime != 0);
    GameConnectorTestAccessor::SetSessionType(gc, layout.data.scoring.scoringInfo.mSession);
    GameConnectorTestAccessor::SetGamePhase(gc, layout.data.scoring.scoringInfo.mGamePhase);

    ASSERT_TRUE(gc.IsSessionActive());
    ASSERT_TRUE(gc.IsInRealtime());
    ASSERT_EQ(gc.GetSessionType(), 10);
    ASSERT_EQ(gc.GetGamePhase(), 5);
}

TEST_CASE_TAGGED(test_issue_267_transition_cockpit, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));
    GameConnectorTestAccessor::SetSharedMem(gc, &layout);

    // 1. Enter Realtime Event
    layout.data.generic.events[SME_ENTER_REALTIME] = (SharedMemoryEvent)1;
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);
    ASSERT_TRUE(gc.IsInRealtime());

    // 2. Exit Realtime Event
    layout.data.generic.events[SME_EXIT_REALTIME] = (SharedMemoryEvent)1;
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);
    ASSERT_FALSE(gc.IsInRealtime());
}

TEST_CASE_TAGGED(test_issue_267_session_end_clears_realtime, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));
    GameConnectorTestAccessor::SetSharedMem(gc, &layout);

    // Setup: In cockpit
    GameConnectorTestAccessor::SetSessionActive(gc, true);
    GameConnectorTestAccessor::SetInRealtime(gc, true);

    // Trigger SME_END_SESSION
    layout.data.generic.events[SME_END_SESSION] = (SharedMemoryEvent)1;
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);

    ASSERT_FALSE(gc.IsSessionActive());
    ASSERT_FALSE(gc.IsInRealtime());
}

TEST_CASE_TAGGED(test_issue_267_session_type_change, "Functional", (std::vector<std::string>{"state_machine"})) {
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryLayout layout;
    memset(&layout, 0, sizeof(layout));
    GameConnectorTestAccessor::SetSharedMem(gc, &layout);

    // Change session type via polling
    layout.data.scoring.scoringInfo.mSession = 5; // Qualify
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);
    ASSERT_EQ(gc.GetSessionType(), 5);

    layout.data.scoring.scoringInfo.mSession = 10; // Race
    GameConnectorTestAccessor::InjectTransitions(gc, layout.data);
    ASSERT_EQ(gc.GetSessionType(), 10);
}

} // namespace FFBEngineTests
