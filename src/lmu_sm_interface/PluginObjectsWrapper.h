#pragma once

// FIX: LMU Plugin Update 2025
// The official PluginObjects.hpp provided by Studio 397 is missing 
// windows.h include which is required for some of the types/definitions used.
// We include it here BEFORE including the vendor file.

#ifdef _WIN32
#include <windows.h>
#else
// Dummy typedefs for Linux compatibility
using DWORD = unsigned long;
using HANDLE = void*;
using HWND = void*;
using BOOL = int;
using UINT = unsigned int;
using LONG = long;
using UINT_PTR = unsigned long;
#ifndef __cdecl
#define __cdecl
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#endif

// Include the official vendor file
#include "PluginObjects.hpp"
