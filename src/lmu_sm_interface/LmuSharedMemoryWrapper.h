#pragma once

// FIX: LMU Plugin Update 2025
// WARNING: DO NOT MODIFY the vendor file SharedMemoryInterface.hpp directly!
// The official SharedMemoryInterface.hpp provided by Studio 397 is missing
// several standard library includes required for compilation.
// We include them here BEFORE including the vendor file to keep it untouched.

#include <optional>  // Required for std::optional
#include <utility>   // Required for std::exchange, std::swap
#include <cstdint>   // Required for uint32_t, uint8_t
#include <cstring>   // Required for memcpy

#ifndef _WIN32
#include "LinuxMock.h"
#endif

#include "InternalsPluginWrapper.h"

// Include the official vendor file
#include "SharedMemoryInterface.hpp"
