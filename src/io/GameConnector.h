#ifndef GAMECONNECTOR_H
#define GAMECONNECTOR_H

#ifdef _WIN32
#include <windows.h>
#else
#include "io/lmu_sm_interface/LinuxMock.h"
#endif

#include "io/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "io/lmu_sm_interface/SafeSharedMemoryLock.h"
#include <mutex>
#include <atomic>
#include <cstring>

namespace FFBEngineTests { class GameConnectorTestAccessor; }

class GameConnector {
public:
    static GameConnector& Get();
    
    // Attempt to connect to LMU Shared Memory
    bool TryConnect();
    
    // Disconnect and clean up resources
    void Disconnect();
    
    // Check for Legacy rFactor 2 Plugin conflict
    bool CheckLegacyConflict();
    
    // Is connected to LMU SM?
    bool IsConnected() const;
    
    // Thread-safe copy of telemetry data
    // Returns true if in realtime (driving) mode, false if in menu/replay
    bool CopyTelemetry(SharedMemoryObjectOut& dest);

    // Returns true if telemetry data hasn't changed for more than timeout (v0.7.15)
    bool IsStale(long timeoutMs = 100) const;

    // Robust State Accessors (#267)
    bool IsSessionActive() const { return m_sessionActive.load(std::memory_order_relaxed); }
    bool IsInRealtime() const { return m_inRealtime.load(std::memory_order_relaxed); }
    long GetSessionType() const { return m_currentSessionType.load(std::memory_order_relaxed); }
    unsigned char GetGamePhase() const { return m_currentGamePhase.load(std::memory_order_relaxed); }
    signed char GetPlayerControl() const { return m_playerControl.load(std::memory_order_relaxed); }

    // Composite predicate: true only when the player is physically behind the wheel
    // and actively in control (not paused, not in garage UI, not AI-controlled).
    // Addresses the ESC-menu-while-on-track edge case (session transition.md).
    bool IsPlayerActivelyDriving() const {
        return m_inRealtime.load(std::memory_order_relaxed)
            && m_playerControl.load(std::memory_order_relaxed) == 0  // Player (not AI/replay/nobody)
            && m_currentGamePhase.load(std::memory_order_relaxed) != 9; // 9 == Paused
    }

private:
    struct TransitionState {
        unsigned char optionsLocation = 255;
        bool inRealtime = false;
        unsigned char gamePhase = 255;
        long session = -1;
        signed char control = -2;
        unsigned char pitState = 255;
        char vehicleName[64] = { 0 };
        char trackName[64] = { 0 };
        char optionsPage[32] = { 0 };
        float steeringRange = -1.0f;
        bool playerHasVehicle = false;  // investigation: quit-to-menu detection (#7.4a)
        int  numVehicles = -1;          // investigation: quit-to-menu detection (#7.4b)
        uint32_t eventState[SME_MAX] = { 0 };
        std::chrono::steady_clock::time_point lastEventLogTime[SME_MAX];
    } m_prevState;

    // CheckTransitions orchestrates the two-phase update:
    //   1. _UpdateStateFromSnapshot  — unconditionally syncs atomics from the SM buffer
    //   2. _LogTransitions           — detects changes vs m_prevState and emits log lines
    void CheckTransitions(const SharedMemoryObjectOut& current);
    void _UpdateStateFromSnapshot(const SharedMemoryObjectOut& current);
    void _LogTransitions(const SharedMemoryObjectOut& current);

    // String-lookup helpers (extracted for reuse and testability)
    static const char* SmeEventName(int eventIndex);
    static const char* GamePhaseName(unsigned char phase);
    static const char* SessionTypeName(long session);
    static const char* ControlModeName(signed char control);
    static const char* PitStateName(unsigned char pitState);

    friend class FFBEngineTests::GameConnectorTestAccessor;

    GameConnector();
    ~GameConnector();
    
    SharedMemoryLayout* m_pSharedMemLayout = nullptr;
    mutable std::optional<SafeSharedMemoryLock> m_smLock;
    HANDLE m_hMapFile = NULL;
    mutable HWND m_hwndGame = NULL;
    DWORD m_processId = 0;

    std::atomic<bool> m_connected{false};
    mutable std::recursive_mutex m_mutex;

    // Robust State Machine (#267)
    std::atomic<bool> m_sessionActive{ false };
    std::atomic<bool> m_inRealtime{ false };
    std::atomic<long> m_currentSessionType{ -1 };
    std::atomic<unsigned char> m_currentGamePhase{ 255 };
    std::atomic<signed char> m_playerControl{ -2 };

    // Heartbeat for staleness detection (v0.7.15)
    double m_lastElapsedTime = -1.0;
    double m_lastSteering = 0.0; // Issue #184: Steering heartbeat
    mutable std::chrono::steady_clock::time_point m_lastUpdateLocalTime;

    // Quit-to-menu detection (#7.5): armed when mInRealtime goes true→false.
    // If SME_ENTER fires within the deadline window, we know the user quit to
    // the main menu (not just returned to the garage monitor).
    bool m_pendingMenuCheck = false;
    std::chrono::steady_clock::time_point m_menuCheckDeadline;

    void _DisconnectLocked();
};
#endif // GAMECONNECTOR_H
