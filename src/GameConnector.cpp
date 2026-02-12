#include "GameConnector.h"
#include "Logger.h"
#ifndef _WIN32
#include "lmu_sm_interface/LinuxMock.h"
#endif
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
    std::lock_guard<std::mutex> lock(m_mutex);
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
        std::cerr << "[GameConnector] Could not map view of file." << std::endl;
        Logger::Get().LogWin32Error("MapViewOfFile", GetLastError());
        _DisconnectLocked();
        return false;
    }

    m_smLock = SafeSharedMemoryLock::MakeSafeSharedMemoryLock();
    if (!m_smLock.has_value()) {
        std::cerr << "[GameConnector] Failed to init LMU Shared Memory Lock" << std::endl;
        Logger::Get().Log("Failed to init SafeSharedMemoryLock.");
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
    std::cout << "[GameConnector] Connected to LMU Shared Memory." << std::endl;
    Logger::Get().Log("Connected to LMU Shared Memory.");
    return true;
#else
    return false;
#endif
}

bool GameConnector::CheckLegacyConflict() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
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
