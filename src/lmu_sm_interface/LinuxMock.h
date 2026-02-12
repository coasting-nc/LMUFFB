#ifndef LINUXMOCK_H
#define LINUXMOCK_H

#ifndef _WIN32
#include <chrono>
#include <optional>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>
#include <vector>
#include <string>

// Dummy typedefs for Linux compatibility
using DWORD = uint32_t;
using HANDLE = void*;
using HWND = void*;
using BOOL = int;
using UINT = unsigned int;
using LONG = long;
using SHORT = short;
using VOID = void;
using PVOID = void*;
using UINT_PTR = unsigned long;
using LPARAM = long;
using WPARAM = unsigned long;
using LRESULT = long;
using HRESULT = long;
using WORD = uint16_t;
using BYTE = uint8_t;
using LONG_PTR = intptr_t;

typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID;

#ifndef __cdecl
#define __cdecl
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// Windows Constants for Mocking
#define INVALID_HANDLE_VALUE ((HANDLE)(UINT_PTR)-1)
#define PAGE_READWRITE 0x04
#define FILE_MAP_ALL_ACCESS 0xF001F
#define FILE_MAP_READ 0x04
#define ERROR_ALREADY_EXISTS 183
#define WAIT_OBJECT_0 0
#define WAIT_FAILED 0xFFFFFFFF
#define INFINITE 0xFFFFFFFF
#define SYNCHRONIZE 0x00100000L
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000

// Memory Mapping Mock (Global Storage)
namespace MockSM {
    inline std::map<std::string, std::vector<uint8_t>>& GetMaps() {
        static std::map<std::string, std::vector<uint8_t>> maps;
        return maps;
    }
    inline DWORD& LastError() {
        static DWORD err = 0;
        return err;
    }
}

// Interlocked functions for Linux mocking
inline long InterlockedCompareExchange(long volatile* Destination, long Exchange, long Comparand) {
    long old = *Destination;
    if (old == Comparand) *Destination = Exchange;
    return old;
}
inline long InterlockedIncrement(long volatile* Addend) { return ++(*Addend); }
inline long InterlockedDecrement(long volatile* Addend) { return --(*Addend); }
inline long InterlockedExchange(long volatile* Target, long Value) {
    long old = *Target;
    *Target = Value;
    return old;
}

// Memory and Event mocks
inline void YieldProcessor() {}
inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) { return WAIT_OBJECT_0; }
inline BOOL SetEvent(HANDLE hEvent) { return TRUE; }
inline BOOL CloseHandle(HANDLE hObject) {
    if (hObject != (HANDLE)0 && hObject != (HANDLE)1 && hObject != (HANDLE)(intptr_t)-1) {
        // We could delete the string* here, but for mocks it's fine to leak a bit in tests
    }
    return TRUE;
}
inline DWORD GetLastError() { return MockSM::LastError(); }

// Shared Memory Mocks
inline HANDLE CreateFileMappingA(HANDLE hFile, void* lpAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, const char* lpName) {
    if (lpName == nullptr) return (HANDLE)1;
    std::string name(lpName);
    auto& maps = MockSM::GetMaps();
    if (maps.find(name) == maps.end()) {
        maps[name].resize(dwMaximumSizeLow);
        MockSM::LastError() = 0;
    } else {
        MockSM::LastError() = ERROR_ALREADY_EXISTS;
    }
    return (HANDLE)new std::string(name);
}

inline HANDLE OpenFileMappingA(DWORD dwDesiredAccess, BOOL bInheritHandle, const char* lpName) {
    if (lpName == nullptr) return nullptr;
    std::string name(lpName);
    auto& maps = MockSM::GetMaps();
    if (maps.find(name) != maps.end()) {
        return (HANDLE)new std::string(name);
    }
    return nullptr;
}

inline void* MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, size_t dwNumberOfBytesToMap) {
    if (hFileMappingObject == (HANDLE)1 || hFileMappingObject == nullptr || hFileMappingObject == INVALID_HANDLE_VALUE) return nullptr;
    std::string* name = (std::string*)hFileMappingObject;
    return MockSM::GetMaps()[*name].data();
}

inline BOOL UnmapViewOfFile(const void* lpBaseAddress) { return TRUE; }
inline HANDLE CreateEventA(void* lpEventAttributes, BOOL bManualReset, BOOL bInitialState, const char* lpName) { return (HANDLE)1; }

// Window mocks
inline HWND GetConsoleWindow() { return (HWND)1; }
inline BOOL IsWindow(HWND hWnd) { return hWnd != nullptr; }

#endif // _WIN32

#endif // LINUXMOCK_H
