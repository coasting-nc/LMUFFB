#ifndef WINDOWS_H_MOCK
#define WINDOWS_H_MOCK

#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <string>

typedef void* HWND;
typedef uint32_t DWORD;
typedef void* HANDLE;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HDC;
typedef void* HBITMAP;
typedef long LRESULT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef float FLOAT;
typedef int BOOL;
typedef long LONG;

#define TRUE 1
#define FALSE 0

#define __cdecl
#define CALLBACK
#define WINAPI

struct GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
};

#define _TRUNCATE ((size_t)-1)
#define MAX_PATH 260
#define INFINITE 0xFFFFFFFF

inline int strncpy_s(char* dest, size_t dest_size, const char* src, size_t count) {
    if (src == nullptr || dest == nullptr || dest_size == 0) return 1;
    size_t to_copy = (count == _TRUNCATE) ? dest_size - 1 : count;
    to_copy = std::min(to_copy, strlen(src));
    to_copy = std::min(to_copy, dest_size - 1);

    memcpy(dest, src, to_copy);
    dest[to_copy] = '\0';
    return 0;
}

template <size_t size>
inline int strncpy_s(char (&dest)[size], const char* src, size_t count) {
    return strncpy_s(dest, size, src, count);
}

// Interlocked functions
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

// Events and Wait
#define WAIT_OBJECT_0 0
inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) { return WAIT_OBJECT_0; }
inline BOOL SetEvent(HANDLE hEvent) { return TRUE; }
inline BOOL CloseHandle(HANDLE hObject) { return TRUE; }

// Memory Mapping
#define PAGE_READWRITE 0x04
#define FILE_MAP_ALL_ACCESS 0xF001F
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
inline HANDLE CreateFileMappingA(HANDLE hFile, void* lpAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, const char* lpName) { return (HANDLE)1; }
inline void* MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, size_t dwNumberOfBytesToMap) { return (void*)1; }
inline BOOL UnmapViewOfFile(const void* lpBaseAddress) { return TRUE; }
inline HANDLE CreateEventA(void* lpEventAttributes, BOOL bManualReset, BOOL bInitialState, const char* lpName) { return (HANDLE)1; }
inline DWORD GetLastError() { return 0; }
#define ERROR_ALREADY_EXISTS 183L

// Processor
inline void YieldProcessor() {}

#endif
