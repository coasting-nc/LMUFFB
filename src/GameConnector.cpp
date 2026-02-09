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
#ifdef _WIN32
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
#endif
    m_smLock.reset();
    m_connected = false;
    m_processId = 0;
}

bool GameConnector::TryConnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connected) return true;

    // Ensure we don't leak handles from a previous partial/failed attempt
    _DisconnectLocked();

#ifdef _WIN32
    m_hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, LMU_SHARED_MEMORY_FILE);
    
    if (m_hMapFile == NULL) {
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

    HWND hwnd = m_pSharedMemLayout->data.generic.appInfo.mAppWindow;
    if (hwnd) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid) {
            m_processId = pid;
            m_hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!m_hProcess) {
                std::cout << "[GameConnector] Note: Failed to open process handle (Error: " << GetLastError() << ")." << std::endl;
            }
        }
    }

    m_connected = true;
    m_lastUpdateLocalTime = std::chrono::steady_clock::now();
    std::cout << "[GameConnector] Connected to LMU Shared Memory." << std::endl;
    return true;
#else
    return false;
#endif
}

bool GameConnector::CheckLegacyConflict() {
#ifdef _WIN32
    HANDLE hLegacy = OpenFileMappingA(FILE_MAP_READ, FALSE, LEGACY_SHARED_MEMORY_NAME);
    if (hLegacy) {
        std::cout << "[Warning] Legacy rFactor 2 Shared Memory Plugin detected. This may conflict with LMU 1.2 data." << std::endl;
        CloseHandle(hLegacy);
        return true;
    }
#endif
    return false;
}

bool GameConnector::IsConnected() const {
  if (!m_connected.load(std::memory_order_acquire)) return false;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_connected.load(std::memory_order_relaxed)) return false;

#ifdef _WIN32
  if (m_hProcess) {
    DWORD wait = WaitForSingleObject(m_hProcess, 0);
    if (wait == WAIT_OBJECT_0 || wait == WAIT_FAILED) {
      const_cast<GameConnector*>(this)->_DisconnectLocked();
      return false;
    }
  }
#endif

  return m_connected.load(std::memory_order_relaxed) && m_pSharedMemLayout && m_smLock.has_value();
}

bool GameConnector::CopyTelemetry(SharedMemoryObjectOut& dest) {
    if (!m_connected.load(std::memory_order_acquire)) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected.load(std::memory_order_relaxed) || !m_pSharedMemLayout || !m_smLock.has_value()) return false;

    if (m_smLock->Lock(50)) {
        CopySharedMemoryObj(dest, m_pSharedMemLayout->data);
        
        if (dest.telemetry.playerHasVehicle) {
            uint8_t idx = dest.telemetry.playerVehicleIdx;
            if (idx < 104) {
                double currentET = dest.telemetry.telemInfo[idx].mElapsedTime;
                if (currentET != m_lastElapsedTime) {
                    m_lastElapsedTime = currentET;
                    m_lastUpdateLocalTime = std::chrono::steady_clock::now();
                }
            }
        } else {
            m_lastUpdateLocalTime = std::chrono::steady_clock::now();
        }

        bool isRealtime = (m_pSharedMemLayout->data.scoring.scoringInfo.mInRealtime != 0);
        m_smLock->Unlock();
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
