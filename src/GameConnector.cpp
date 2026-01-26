#include "GameConnector.h"
#include <iostream>

#define LEGACY_SHARED_MEMORY_NAME "$rFactor2SMMP_Telemetry$"

GameConnector& GameConnector::Get() {
    static GameConnector instance;
    return instance;
}

GameConnector::GameConnector() {}

GameConnector::~GameConnector() {
    if (m_pSharedMemLayout) UnmapViewOfFile(m_pSharedMemLayout);
    if (m_hMapFile) CloseHandle(m_hMapFile);
}

bool GameConnector::TryConnect() {
    if (m_connected) return true;

    m_hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, LMU_SHARED_MEMORY_FILE);
    
    if (m_hMapFile == NULL) {
        // Not running yet
        return false;
    } 

    m_pSharedMemLayout = (SharedMemoryLayout*)MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, sizeof(SharedMemoryLayout));
    if (m_pSharedMemLayout == NULL) {
        std::cerr << "[GameConnector] Could not map view of file." << std::endl;
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
        return false;
    }

    m_smLock = SharedMemoryLock::MakeSharedMemoryLock();
    if (!m_smLock.has_value()) {
        std::cerr << "[GameConnector] Failed to init LMU Shared Memory Lock" << std::endl;
        // Proceed anyway? No, lock is mandatory for data integrity in 1.2
        // But maybe we can read without lock if we accept tearing? 
        // No, let's enforce it for robustness.
        // Actually, if lock creation fails, it might mean permissions or severe error.
        return false;
    }

    // Close any stale process handle from a previous connection
    if (m_hProcess) {
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }

    HWND hwnd = m_pSharedMemLayout->data.generic.appInfo.mAppWindow;
    if (hwnd) {
      DWORD pid = 0;
      GetWindowThreadProcessId(hwnd, &pid);
      if (pid) {
        m_processId = pid;
        m_hProcess = OpenProcess(
            SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (m_hProcess) {
          // Successfully obtained a valid process handle - we're truly connected
          m_connected = true;
          std::cout << "[GameConnector] Connected to LMU Shared Memory." << std::endl;
          return true;
        } else {
          std::cout << "[GameConnector] Failed to open process handle. Error: " << GetLastError() << std::endl;
        }
      }
    }

    // If we get here, we have shared memory but no valid process handle
    // Don't mark as connected yet - will try again on next attempt
    return false;
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
  // Check if the game process is still running
  if (m_hProcess) {
    DWORD wait = WaitForSingleObject(m_hProcess, 0);
    if (wait == WAIT_OBJECT_0) {
      // Process has exited - clean up connection state
      m_connected = false;
      return false;
    }
    if (wait == WAIT_FAILED) {
      // Handle is invalid - clean up connection state
      m_connected = false;
      return false;
    }      
  }

  return m_connected && m_pSharedMemLayout &&
         m_smLock.has_value();
}

void GameConnector::CopyTelemetry(SharedMemoryObjectOut& dest) {
    if (!m_connected || !m_pSharedMemLayout || !m_smLock.has_value()) return;

    m_smLock->Lock();
    CopySharedMemoryObj(dest, m_pSharedMemLayout->data);
    m_smLock->Unlock();
}

bool GameConnector::IsInRealtime() const {
    if (!m_connected || !m_pSharedMemLayout || !m_smLock.has_value()) {
        return false;
    }
    
    // Thread-safe check of game state
    m_smLock->Lock();
    
    bool inRealtime = false;
    
    // Find player vehicle and check session state
    for (int i = 0; i < 104; i++) {
        if (m_pSharedMemLayout->data.scoring.vehScoringInfo[i].mIsPlayer) {
            // Check if in active driving session
            // mInRealtime: 0=menu/replay/monitor, 1=driving/practice/race
            inRealtime = (m_pSharedMemLayout->data.scoring.scoringInfo.mInRealtime != 0);
            break;
        }
    }
    
    m_smLock->Unlock();
    return inRealtime;
}
