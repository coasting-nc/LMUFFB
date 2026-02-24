# LMU Shared Memory Interface (`src/lmu_sm_interface/`)

This directory contains headers and utilities defining the shared memory interface with Le Mans Ultimate (LMU) and rFactor 2.

## ⚠️ VENDOR FILES - DO NOT MODIFY ⚠️

The following files are unmodified code provided directly by Studio 397 (developers of LMU/rF2). **DO NOT modify these files under any circumstances.**
- `SharedMemoryInterface.hpp`
- `InternalsPlugin.hpp`
- `PluginObjects.hpp`

If you need to fix compatibility issues (like missing standard library `#include`s, broken Linux compatibility, compile errors, missing types, missing Windows APIs), **use the provided Wrapper Headers** instead of editing the vendor files. This ensures that we can freely update these headers directly from Studio 397 in the future without losing any of our custom project fixes.

## Wrapper Headers (Safe to Use & Modify)

Whenever you interact with the shared memory interface or internals objects in the project codebase, you should always include the corresponding **Wrapper Header** instead of the vendor file. 

The wrappers automatically supply missing standards library headers, redefine missing Windows APIs (via `LinuxMock.h` for Headless builds), and patch other quirks *before* the vendor code is parsed by the compiler.

### Mappings
| What you want | What you MUST include instead | What it does |
|---|---|---|
| `SharedMemoryInterface.hpp` | `#include "LmuSharedMemoryWrapper.h"` | Adds missing C++ STL headers (`<optional>`, `<utility>`, `<cstdint>`, `<cstring>`), defines mock Windows APIs (`DWORD`, `HANDLE`, etc.) when compiling under Linux, then includes the real vendor file. |
| `InternalsPlugin.hpp` | `#include "InternalsPluginWrapper.h"` | Wraps `InternalsPlugin.hpp` and replaces `#include "PluginObjects.hpp"` with standard compliant headers. |
| `PluginObjects.hpp` | `#include "PluginObjectsWrapper.h"` | Adds missing `<windows.h>`/`LinuxMock.h` headers before dropping into the actual `PluginObjects.hpp`. |
| `SharedMemoryLock` | `#include "SafeSharedMemoryLock.h"` | Wraps `SharedMemoryLock` inside a `SafeSharedMemoryLock` object with additional functionality like explicit timeouts without physically changing the vendor class definition. |

## Linux Mock (`LinuxMock.h`)

Since LMU targets Windows, its `SharedMemoryInterface.hpp` leverages standard Windows libraries and synchronization primitives (`Windows.h`, `HANDLE`, locks, `WaitForSingleObject()`, memory mapped events, etc.). 

If you are building the project on Linux (`-DHEADLESS_GUI=ON`), our Wrappers will `#include "LinuxMock.h"`. This mock safely provides identically-typed dummy functions and structures for Windows APIs, tricking the MSVC-specific logic in `SharedMemoryInterface.hpp` into correctly compiling natively on GCC/Clang with standard Linux capabilities.

**Note:** The mock ensures compilation, but it does NOT actually establish real inter-process Shared Memory mapping out-of-the-box on Linux. This setup is specifically for building headless Linux tests or core abstractions.
