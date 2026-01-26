#ifndef GAMECONNECTOR_H
#define GAMECONNECTOR_H

#include "lmu_sm_interface/SharedMemoryInterface.hpp"
#include <optional>
#include <windows.h>

class GameConnector {
public:
    static GameConnector& Get();
    
    // Attempt to connect to LMU Shared Memory
    bool TryConnect();
    
    // Check for Legacy rFactor 2 Plugin conflict
    bool CheckLegacyConflict();
    
    // Is connected to LMU SM?
    bool IsConnected() const;
    
    // Is the game in Realtime (Driving) mode?
    bool IsInRealtime() const;
    
    // Thread-safe copy of telemetry data
    void CopyTelemetry(SharedMemoryObjectOut& dest);

private:
    GameConnector();
    ~GameConnector();
    
    SharedMemoryLayout* m_pSharedMemLayout = nullptr;
    mutable std::optional<SharedMemoryLock> m_smLock;
    HANDLE m_hMapFile = NULL;
    mutable HANDLE m_hProcess = NULL;
    DWORD m_processId = 0;

    mutable bool m_connected = false;
};

#endif // GAMECONNECTOR_H
