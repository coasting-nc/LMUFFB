#ifndef LINUXMOCK_H
#define LINUXMOCK_H

#ifndef _WIN32
#include <chrono>
#include <optional>
#include <cstdint>

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

#endif // _WIN32

#endif // LINUXMOCK_H
