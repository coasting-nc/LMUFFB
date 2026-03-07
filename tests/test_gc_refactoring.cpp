// test_gc_refactoring.cpp
//
// Pre-refactoring regression tests for GameConnector::CheckTransitions.
// Written before the refactoring so they act as a safety net:
// every test here should pass both BEFORE and AFTER the changes.
//
// Coverage:
//   5.1  Polling always wins (state from SM buffer, not only from events)
//   5.2  Event fast-path still applies within the same tick
//   5.3  GamePhase / SessionType updated unconditionally from buffer
//   5.4  PlayerControl updated from player vehicle
//   5.5  IsPlayerActivelyDriving composite predicate (new)
//   5.6  No duplicate log entries on identical ticks
//   5.7  SME event name lookup helper (after SmeEventName() is extracted)

#include "test_ffb_common.h"
#include "../src/GameConnector.h"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include <cstring>
#include <fstream>
#include <string>

namespace FFBEngineTests {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SharedMemoryObjectOut MakeEmptySnapshot() {
    SharedMemoryObjectOut snap;
    memset(&snap, 0, sizeof(snap));
    return snap;
}

// Count lines in a log file containing a given substring.
static int CountLinesInFile(const std::string& filename, const std::string& needle) {
    std::ifstream f(filename);
    int count = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (line.find(needle) != std::string::npos)
            ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// 5.1  Polling always wins
// ---------------------------------------------------------------------------

TEST_CASE_TAGGED(test_gc_poll_overrides_stale_realtime,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // Manually tell the state machine "we are in realtime"
    GameConnectorTestAccessor::SetInRealtime(gc, true);
    ASSERT_TRUE(gc.IsInRealtime());

    // Now inject a snapshot where mInRealtime==0 and NO exit event was raised.
    // The polling pass must win and drive inRealtime back to false.
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    snap.scoring.scoringInfo.mInRealtime = 0;   // buffer says: not in realtime
    // (no SME_EXIT_REALTIME event set)

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    ASSERT_FALSE(gc.IsInRealtime()); // poll must have won
}

TEST_CASE_TAGGED(test_gc_poll_overrides_stale_session,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // Manually tell the state machine "session is active"
    GameConnectorTestAccessor::SetSessionActive(gc, true);
    ASSERT_TRUE(gc.IsSessionActive());

    // Inject a snapshot where the track name is empty (no session) and no end-session event.
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    snap.scoring.scoringInfo.mTrackName[0] = '\0'; // no track loaded

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    ASSERT_FALSE(gc.IsSessionActive()); // poll must have won
}

// ---------------------------------------------------------------------------
// 5.2  Event fast-path overrides stale buffer on the same tick
// ---------------------------------------------------------------------------

TEST_CASE_TAGGED(test_gc_end_session_event_overrides_poll,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // Snapshot: buffer still claims "track loaded + in realtime" (hasn't been zeroed yet)
    // but SME_END_SESSION fires simultaneously.
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Monza", 63);
    snap.scoring.scoringInfo.mInRealtime = 1;
    snap.generic.events[SME_END_SESSION] = (SharedMemoryEvent)1;

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    // The event fast-path must have cleared both flags despite the buffer still being "hot".
    ASSERT_FALSE(gc.IsSessionActive());
    ASSERT_FALSE(gc.IsInRealtime());
}

// ---------------------------------------------------------------------------
// 5.3  GamePhase and SessionType are always updated from the buffer
// ---------------------------------------------------------------------------

TEST_CASE_TAGGED(test_gc_gamephase_updated_unconditionally,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();

    // First tick: phase = 5 (Green Flag)
    snap.scoring.scoringInfo.mGamePhase = 5;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ((int)gc.GetGamePhase(), 5);

    // Second tick: phase = 8 (Session Over) — no special event needed
    snap.scoring.scoringInfo.mGamePhase = 8;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ((int)gc.GetGamePhase(), 8);

    // Third tick: back to 0 (Before Session)
    snap.scoring.scoringInfo.mGamePhase = 0;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ((int)gc.GetGamePhase(), 0);
}

TEST_CASE_TAGGED(test_gc_sessiontype_updated_unconditionally,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();

    snap.scoring.scoringInfo.mSession = 5; // Qualifying
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ(gc.GetSessionType(), 5L);

    snap.scoring.scoringInfo.mSession = 10; // Race
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ(gc.GetSessionType(), 10L);
}

// ---------------------------------------------------------------------------
// 5.4  PlayerControl updated from vehicle, unchanged when no vehicle
// ---------------------------------------------------------------------------

TEST_CASE_TAGGED(test_gc_player_control_from_vehicle,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    snap.telemetry.playerHasVehicle = true;
    snap.telemetry.playerVehicleIdx = 0;
    snap.scoring.vehScoringInfo[0].mControl = 0; // Player

    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ((int)gc.GetPlayerControl(), 0);

    // Transition to AI
    snap.scoring.vehScoringInfo[0].mControl = 1;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ((int)gc.GetPlayerControl(), 1);

    // Transition to Nobody
    snap.scoring.vehScoringInfo[0].mControl = -1;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_EQ((int)gc.GetPlayerControl(), -1);
}

TEST_CASE_TAGGED(test_gc_player_control_unchanged_when_no_vehicle,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // Set a known control value first
    GameConnectorTestAccessor::SetPlayerControl(gc, 0); // Player
    ASSERT_EQ((int)gc.GetPlayerControl(), 0);

    // Now inject a snapshot with no vehicle
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    snap.telemetry.playerHasVehicle = false;

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    // GetPlayerControl() must be unchanged because there is no vehicle to read from
    ASSERT_EQ((int)gc.GetPlayerControl(), 0);
}

// ---------------------------------------------------------------------------
// 5.5  IsPlayerActivelyDriving composite predicate
// ---------------------------------------------------------------------------

TEST_CASE_TAGGED(test_gc_actively_driving_true,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Le Mans", 63);
    snap.scoring.scoringInfo.mInRealtime  = 1;
    snap.scoring.scoringInfo.mGamePhase   = 5; // Green Flag — not paused
    snap.scoring.scoringInfo.mSession     = 10;
    snap.telemetry.playerHasVehicle       = true;
    snap.telemetry.playerVehicleIdx       = 0;
    snap.scoring.vehScoringInfo[0].mControl = 0; // Player

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    ASSERT_TRUE(gc.IsPlayerActivelyDriving());
}

TEST_CASE_TAGGED(test_gc_actively_driving_false_when_paused,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    // Addresses the ESC-menu-while-on-track edge case from session transition.md.
    // Game phase 9 == Paused (single-player ESC menu while on track).
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Spa", 63);
    snap.scoring.scoringInfo.mInRealtime  = 1;
    snap.scoring.scoringInfo.mGamePhase   = 9; // Paused
    snap.scoring.scoringInfo.mSession     = 5;
    snap.telemetry.playerHasVehicle       = true;
    snap.telemetry.playerVehicleIdx       = 0;
    snap.scoring.vehScoringInfo[0].mControl = 0; // Player (still player, but paused)

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    // Even though we are "in realtime" and Player-controlled, user pressed ESC → paused
    ASSERT_FALSE(gc.IsPlayerActivelyDriving());
}

TEST_CASE_TAGGED(test_gc_actively_driving_false_when_ai,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Silverstone", 63);
    snap.scoring.scoringInfo.mInRealtime  = 1;
    snap.scoring.scoringInfo.mGamePhase   = 5; // Green Flag
    snap.telemetry.playerHasVehicle       = true;
    snap.telemetry.playerVehicleIdx       = 0;
    snap.scoring.vehScoringInfo[0].mControl = 1; // AI

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    ASSERT_FALSE(gc.IsPlayerActivelyDriving());
}

TEST_CASE_TAGGED(test_gc_actively_driving_false_when_not_realtime,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // Player is in Garage/Monitor UI — not in realtime
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Monza", 63);
    snap.scoring.scoringInfo.mInRealtime  = 0; // NOT in cockpit
    snap.scoring.scoringInfo.mGamePhase   = 0; // Before Session (Garage)
    snap.telemetry.playerHasVehicle       = true;
    snap.telemetry.playerVehicleIdx       = 0;
    snap.scoring.vehScoringInfo[0].mControl = 0; // Player (sitting in garage)

    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    ASSERT_FALSE(gc.IsPlayerActivelyDriving());
}

TEST_CASE_TAGGED(test_gc_logging_gate_independent_of_session_active,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    // This confirms the change in main.cpp where `(!is_session_active && was_driving)`
    // was removed. FFB and logging rely exclusively on `IsPlayerActivelyDriving()`.
    // Returning to the garage keeps IsSessionActive() = true, but must still safely
    // stop FFB/logging by turning IsPlayerActivelyDriving() = false.
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // 1. Actively driving
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Le Mans", 63);
    snap.scoring.scoringInfo.mInRealtime  = 1;
    snap.scoring.scoringInfo.mGamePhase   = 5;
    snap.telemetry.playerHasVehicle       = true;
    snap.telemetry.playerVehicleIdx       = 0;
    snap.scoring.vehScoringInfo[0].mControl = 0;

    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_TRUE(gc.IsSessionActive());
    ASSERT_TRUE(gc.IsPlayerActivelyDriving());

    // 2. Return to garage (de-realtime)
    snap.scoring.scoringInfo.mInRealtime  = 0;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    // Session is still active (track loaded, sitting in garage UI)
    ASSERT_TRUE(gc.IsSessionActive());
    // But driving check flips to false immediately, correctly gating FFB and logging.
    ASSERT_FALSE(gc.IsPlayerActivelyDriving());
}

// ---------------------------------------------------------------------------
// 5.5b  Quit-to-main-menu detection via SME_ENTER after de-realtime
// ---------------------------------------------------------------------------
//
// Root cause (confirmed via debug log analysis, 2026-03-07):
//   - LMU does NOT fire SME_END_SESSION or SME_UNLOAD when quitting to menu.
//   - LMU does NOT clear mTrackName in the buffer (it stays stale).
//   - SME_ENTER IS fired within 0-1 seconds of de-realtime on quit-to-menu.
//   - SME_ENTER is NOT fired when the user returns to the garage monitor.
//
// Fix: arm m_pendingMenuCheck when mInRealtime goes true→false. If SME_ENTER
// fires within the 3-second window, end the session. If mInRealtime goes back
// to true first (user clicked Drive), cancel.

TEST_CASE_TAGGED(test_gc_main_menu_ends_session_via_sme_enter,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // Step 1: actively driving
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Le Mans", 63);
    snap.scoring.scoringInfo.mInRealtime = 1;
    snap.scoring.scoringInfo.mGamePhase  = 5;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_TRUE(gc.IsSessionActive());
    ASSERT_TRUE(gc.IsInRealtime());

    // Step 2: quit to main menu — LMU fires SME_ENTER but NOT SME_END_SESSION.
    // mTrackName is NOT cleared (stale buffer reflects last track).
    snap.scoring.scoringInfo.mInRealtime = 0;
    snap.generic.events[SME_ENTER] = (SharedMemoryEvent)1; // counter changed
    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    // The pending check + SME_ENTER must have ended the session.
    ASSERT_FALSE(gc.IsSessionActive());
    ASSERT_FALSE(gc.IsInRealtime());
}

TEST_CASE_TAGGED(test_gc_garage_return_keeps_session_active,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    // Step 1: actively driving
    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    strncpy(snap.scoring.scoringInfo.mTrackName, "Le Mans", 63);
    snap.scoring.scoringInfo.mInRealtime = 1;
    snap.scoring.scoringInfo.mGamePhase  = 5;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_TRUE(gc.IsSessionActive());

    // Step 2: return to garage — mInRealtime=0, but SME_ENTER does NOT fire.
    snap.scoring.scoringInfo.mInRealtime = 0;
    // (SME_ENTER counter stays 0 — not fired)
    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    // Session must stay active — the track is still loaded, user is in garage.
    ASSERT_TRUE(gc.IsSessionActive());
    ASSERT_FALSE(gc.IsInRealtime());

    // Step 3: user clicks Drive — inRealtime comes back
    snap.scoring.scoringInfo.mInRealtime = 1;
    GameConnectorTestAccessor::InjectTransitions(gc, snap);
    ASSERT_TRUE(gc.IsSessionActive());
    ASSERT_TRUE(gc.IsInRealtime());
}

// ---------------------------------------------------------------------------
// 5.6  No duplicate log entries on identical ticks
// ---------------------------------------------------------------------------

TEST_CASE_TAGGED(test_gc_no_duplicate_log_on_same_state,
    "Functional", (std::vector<std::string>{"transitions", "refactoring"}))
{
    Logger::Get().Init("test_refactoring_noduplicate.log");
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();
    snap.scoring.scoringInfo.mGamePhase   = 5;
    snap.scoring.scoringInfo.mSession     = 10;
    strncpy(snap.scoring.scoringInfo.mTrackName, "Imola", 63);

    // First call: this should log the transitions (phase, session, track)
    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    // Count log entries after the first call
    int count_after_first = CountLinesInFile("test_refactoring_noduplicate.log", "[Transition] GamePhase:");

    // Second call with identical data: must NOT add more log entries
    GameConnectorTestAccessor::InjectTransitions(gc, snap);

    int count_after_second = CountLinesInFile("test_refactoring_noduplicate.log", "[Transition] GamePhase:");

    // The count must not have grown — no duplicate on unchanged state
    ASSERT_EQ(count_after_first, count_after_second);
}

// ---------------------------------------------------------------------------
// 5.7  SME event name lookup (validated after SmeEventName() is extracted)
// ---------------------------------------------------------------------------

TEST_CASE_TAGGED(test_gc_sme_event_name_lookup,
    "Functional", (std::vector<std::string>{"state_machine", "refactoring"}))
{
    // Validate the extracted SmeEventName() helper.
    // If the helper doesn't exist yet (before refactoring), this test still
    // exercises the behaviour indirectly through transition logging.
    Logger::Get().Init("test_refactoring_sme_names.log");
    GameConnector& gc = GameConnector::Get();
    GameConnectorTestAccessor::Reset(gc);

    SharedMemoryObjectOut snap = MakeEmptySnapshot();

    // Fire each named SME event and confirm the correct string appears in the log.
    struct { int idx; const char* expected; } cases[] = {
        { SME_ENTER,              "SME_ENTER" },
        { SME_EXIT,               "SME_EXIT" },
        { SME_STARTUP,            "SME_STARTUP" },
        { SME_SHUTDOWN,           "SME_SHUTDOWN" },
        { SME_LOAD,               "SME_LOAD" },
        { SME_UNLOAD,             "SME_UNLOAD" },
        { SME_START_SESSION,      "SME_START_SESSION" },
        { SME_END_SESSION,        "SME_END_SESSION" },
        { SME_ENTER_REALTIME,     "SME_ENTER_REALTIME" },
        { SME_EXIT_REALTIME,      "SME_EXIT_REALTIME" },
        { SME_INIT_APPLICATION,   "SME_INIT_APPLICATION" },
        { SME_UNINIT_APPLICATION, "SME_UNINIT_APPLICATION" },
    };

    // Trigger each event with an incrementing value (to ensure it's always "new")
    uint32_t trigger = 1;
    for (auto& c : cases) {
        // Reset all events first so we only see one at a time
        memset(snap.generic.events, 0, sizeof(snap.generic.events));
        snap.generic.events[c.idx] = (SharedMemoryEvent)trigger++;
        GameConnectorTestAccessor::InjectTransitions(gc, snap);
        ASSERT_TRUE(IsInLog("test_refactoring_sme_names.log", c.expected));
    }
}

} // namespace FFBEngineTests
