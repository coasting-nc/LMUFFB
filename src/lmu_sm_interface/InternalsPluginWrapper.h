#pragma once

// FIX: LMU Plugin Update 2025
// The official InternalsPlugin.hpp provided by Studio 397 depends on
// PluginObjects.hpp which is missing windows.h.
// We include our wrapper here to ensure dependencies are met.

#include "PluginObjectsWrapper.h"
#include "InternalsPlugin.hpp"
