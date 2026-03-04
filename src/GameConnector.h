#ifndef GAMECONNECTOR_H
#define GAMECONNECTOR_H

#ifdef _WIN32
#include <windows.h>
#else
#include "lmu_sm_interface/LinuxMock.h"
#endif

#include "lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "lmu_sm_interface/SafeSharedMemoryLock.h"
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
    } m_prevState;

    void CheckTransitions(const SharedMemoryObjectOut& current);

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

    // Heartbeat for staleness detection (v0.7.15)
    double m_lastElapsedTime = -1.0;
    double m_lastSteering = 0.0; // Issue #184: Steering heartbeat
    mutable std::chrono::steady_clock::time_point m_lastUpdateLocalTime;

    void _DisconnectLocked();
};
#endif // GAMECONNECTOR_H
