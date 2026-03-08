#include "GameConnector.h"
#include "Logger.h"
#ifndef _WIN32
#include "lmu_sm_interface/LinuxMock.h"
#endif
#include "lmu_sm_interface/SafeSharedMemoryLock.h"
#include <iostream>
#include <cstring>
#include "StringUtils.h"

#define LEGACY_SHARED_MEMORY_NAME "$rFactor2SMMP_Telemetry$"

GameConnector& GameConnector::Get() {
    static GameConnector instance;
    return instance;
}

GameConnector::GameConnector() {}

GameConnector::~GameConnector() {
    Disconnect();
}

void GameConnector::Disconnect() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    _DisconnectLocked();
}

void GameConnector::_DisconnectLocked() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
    if (m_pSharedMemLayout) {
        UnmapViewOfFile(m_pSharedMemLayout);
        m_pSharedMemLayout = nullptr;
    }
    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
    }
    m_hwndGame = NULL;
#endif
    m_smLock.reset();
    m_connected = false;
    m_processId = 0;
}

bool GameConnector::TryConnect() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_connected) return true;

    // Ensure we don't leak handles from a previous partial/failed attempt
    _DisconnectLocked();

#if defined(_WIN32) || defined(HEADLESS_GUI)
    m_hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, LMU_SHARED_MEMORY_FILE);
    
    if (m_hMapFile == NULL) {
        return false;
    } 

    m_pSharedMemLayout = (SharedMemoryLayout*)MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, sizeof(SharedMemoryLayout));
    if (m_pSharedMemLayout == NULL) {
        Logger::Get().Log("[GameConnector] Could not map view of file.");
        Logger::Get().LogWin32Error("MapViewOfFile", GetLastError());
        _DisconnectLocked();
        return false;
    }

    m_smLock = SafeSharedMemoryLock::MakeSafeSharedMemoryLock();
    if (!m_smLock.has_value()) {
        Logger::Get().Log("[GameConnector] Failed to init LMU Shared Memory Lock");
        _DisconnectLocked();
        return false;
    }

    HWND hwnd = m_pSharedMemLayout->data.generic.appInfo.mAppWindow;
    if (hwnd) {
        m_hwndGame = hwnd; // Store HWND for liveness check (IsWindow)
        // Note: multiple threads might access shared memory, but HWND is usually stable during session.
        // We use IsWindow(m_hwndGame) instead of OpenProcess to avoid AV heuristics flagging "Process Access".
    }

    m_connected = true;

    // Initialize Robust State Machine (#267)
    if (m_pSharedMemLayout) {
        m_sessionActive = (m_pSharedMemLayout->data.scoring.scoringInfo.mTrackName[0] != '\0');
        m_inRealtime = (m_pSharedMemLayout->data.scoring.scoringInfo.mInRealtime != 0);
        m_currentSessionType = m_pSharedMemLayout->data.scoring.scoringInfo.mSession;
        m_currentGamePhase = m_pSharedMemLayout->data.scoring.scoringInfo.mGamePhase;

        if (m_pSharedMemLayout->data.telemetry.playerHasVehicle) {
            uint8_t idx = m_pSharedMemLayout->data.telemetry.playerVehicleIdx;
            if (idx < 104) {
                m_playerControl = m_pSharedMemLayout->data.scoring.vehScoringInfo[idx].mControl;
            }
        }
    }

    m_lastUpdateLocalTime = std::chrono::steady_clock::now();
    Logger::Get().Log("[GameConnector] Connected to LMU Shared Memory.");
    return true;
#else
    return false;
#endif
}

bool GameConnector::CheckLegacyConflict() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
    HANDLE hLegacy = OpenFileMappingA(FILE_MAP_READ, FALSE, LEGACY_SHARED_MEMORY_NAME);
    if (hLegacy) {
        Logger::Get().Log("[Warning] Legacy rFactor 2 Shared Memory Plugin detected. This may conflict with LMU 1.2 data.");
        CloseHandle(hLegacy);
        return true;
    }
#endif
    return false;
}

bool GameConnector::IsConnected() const {
  if (!m_connected.load(std::memory_order_acquire)) return false;

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!m_connected.load(std::memory_order_relaxed)) return false;

#if defined(_WIN32) || defined(HEADLESS_GUI)
  if (m_hwndGame) {
    if (!IsWindow(m_hwndGame)) {
      // Window is gone, game likely exited
      const_cast<GameConnector*>(this)->_DisconnectLocked();
      return false;
    }
  }
#endif

  return m_connected.load(std::memory_order_relaxed) && m_pSharedMemLayout && m_smLock.has_value();
}

bool GameConnector::CopyTelemetry(SharedMemoryObjectOut& dest) {
    if (!m_connected.load(std::memory_order_acquire)) return false;

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_connected.load(std::memory_order_relaxed) || !m_pSharedMemLayout || !m_smLock.has_value()) return false;

    if (m_smLock->Lock(50)) {
        CopySharedMemoryObj(dest, m_pSharedMemLayout->data);
        
        if (dest.telemetry.playerHasVehicle) {
            uint8_t idx = dest.telemetry.playerVehicleIdx;
            if (idx < 104) {
                double currentET = dest.telemetry.telemInfo[idx].mElapsedTime;
                double currentSteer = dest.telemetry.telemInfo[idx].mUnfilteredSteering;

                // Heartbeat: Update timer if game is running (ET changes)
                // OR if user is moving the wheel (Steering changes) - Issue #184
                if (currentET != m_lastElapsedTime || std::abs(currentSteer - m_lastSteering) > 0.0001) {
                    m_lastElapsedTime = currentET;
                    m_lastSteering = currentSteer;
                    m_lastUpdateLocalTime = std::chrono::steady_clock::now();
                }
            }
        } else {
            m_lastUpdateLocalTime = std::chrono::steady_clock::now();
        }

        m_smLock->Unlock();

        CheckTransitions(dest);

        return m_inRealtime.load(std::memory_order_relaxed);
    } else {
        return false;
    }
}

bool GameConnector::IsStale(long timeoutMs) const {
    if (!m_connected.load(std::memory_order_acquire)) return true;

    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdateLocalTime).count();
    return (diff > timeoutMs);
}

// ---------------------------------------------------------------------------
// Static string-lookup helpers
// ---------------------------------------------------------------------------

/*static*/ const char* GameConnector::SmeEventName(int i) {
    switch (i) {
        case SME_ENTER:              return "SME_ENTER";
        case SME_EXIT:               return "SME_EXIT";
        case SME_STARTUP:            return "SME_STARTUP";
        case SME_SHUTDOWN:           return "SME_SHUTDOWN";
        case SME_LOAD:               return "SME_LOAD";
        case SME_UNLOAD:             return "SME_UNLOAD";
        case SME_START_SESSION:      return "SME_START_SESSION";
        case SME_END_SESSION:        return "SME_END_SESSION";
        case SME_ENTER_REALTIME:     return "SME_ENTER_REALTIME";
        case SME_EXIT_REALTIME:      return "SME_EXIT_REALTIME";
        case SME_INIT_APPLICATION:   return "SME_INIT_APPLICATION";
        case SME_UNINIT_APPLICATION: return "SME_UNINIT_APPLICATION";
        default:                     return "Unknown";
    }
}

/*static*/ const char* GameConnector::GamePhaseName(unsigned char phase) {
    switch (phase) {
        case 0: return "Before Session";
        case 1: return "Reconnaissance";
        case 2: return "Grid Walk";
        case 3: return "Formation";
        case 4: return "Countdown";
        case 5: return "Green Flag";
        case 6: return "Full Course Yellow";
        case 7: return "Stopped";
        case 8: return "Over";
        case 9: return "Paused";
        default: return "Unknown";
    }
}

/*static*/ const char* GameConnector::SessionTypeName(long session) {
    if (session == 0)                   return "Test Day";
    if (session >= 1 && session <= 4)   return "Practice";
    if (session >= 5 && session <= 8)   return "Qualifying";
    if (session == 9)                   return "Warmup";
    if (session >= 10 && session <= 13) return "Race";
    return "Unknown";
}

/*static*/ const char* GameConnector::ControlModeName(signed char control) {
    switch (control) {
        case -1: return "Nobody";
        case  0: return "Player";
        case  1: return "AI";
        case  2: return "Remote";
        case  3: return "Replay";
        default: return "Unknown";
    }
}

/*static*/ const char* GameConnector::PitStateName(unsigned char pitState) {
    switch (pitState) {
        case 0: return "None";
        case 1: return "Request";
        case 2: return "Entering";
        case 3: return "Stopped";
        case 4: return "Exiting";
        default: return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// Phase 1: Update state machine atomics unconditionally from the SM snapshot.
//
// "Ground truth polling": what the shared memory says right now is the truth.
// SME events (applied at the end) are a fast-path that can override the polled
// values within the same tick — they handle rapid transitions before the next
// poll tick arrives.
// ---------------------------------------------------------------------------

void GameConnector::_UpdateStateFromSnapshot(const SharedMemoryObjectOut& current) {
    auto& scoring = current.scoring.scoringInfo;
    auto  now     = std::chrono::steady_clock::now();

    // --- Ground truth (polling) ---
    // Save previous inRealtime before overwriting, so we can detect transitions.
    bool prevInRealtime = m_inRealtime.load(std::memory_order_relaxed);

    m_sessionActive      = (scoring.mTrackName[0] != '\0');
    m_inRealtime         = (scoring.mInRealtime != 0);
    m_currentGamePhase   = scoring.mGamePhase;
    m_currentSessionType = scoring.mSession;

    bool currentInRealtime = m_inRealtime.load(std::memory_order_relaxed);

    // --- Quit-to-menu detection (#7.5) ---
    // When mInRealtime goes true→false while a session is active, arm a
    // pending check. If SME_ENTER fires within the deadline window, the user
    // quit to the main menu (LMU doesn't fire SME_END_SESSION for this path).
    // If mInRealtime goes back to true first, the user is driving again —
    // cancel the check. Debug logs confirm SME_ENTER fires within 0-1 seconds
    // on quit-to-menu and does NOT fire on a normal return to the garage monitor.
    if (prevInRealtime && !currentInRealtime &&
        m_sessionActive.load(std::memory_order_relaxed)) {
        m_pendingMenuCheck  = true;
        m_menuCheckDeadline = now + std::chrono::seconds(3);
    }
    if (currentInRealtime) {
        m_pendingMenuCheck = false; // user clicked Drive — cancel
    }
    if (m_pendingMenuCheck && now > m_menuCheckDeadline) {
        m_pendingMenuCheck = false; // deadline expired — no SME_ENTER came, must be garage
    }


    if (current.telemetry.playerHasVehicle) {
        uint8_t idx = current.telemetry.playerVehicleIdx;
        if (idx < 104) {
            m_playerControl = current.scoring.vehScoringInfo[idx].mControl;
        }
    }

    // --- Fast-path: apply SME event overrides on top of polled truth (#267, #274) ---
    // This provides "instantaneous" updates while polling ensures long-term consistency.
    auto& generic = current.generic;
    for (int i = 0; i < SME_MAX; ++i) {
        if (i == SME_UPDATE_SCORING || i == SME_UPDATE_TELEMETRY || i == SME_FFB || i == SME_SET_ENVIRONMENT)
            continue;
        if (generic.events[i] != m_prevState.eventState[i] && generic.events[i] != 0) {
            if (i == SME_START_SESSION)                     m_sessionActive = true;
            if (i == SME_END_SESSION || i == SME_UNLOAD) { m_sessionActive = false; m_inRealtime = false; }
            if (i == SME_ENTER_REALTIME)                    m_inRealtime = true;
            if (i == SME_EXIT_REALTIME)                     m_inRealtime = false;

            // Quit-to-menu detection (#7.5): SME_ENTER fires within ~1 second of
            // de-realtime when the user quits to the main menu. It does NOT fire
            // when the user returns to the garage monitor. (Confirmed via debug logs.)
            if (i == SME_ENTER && m_pendingMenuCheck) {
                m_sessionActive    = false;
                m_pendingMenuCheck = false;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Phase 2: Detect changes vs m_prevState and emit log lines.
//   This phase has no side-effects on the state machine atomics.
// ---------------------------------------------------------------------------

void GameConnector::_LogTransitions(const SharedMemoryObjectOut& current) {
    auto& scoring = current.scoring.scoringInfo;
    auto& generic = current.generic;

    // 0. Shared Memory Events (Issue #244)
    for (int i = 0; i < SME_MAX; ++i) {
        if (i == SME_UPDATE_SCORING || i == SME_UPDATE_TELEMETRY || i == SME_FFB || i == SME_SET_ENVIRONMENT)
            continue;

        if (generic.events[i] != m_prevState.eventState[i]) {
            if (generic.events[i] != 0) {
                bool shouldLog = true;

                // SME_STARTUP is known to spam values (e.g. 10, 16) in menus. Apply a 5-second cooldown.
                if (i == SME_STARTUP) {
                    auto now  = std::chrono::steady_clock::now();
                    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - m_prevState.lastEventLogTime[i]).count();
                    if (diff < 5) {
                        shouldLog = false;
                    } else {
                        m_prevState.lastEventLogTime[i] = now;
                    }
                }

                if (shouldLog) {
                    Logger::Get().LogFile("[Transition] Event: %s (%u)", SmeEventName(i), generic.events[i]);
                }
            }
            m_prevState.eventState[i] = generic.events[i];
        }
    }

    // 1. Options Location (UI Menu)
    if (generic.appInfo.mOptionsLocation != m_prevState.optionsLocation) {
        const char* locStr = "Unknown";
        switch (generic.appInfo.mOptionsLocation) {
            case 0: locStr = "Main UI"; break;
            case 1: locStr = "Track Loading"; break;
            case 2: locStr = "Monitor (Garage)"; break;
            case 3: locStr = "On Track"; break;
        }
        Logger::Get().LogFile("[Transition] OptionsLocation: %d -> %d (%s)",
            m_prevState.optionsLocation, generic.appInfo.mOptionsLocation, locStr);
        m_prevState.optionsLocation = generic.appInfo.mOptionsLocation;
    }

    // 1.1 Options Page
    if (strncmp(generic.appInfo.mOptionsPage, m_prevState.optionsPage, 31) != 0) {
        Logger::Get().LogFile("[Transition] OptionsPage: '%s' -> '%s'", m_prevState.optionsPage, generic.appInfo.mOptionsPage);
        StringUtils::SafeCopy(m_prevState.optionsPage, sizeof(m_prevState.optionsPage), generic.appInfo.mOptionsPage);
    }

    // 2. InRealtime (Driving vs Menu)
    bool currentInRealtime = (scoring.mInRealtime != 0);
    if (currentInRealtime != m_prevState.inRealtime) {
        Logger::Get().LogFile("[Transition] InRealtime: %s -> %s",
            m_prevState.inRealtime ? "true" : "false", currentInRealtime ? "true" : "false");

        // Investigation dump (#7.4c): fires on every de-realtime event (garage return OR
        // quit-to-menu). Captures signals needed to distinguish the two paths.
        if (!currentInRealtime && m_prevState.inRealtime) {
            Logger::Get().LogFile(
                "[Diag] De-realtime snapshot: track='%s' optionsLoc=%d "
                "numVehicles=%d playerHasVehicle=%s smUpdateScoring=%u",
                scoring.mTrackName,
                (int)generic.appInfo.mOptionsLocation,
                (int)scoring.mNumVehicles,
                current.telemetry.playerHasVehicle ? "true" : "false",
                (unsigned)generic.events[SME_UPDATE_SCORING]);
        }

        m_prevState.inRealtime = currentInRealtime;
    }

    // 3. Game Phase (Session state / Pause)
    if (scoring.mGamePhase != m_prevState.gamePhase) {
        Logger::Get().LogFile("[Transition] GamePhase: %d -> %d (%s)",
            m_prevState.gamePhase, scoring.mGamePhase, GamePhaseName(scoring.mGamePhase));
        m_prevState.gamePhase = scoring.mGamePhase;
    }

    // 4. Session Type
    if (scoring.mSession != m_prevState.session) {
        Logger::Get().LogFile("[Transition] Session: %ld -> %ld (%s)",
            m_prevState.session, scoring.mSession, SessionTypeName(scoring.mSession));
        m_prevState.session = scoring.mSession;
    }

    // 5. Track Name (Context Change)
    if (strcmp(scoring.mTrackName, m_prevState.trackName) != 0) {
        Logger::Get().LogFile("[Transition] Track: '%s' -> '%s'", m_prevState.trackName, scoring.mTrackName);
        StringUtils::SafeCopy(m_prevState.trackName, sizeof(m_prevState.trackName), scoring.mTrackName);
    }

    // 6. Player Control & Pit State
    if (current.telemetry.playerHasVehicle) {
        uint8_t idx = current.telemetry.playerVehicleIdx;
        if (idx < 104) {
            auto& vehScoring = current.scoring.vehScoringInfo[idx];

            if (vehScoring.mControl != m_prevState.control) {
                Logger::Get().LogFile("[Transition] Control: %d -> %d (%s)",
                    m_prevState.control, vehScoring.mControl, ControlModeName(vehScoring.mControl));
                m_prevState.control = vehScoring.mControl;
            }

            if (vehScoring.mPitState != m_prevState.pitState) {
                Logger::Get().LogFile("[Transition] PitState: %d -> %d (%s)",
                    m_prevState.pitState, vehScoring.mPitState, PitStateName(vehScoring.mPitState));
                m_prevState.pitState = vehScoring.mPitState;
            }

            if (strcmp(vehScoring.mVehicleName, m_prevState.vehicleName) != 0) {
                Logger::Get().LogFile("[Transition] Vehicle: '%s' -> '%s'", m_prevState.vehicleName, vehScoring.mVehicleName);
                StringUtils::SafeCopy(m_prevState.vehicleName, sizeof(m_prevState.vehicleName), vehScoring.mVehicleName);
            }

            // 7. Steering Range
            float currentRange = current.telemetry.telemInfo[idx].mPhysicalSteeringWheelRange;
            if (std::abs(currentRange - m_prevState.steeringRange) > 0.001f) {
                Logger::Get().LogFile("[Transition] SteeringRange: %.2f -> %.2f", m_prevState.steeringRange, currentRange);
                m_prevState.steeringRange = currentRange;
            }
        }
    }

    // 7. PlayerHasVehicle changes — investigation: quit-to-menu detection (#7.4a)
    bool currentHasVehicle = current.telemetry.playerHasVehicle;
    if (currentHasVehicle != m_prevState.playerHasVehicle) {
        Logger::Get().LogFile("[Transition] PlayerHasVehicle: %s -> %s",
            m_prevState.playerHasVehicle ? "true" : "false",
            currentHasVehicle ? "true" : "false");
        m_prevState.playerHasVehicle = currentHasVehicle;
    }

    // 8. NumVehicles changes — investigation: quit-to-menu detection (#7.4b)
    int currentNumVehicles = (int)scoring.mNumVehicles;
    if (currentNumVehicles != m_prevState.numVehicles) {
        Logger::Get().LogFile("[Transition] NumVehicles: %d -> %d",
            m_prevState.numVehicles, currentNumVehicles);
        m_prevState.numVehicles = currentNumVehicles;
    }
}

// ---------------------------------------------------------------------------
// CheckTransitions: orchestrator (lock, then run both phases in order)
// ---------------------------------------------------------------------------

void GameConnector::CheckTransitions(const SharedMemoryObjectOut& current) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    _UpdateStateFromSnapshot(current);  // Phase 1: sync state machine atomics
    _LogTransitions(current);           // Phase 2: log any changes
}
