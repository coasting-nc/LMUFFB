#pragma once

// FIX: LMU Plugin Update 2025
// The official PluginObjects.hpp provided by Studio 397 is missing 
// windows.h include which is required for some of the types/definitions used.
// We include it here BEFORE including the vendor file.

#ifdef _WIN32
#include <windows.h>
#else
#include "LinuxMock.h"
#endif

// Include the official vendor file
#include "PluginObjects.hpp"
