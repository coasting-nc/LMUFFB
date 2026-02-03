#include "GameConnector.h"
#include "lmu_sm_interface/SafeSharedMemoryLock.h"
#include <iostream>

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
    std::lock_guard<std::mutex> lock(m_mutex);
    _DisconnectLocked();
}

void GameConnector::_DisconnectLocked() {
    if (m_pSharedMemLayout) {
        UnmapViewOfFile(m_pSharedMemLayout);
        m_pSharedMemLayout = nullptr;
    }
    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
    }
    if (m_hProcess) {
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }
    m_smLock.reset();
    m_connected = false;
    m_processId = 0;
}

bool GameConnector::TryConnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connected) return true;

    // Ensure we don't leak handles from a previous partial/failed attempt
    _DisconnectLocked();

    m_hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, LMU_SHARED_MEMORY_FILE);
    
    if (m_hMapFile == NULL) {
        // Not running yet
        return false;
    } 

    m_pSharedMemLayout = (SharedMemoryLayout*)MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, sizeof(SharedMemoryLayout));
    if (m_pSharedMemLayout == NULL) {
        std::cerr << "[GameConnector] Could not map view of file." << std::endl;
        _DisconnectLocked();
        return false;
    }

    m_smLock = SafeSharedMemoryLock::MakeSafeSharedMemoryLock();
    if (!m_smLock.has_value()) {
        std::cerr << "[GameConnector] Failed to init LMU Shared Memory Lock" << std::endl;
        _DisconnectLocked();
        return false;
    }

    // Try to get process handle for lifecycle management
    // But don't treat missing handle as fatal - allow connection to proceed
    HWND hwnd = m_pSharedMemLayout->data.generic.appInfo.mAppWindow;
    if (hwnd) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid) {
            m_processId = pid;
            m_hProcess = OpenProcess(
                SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!m_hProcess) {
                std::cout << "[GameConnector] Note: Failed to open process handle (Error: " << GetLastError() << "). Connection will continue without lifecycle monitoring." << std::endl;
            }
        }
    } else {
        std::cout << "[GameConnector] Note: Window handle not yet available. Will retry process handle acquisition later." << std::endl;
    }

    // Mark as connected even if process handle isn't available yet
    // Process monitoring is optional - core functionality is shared memory access
    m_connected = true;
    std::cout << "[GameConnector] Connected to LMU Shared Memory." << std::endl;
    return true;
}

bool GameConnector::CheckLegacyConflict() {
    HANDLE hLegacy = OpenFileMappingA(FILE_MAP_READ, FALSE, LEGACY_SHARED_MEMORY_NAME);
    if (hLegacy) {
        std::cout << "[Warning] Legacy rFactor 2 Shared Memory Plugin detected. This may conflict with LMU 1.2 data." << std::endl;
        CloseHandle(hLegacy);
        return true;
    }
    return false;
}

bool GameConnector::IsConnected() const {
  if (!m_connected.load(std::memory_order_acquire)) return false;

  std::lock_guard<std::mutex> lock(m_mutex);
  // Double check under lock to ensure we don't access closed handles
  if (!m_connected.load(std::memory_order_relaxed)) return false;

  // Check if the game process is still running
  if (m_hProcess) {
    DWORD wait = WaitForSingleObject(m_hProcess, 0);
    if (wait == WAIT_OBJECT_0 || wait == WAIT_FAILED) {
      // Process has exited or handle is invalid - clean up everything immediately
      const_cast<GameConnector*>(this)->_DisconnectLocked();
      return false;
    }
  }

  return m_connected.load(std::memory_order_relaxed) && m_pSharedMemLayout &&
         m_smLock.has_value();
}

bool GameConnector::CopyTelemetry(SharedMemoryObjectOut& dest) {
    // Fast path check
    if (!m_connected.load(std::memory_order_acquire)) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected.load(std::memory_order_relaxed) || !m_pSharedMemLayout || !m_smLock.has_value()) return false;

    // Use a reasonable timeout (e.g., 50ms) to avoid hanging forever if the game crashes while holding the lock
    if (m_smLock->Lock(50)) {
        CopySharedMemoryObj(dest, m_pSharedMemLayout->data);
        
        // Get realtime status while we have the lock
        // mInRealtime: 0=menu/replay/monitor, 1=driving/practice/race
        bool isRealtime = (m_pSharedMemLayout->data.scoring.scoringInfo.mInRealtime != 0);
        
        m_smLock->Unlock();
        
        return isRealtime;
    } else {
        // Timeout - game may have crashed while holding lock
        // Return false to indicate not in realtime (safe fallback)
        return false;
    }
}
