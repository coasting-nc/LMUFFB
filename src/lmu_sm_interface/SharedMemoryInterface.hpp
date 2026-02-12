#pragma once
#include "InternalsPlugin.hpp"

/*
* Usage example:

int main(int argc, char* argv[])
{
    int retVal = 0;
    if (argc < 2) {
        std::cerr << "Usage: child.exe <LMU-pid>\n";
        return 1;
    }
    // Get the LMU Handle
    DWORD parentPid = 0;
    try {
        parentPid = static_cast<DWORD>(std::stoul(argv[1]));
    }
    catch (...) {
        std::cerr << "Invalid parent PID argument.\n";
        return 1;
    }
    auto smLock = SharedMemoryLock::MakeSharedMemoryLock();
    if (!smLock.has_value()) {
        std::cerr << "Cannot initialize SharedMemoryLock.\n";
        return 1;
    }
    static SharedMemoryObjectOut copiedMem;
    // Try to open a handle to the parent process with SYNCHRONIZE right.
    // SYNCHRONIZE is enough to wait on the process handle for exit.
    HANDLE hParent = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentPid);
    HANDLE hEvent = OpenEventA(SYNCHRONIZE, FALSE, "LMU_Data_Event");
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"LMU_Data");
    if (hParent && hEvent && hMapFile) {
        if (SharedMemoryLayout* pBuf = (SharedMemoryLayout*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryLayout))) {
            HANDLE objectHandlesArray[2] = { hParent, hEvent };
            for (DWORD waitObject = WaitForMultipleObjects(2, objectHandlesArray, FALSE, INFINITE); waitObject != WAIT_OBJECT_0; waitObject = WaitForMultipleObjects(2, objectHandlesArray, FALSE, INFINITE)) {
                if (waitObject == WAIT_OBJECT_0 + 1) {
                    smLock->Lock();
                    CopySharedMemoryObj(copiedMem, pBuf->data);
                    smLock->Unlock();
                    // >>>>> ProcessSharedMemory(copiedMem); <<<<<<
                }
                else {
                    std::cerr << "Wait failed: " << GetLastError() << "\n";
                    break;
                }
            }
            UnmapViewOfFile(pBuf);
        }
        else {
            std::cerr << "Could not map view of file. Error: " << GetLastError() << std::endl;
            retVal = 1;
        }
    }
    else {
        std::cerr << "Something went wrong durin initialization. Error: " << GetLastError() << std::endl;
        retVal = 1;
    }
    if (hMapFile)
        CloseHandle(hMapFile);
    if (hEvent)
        CloseHandle(hEvent);
    if (hParent)
        CloseHandle(hParent);

    return retVal;
}

*/

#ifdef _WIN32
#include <windows.h>
#elif defined(HEADLESS_GUI)
#include "LinuxMock.h"
#endif

#define LMU_SHARED_MEMORY_FILE "LMU_Data"
#define LMU_SHARED_MEMORY_EVENT "LMU_Data_Event"
enum SharedMemoryEvent : uint32_t {
    SME_ENTER,
    SME_EXIT,
    SME_STARTUP,
    SME_SHUTDOWN,
    SME_LOAD,
    SME_UNLOAD,
    SME_START_SESSION,
    SME_END_SESSION,
    SME_ENTER_REALTIME,
    SME_EXIT_REALTIME,
    SME_UPDATE_SCORING,
    SME_UPDATE_TELEMETRY,
    SME_INIT_APPLICATION,
    SME_UNINIT_APPLICATION,
    SME_SET_ENVIRONMENT,
    SME_FFB,
    SME_MAX
};

class SharedMemoryLock {
public:
    static std::optional<SharedMemoryLock> MakeSharedMemoryLock() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
        SharedMemoryLock memoryLock;
        if (memoryLock.Init()) {
            return std::move(memoryLock);
        }
#endif
        return std::nullopt;
    }
    bool Lock(DWORD dwMilliseconds = 0xFFFFFFFF) {
#if defined(_WIN32) || defined(HEADLESS_GUI)
        auto* data = (LockData*)mDataPtr;
        int MAX_SPINS = 4000;
        for (int spins = 0; spins < MAX_SPINS; ++spins) {
            if (InterlockedCompareExchange(&data->busy, 1, 0) == 0)
                return true;
            YieldProcessor(); // CPU pause hint
        }
        InterlockedIncrement(&data->waiters);
        while (true) {
            if (InterlockedCompareExchange(&data->busy, 1, 0) == 0) {
                InterlockedDecrement(&data->waiters);
                return true;
            }
            return WaitForSingleObject(mWaitEventHandle, dwMilliseconds) == 0; // WAIT_OBJECT_0
        }
#else
        return false;
#endif
    }
    void Unlock() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
        auto* data = (LockData*)mDataPtr;
        InterlockedExchange(&data->busy, 0);
        if (data->waiters > 0) {
            SetEvent(mWaitEventHandle);
        }
#endif
    }
    void Reset() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
        auto* data = (LockData*)mDataPtr;
        data->waiters = 0;
        data->busy = 0;
#endif
    }
    ~SharedMemoryLock() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
        if (mWaitEventHandle)
            CloseHandle(mWaitEventHandle);
        if (mMapHandle)
            CloseHandle(mMapHandle);
        if (mDataPtr)
            UnmapViewOfFile(mDataPtr);
#endif
    }
    SharedMemoryLock(SharedMemoryLock&& other) : mMapHandle(std::exchange(other.mMapHandle, nullptr)), mWaitEventHandle(std::exchange(other.mWaitEventHandle, nullptr)) ,
        mDataPtr(std::exchange(other.mDataPtr, nullptr)) {}
    SharedMemoryLock& operator=(SharedMemoryLock&& other) {
        std::swap(mMapHandle, other.mMapHandle);
        std::swap(mWaitEventHandle, other.mWaitEventHandle);
        std::swap(mDataPtr, other.mDataPtr);
        return *this;
    }
private:
#if defined(_WIN32) || defined(HEADLESS_GUI)
    struct LockData {
        volatile LONG waiters;
        volatile LONG busy;
    };
#endif
    SharedMemoryLock() = default;
    bool Init() {
#if defined(_WIN32) || defined(HEADLESS_GUI)
        mMapHandle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)sizeof(LockData), "LMU_SharedMemoryLockData");
        if (!mMapHandle) {
            return false;
        }
        mDataPtr = (LockData*)MapViewOfFile(mMapHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LockData));
        if (!mDataPtr) {
            CloseHandle(mMapHandle);
            return false;
        }
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            Reset();
        }
        mWaitEventHandle = CreateEventA(NULL, FALSE, FALSE, "LMU_SharedMemoryLockEvent");
        if (!mWaitEventHandle) {
            UnmapViewOfFile(mDataPtr);
            CloseHandle(mMapHandle);
            return false;
        }
        return true;
#else
        return false;
#endif
    }
    HANDLE mMapHandle = NULL;
    HANDLE mWaitEventHandle = NULL;
    void* mDataPtr = nullptr;
};

struct SharedMemoryScoringData { // Remember to check CopySharedMemoryObj still works properly when updating this struct
    ScoringInfoV01 scoringInfo;
    size_t scoringStreamSize;
    VehicleScoringInfoV01 vehScoringInfo[104]; // MUST NOT BE MOVED!
    char scoringStream[65536];
};

struct SharedMemoryTelemtryData { // Remember to check CopySharedMemoryObj still works properly when updating this struct
    uint8_t activeVehicles;
    uint8_t playerVehicleIdx;
    bool playerHasVehicle;
    TelemInfoV01 telemInfo[104];
};

struct SharedMemoryPathData {
    char userData[MAX_PATH];
    char customVariables[MAX_PATH];
    char stewardResults[MAX_PATH];
    char playerProfile[MAX_PATH];
    char pluginsFolder[MAX_PATH];
};

struct SharedMemoryGeneric {
    SharedMemoryEvent events[SharedMemoryEvent::SME_MAX];
    long gameVersion;
    float FFBTorque;
    ApplicationStateV01 appInfo;
};

struct SharedMemoryObjectOut { // Remember to check CopySharedMemoryObj still works properly when updating this struct
    SharedMemoryGeneric generic;
    SharedMemoryPathData paths;
    SharedMemoryScoringData scoring;
    SharedMemoryTelemtryData telemetry;
};

struct SharedMemoryLayout {
    SharedMemoryObjectOut data;
};

static void CopySharedMemoryObj(SharedMemoryObjectOut& dst, SharedMemoryObjectOut& src) {
    memcpy(&dst.generic, &src.generic, sizeof(SharedMemoryGeneric));
    if (src.generic.events[SME_UPDATE_SCORING]) {
        memcpy(&dst.scoring.scoringInfo, &src.scoring.scoringInfo, sizeof(ScoringInfoV01));
        memcpy(&dst.scoring.vehScoringInfo, &src.scoring.vehScoringInfo, src.scoring.scoringInfo.mNumVehicles * sizeof(VehicleScoringInfoV01));
        memcpy(&dst.scoring.scoringStream, &src.scoring.scoringStream, src.scoring.scoringStreamSize);
        dst.scoring.scoringStreamSize = src.scoring.scoringStreamSize;
        dst.scoring.scoringStream[dst.scoring.scoringStreamSize] = '\0';
        dst.scoring.scoringInfo.mVehicle = &dst.scoring.vehScoringInfo[0];
        dst.scoring.scoringInfo.mResultsStream = &dst.scoring.scoringStream[0];
    }
    if (src.generic.events[SME_UPDATE_TELEMETRY]) {
        dst.telemetry.activeVehicles = src.telemetry.activeVehicles;
        dst.telemetry.playerHasVehicle = src.telemetry.playerHasVehicle;
        dst.telemetry.playerVehicleIdx = src.telemetry.playerVehicleIdx;
        memcpy(&dst.telemetry.telemInfo, &src.telemetry.telemInfo, src.telemetry.activeVehicles * sizeof(TelemInfoV01));
    }
    if (src.generic.events[SME_ENTER] || src.generic.events[SME_EXIT] || src.generic.events[SME_SET_ENVIRONMENT]) {
        memcpy(&dst.paths, &src.paths, sizeof(SharedMemoryPathData));
    }
}