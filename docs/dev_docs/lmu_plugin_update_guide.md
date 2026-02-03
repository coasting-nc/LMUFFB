# LMU Plugin Update Guide

This document outlines the procedure for updating the Le Mans Ultimate (LMU) Shared Memory Plugin interface files in `src/lmu_sm_interface/`. These files are provided by Studio 397 (the game developers) and may need periodic updates to support new game versions.

## Files Involved
- `InternalsPlugin.hpp`
- `PluginObjects.hpp`
- `SharedMemoryInterface.hpp`

## Critical Maintenance Note: Wrapper Header Strategy

The project uses a **Wrapper Header** approach to avoid modifying the official vendor files.

### The Problem
As of the 2025 LMU plugin update (v1.2/1.3), `SharedMemoryInterface.hpp` provided by Studio 397 is missing several required standard library includes:
- `<optional>` — Required for `std::optional`
- `<utility>` — Required for `std::exchange`, `std::swap`
- `<cstdint>` — Required for `uint32_t`, `uint8_t`
- `<cstring>` — Required for `memcpy`

Without these includes, compilation fails.

### The Solution
**Do NOT modify `SharedMemoryInterface.hpp` directly.**

Instead, we use `src/lmu_sm_interface/LmuSharedMemoryWrapper.h` to inject the necessary headers **before** including the vendor file.

### Why This Approach?
1. **Preserves Official Files**: Vendor files remain untouched and pristine.
2. **Easy Updates**: Future plugin updates can be dropped in without merge conflicts.
3. **Clear Separation**: Makes it obvious which headers are our fix vs. what came from Studio 397.
4. **Maintainability**: All future developers can see the fix is intentional, not an oversight.

### Usage in Code
Always include the wrapper (never the vendor file directly):
```cpp
#include "lmu_sm_interface/LmuSharedMemoryWrapper.h"
```

## Update Procedure

1.  **Replace Files**: Overwrite the existing files in `src/lmu_sm_interface/` with the new versions from the game SDK folder.
    -   **Source Location**: `Program Files (x86)\Steam\steamapps\common\Le Mans Ultimate\Support\SharedMemoryInterface`
2.  **Do Nothing Else**: You do NOT need to edit `SharedMemoryInterface.hpp` to add includes. The wrapper handles this.
3.  **Compile**: Run a full build to verify compatibility.
4.  **Test**: Run `run_combined_tests` to ensure the interface changes haven't introduced regressions.

## Troubleshooting Future Updates

### New Compilation Errors After Update?
If a future plugin update introduces new compilation errors in `SharedMemoryInterface.hpp`:

1. **Identify the Missing Header**: Read the compiler error carefully. It will typically mention undefined types like `std::vector`, `std::string`, etc.
2. **Add to Wrapper**: Update `LmuSharedMemoryWrapper.h` to include the missing standard library header.
3. **Example**:
   ```cpp
   // If you see errors about std::vector being undefined:
   #include <vector>  // Add this to the wrapper
   ```

### Breaking API Changes?
If Studio 397 changes struct definitions or function signatures:

1. **Update Consuming Code**: Search the codebase for usages of the changed API (e.g., `grep -r "TelemInfoV01"`)
2. **Verify Sizes**: Check if struct sizes changed using `sizeof()` in debug builds
3. **Run Tests**: The unit test suite should catch most integration issues

### Need Help?
See the v0.6.38 changelog entry or the original PR from @DiSHTiX for reference on how the current wrapper was implemented.
