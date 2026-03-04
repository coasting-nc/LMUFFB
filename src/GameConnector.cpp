#include "GameConnector.h"
#include "Logger.h"
#ifndef _WIN32
#include "lmu_sm_interface/LinuxMock.h"
#endif
#include "lmu_sm_interface/SafeSharedMemoryLock.h"
#include <iostream>
#include <cstring>

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
        Logger::Get().Log("[GameConnector] Could not map view of file.");
        Logger::Get().LogWin32Error("MapViewOfFile", GetLastError());
        _DisconnectLocked();
        return false;
    }

    m_smLock = SafeSharedMemoryLock::MakeSafeSharedMemoryLock();
    if (!m_smLock.has_value()) {
        Logger::Get().Log("[GameConnector] Failed to init LMU Shared Memory Lock");
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
    m_lastUpdateLocalTime = std::chrono::steady_clock::now();
    Logger::Get().Log("[GameConnector] Connected to LMU Shared Memory.");
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

        bool isRealtime = (m_pSharedMemLayout->data.scoring.scoringInfo.mInRealtime != 0);
        m_smLock->Unlock();

        CheckTransitions(dest);

        return isRealtime;
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

void GameConnector::CheckTransitions(const SharedMemoryObjectOut& current) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto& scoring = current.scoring.scoringInfo;
    auto& generic = current.generic;

    // 0. Shared Memory Events (Issue #244)
    for (int i = 0; i < SME_MAX; ++i) {
        // Skip high-frequency events
        if (i == SME_UPDATE_SCORING || i == SME_UPDATE_TELEMETRY || i == SME_FFB || i == SME_SET_ENVIRONMENT) continue;

        if (generic.events[i] != m_prevState.eventState[i]) {
            if (generic.events[i] != 0) {
                bool shouldLog = true;

                // SME_STARTUP is known to spam values (e.g. 10, 16) in menus. Apply a 5-second cooldown.
                if (i == SME_STARTUP) {
                    auto now = std::chrono::steady_clock::now();
                    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - m_prevState.lastEventLogTime[i]).count();
                    if (diff < 5) {
                        shouldLog = false;
                    } else {
                        m_prevState.lastEventLogTime[i] = now;
                    }
                }

                if (shouldLog) {
                    const char* eventStr = "Unknown";
                    switch (i) {
                    case SME_ENTER: eventStr = "SME_ENTER"; break;
                    case SME_EXIT: eventStr = "SME_EXIT"; break;
                    case SME_STARTUP: eventStr = "SME_STARTUP"; break;
                    case SME_SHUTDOWN: eventStr = "SME_SHUTDOWN"; break;
                    case SME_LOAD: eventStr = "SME_LOAD"; break;
                    case SME_UNLOAD: eventStr = "SME_UNLOAD"; break;
                    case SME_START_SESSION: eventStr = "SME_START_SESSION"; break;
                    case SME_END_SESSION: eventStr = "SME_END_SESSION"; break;
                    case SME_ENTER_REALTIME: eventStr = "SME_ENTER_REALTIME"; break;
                    case SME_EXIT_REALTIME: eventStr = "SME_EXIT_REALTIME"; break;
                    case SME_INIT_APPLICATION: eventStr = "SME_INIT_APPLICATION"; break;
                    case SME_UNINIT_APPLICATION: eventStr = "SME_UNINIT_APPLICATION"; break;
                    }
                    Logger::Get().LogFile("[Transition] Event: %s (%u)", eventStr, generic.events[i]);
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
        strncpy(m_prevState.optionsPage, generic.appInfo.mOptionsPage, 31);
        m_prevState.optionsPage[31] = '\0';
    }

    // 2. InRealtime (Driving vs Menu)
    bool currentInRealtime = (scoring.mInRealtime != 0);
    if (currentInRealtime != m_prevState.inRealtime) {
        Logger::Get().LogFile("[Transition] InRealtime: %s -> %s",
            m_prevState.inRealtime ? "true" : "false", currentInRealtime ? "true" : "false");
        m_prevState.inRealtime = currentInRealtime;
    }

    // 3. Game Phase (Session state / Pause)
    if (scoring.mGamePhase != m_prevState.gamePhase) {
        const char* phaseStr = "Unknown";
        switch (scoring.mGamePhase) {
            case 0: phaseStr = "Before Session"; break;
            case 1: phaseStr = "Reconnaissance"; break;
            case 2: phaseStr = "Grid Walk"; break;
            case 3: phaseStr = "Formation"; break;
            case 4: phaseStr = "Countdown"; break;
            case 5: phaseStr = "Green Flag"; break;
            case 6: phaseStr = "Full Course Yellow"; break;
            case 7: phaseStr = "Stopped"; break;
            case 8: phaseStr = "Over"; break;
            case 9: phaseStr = "Paused"; break;
        }
        Logger::Get().LogFile("[Transition] GamePhase: %d -> %d (%s)",
            m_prevState.gamePhase, scoring.mGamePhase, phaseStr);
        m_prevState.gamePhase = scoring.mGamePhase;
    }

    // 4. Session Type
    if (scoring.mSession != m_prevState.session) {
        const char* sessionStr = "Unknown";
        if (scoring.mSession == 0) sessionStr = "Test Day";
        else if (scoring.mSession >= 1 && scoring.mSession <= 4) sessionStr = "Practice";
        else if (scoring.mSession >= 5 && scoring.mSession <= 8) sessionStr = "Qualifying";
        else if (scoring.mSession == 9) sessionStr = "Warmup";
        else if (scoring.mSession >= 10 && scoring.mSession <= 13) sessionStr = "Race";

        Logger::Get().LogFile("[Transition] Session: %ld -> %ld (%s)",
            m_prevState.session, scoring.mSession, sessionStr);
        m_prevState.session = scoring.mSession;
    }

    // 5. Track/Vehicle Names (Context Changes)
    if (strcmp(scoring.mTrackName, m_prevState.trackName) != 0) {
        Logger::Get().LogFile("[Transition] Track: '%s' -> '%s'", m_prevState.trackName, scoring.mTrackName);
        strncpy(m_prevState.trackName, scoring.mTrackName, 63);
    }

    // 6. Player Control & Pit State
    if (current.telemetry.playerHasVehicle) {
        uint8_t idx = current.telemetry.playerVehicleIdx;
        if (idx < 104) {
            auto& vehScoring = current.scoring.vehScoringInfo[idx];

            if (vehScoring.mControl != m_prevState.control) {
                const char* ctrlStr = "Unknown";
                switch (vehScoring.mControl) {
                    case -1: ctrlStr = "Nobody"; break;
                    case 0: ctrlStr = "Player"; break;
                    case 1: ctrlStr = "AI"; break;
                    case 2: ctrlStr = "Remote"; break;
                    case 3: ctrlStr = "Replay"; break;
                }
                Logger::Get().LogFile("[Transition] Control: %d -> %d (%s)",
                    m_prevState.control, vehScoring.mControl, ctrlStr);
                m_prevState.control = vehScoring.mControl;
            }

            if (vehScoring.mPitState != m_prevState.pitState) {
                const char* pitStr = "None";
                switch (vehScoring.mPitState) {
                    case 0: pitStr = "None"; break;
                    case 1: pitStr = "Request"; break;
                    case 2: pitStr = "Entering"; break;
                    case 3: pitStr = "Stopped"; break;
                    case 4: pitStr = "Exiting"; break;
                }
                Logger::Get().LogFile("[Transition] PitState: %d -> %d (%s)",
                    m_prevState.pitState, vehScoring.mPitState, pitStr);
                m_prevState.pitState = vehScoring.mPitState;
            }

            if (strcmp(vehScoring.mVehicleName, m_prevState.vehicleName) != 0) {
                Logger::Get().LogFile("[Transition] Vehicle: '%s' -> '%s'", m_prevState.vehicleName, vehScoring.mVehicleName);
                strncpy(m_prevState.vehicleName, vehScoring.mVehicleName, 63);
            }

            // 7. Steering Range
            float currentRange = current.telemetry.telemInfo[idx].mPhysicalSteeringWheelRange;
            if (std::abs(currentRange - m_prevState.steeringRange) > 0.001f) {
                Logger::Get().LogFile("[Transition] SteeringRange: %.2f -> %.2f", m_prevState.steeringRange, currentRange);
                m_prevState.steeringRange = currentRange;
            }
        }
    }
}
