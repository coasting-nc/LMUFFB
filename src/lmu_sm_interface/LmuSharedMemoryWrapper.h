#pragma once

// FIX: LMU Plugin Update 2025
// The official SharedMemoryInterface.hpp provided by Studio 397 is missing
// several standard library includes required for compilation.
// We include them here BEFORE including the vendor file.

#include <optional>  // Required for std::optional
#include <utility>   // Required for std::exchange, std::swap
#include <cstdint>   // Required for uint32_t, uint8_t
#include <cstring>   // Required for memcpy
#include "InternalsPluginWrapper.h"

// Include the official vendor file
#include "SharedMemoryInterface.hpp"
