#ifndef GAMECONNECTOR_H
#define GAMECONNECTOR_H

#include "lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "lmu_sm_interface/SafeSharedMemoryLock.h"
#include <windows.h>
#include <mutex>
#include <atomic>

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

private:
    GameConnector();
    ~GameConnector();
    
    SharedMemoryLayout* m_pSharedMemLayout = nullptr;
    mutable std::optional<SafeSharedMemoryLock> m_smLock;
    HANDLE m_hMapFile = NULL;
    mutable HANDLE m_hProcess = NULL;
    DWORD m_processId = 0;

    std::atomic<bool> m_connected{false};
    mutable std::mutex m_mutex;

    void _DisconnectLocked();
};
#endif // GAMECONNECTOR_H
