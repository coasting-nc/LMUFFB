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
#include <cstdarg>
#include <cstdio>

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
using ULONG_PTR = uintptr_t;
using LPDWORD = uint32_t*;
using HMODULE = void*;
using HICON = void*;
using HRSRC = void*;
using LPVOID = void*;
using LPCSTR = const char*;

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

#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
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

// Window Styles & Constants
#define WS_OVERLAPPEDWINDOW 0
#define WS_VISIBLE 0
#define GWL_EXSTYLE -20
#define WS_EX_TOPMOST 0x00000008L
#define HWND_TOPMOST ((HWND)-1)
#define HWND_NOTOPMOST ((HWND)-2)
#define SWP_NOMOVE 0x0002
#define SWP_NOSIZE 0x0001
#define SWP_FRAMECHANGED 0x0020

// Resources
#define RT_GROUP_ICON 14
#define LOAD_LIBRARY_AS_DATAFILE 0x00000002
#define MAKEINTRESOURCE(i) ((char*)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCEA(i) ((char*)((ULONG_PTR)((WORD)(i))))

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
inline int sprintf_s(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}
inline int sprintf_s(char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    va_end(args);
    return ret;
}
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
namespace MockGUI {
    inline LONG_PTR& ExStyle() { static LONG_PTR style = 0; return style; }
}

inline HWND GetConsoleWindow() { return (HWND)1; }
inline BOOL IsWindow(HWND hWnd) {
    return (hWnd == (HWND)1 || hWnd == (HWND)2); // Accept our mock handles
}
inline HWND CreateWindowA(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, void* hMenu, HMODULE hInstance, void* lpParam) {
    return (HWND)2;
}
inline LONG_PTR GetWindowLongPtr(HWND hWnd, int nIndex) {
    if (nIndex == GWL_EXSTYLE) return MockGUI::ExStyle();
    return 0;
}
inline BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
    if (hWndInsertAfter == HWND_TOPMOST) MockGUI::ExStyle() |= WS_EX_TOPMOST;
    if (hWndInsertAfter == HWND_NOTOPMOST) MockGUI::ExStyle() &= ~WS_EX_TOPMOST;
    return TRUE;
}
inline BOOL DestroyWindow(HWND hWnd) { return TRUE; }

// Resource Mocks
inline HMODULE GetModuleHandle(const char* lpModuleName) { return (HMODULE)1; }
inline HICON LoadIcon(HMODULE hInstance, const char* lpIconName) { return (HICON)1; }
inline DWORD GetModuleFileNameA(HMODULE hModule, char* lpFilename, DWORD nSize) {
    strncpy(lpFilename, "LMUFFB.exe", nSize);
    return strlen(lpFilename);
}
inline HMODULE LoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags) { return (HMODULE)1; }
inline HRSRC FindResourceA(HMODULE hModule, const char* lpName, const char* lpType) { return (HRSRC)1; }
inline BOOL FreeLibrary(HMODULE hLibModule) { return TRUE; }

// Version Info Mocks
inline DWORD GetFileVersionInfoSizeA(const char* lptstrFilename, LPDWORD lpdwHandle) {
    if (lpdwHandle) *lpdwHandle = 0;
    return 1024;
}
inline BOOL GetFileVersionInfoA(const char* lptstrFilename, DWORD dwHandle, DWORD dwLen, void* lpData) {
    memset(lpData, 0, dwLen);
    return TRUE;
}
inline BOOL VerQueryValueA(const void* pBlock, const char* lpSubBlock, void** lplpBuffer, UINT* puLen) {
    static uint16_t translation[2] = { 0x0409, 0x04B0 }; // English (US), Unicode
    if (strstr(lpSubBlock, "Translation")) {
        *lplpBuffer = &translation;
        if (puLen) *puLen = sizeof(translation);
        return TRUE;
    }
    if (strstr(lpSubBlock, "CompanyName")) {
        *lplpBuffer = (void*)"lmuFFB";
        if (puLen) *puLen = 7;
        return TRUE;
    }
    if (strstr(lpSubBlock, "ProductVersion")) {
        *lplpBuffer = (void*)"0.7.79";
        if (puLen) *puLen = 7;
        return TRUE;
    }
    return FALSE;
}

#endif // _WIN32

#endif // LINUXMOCK_H
