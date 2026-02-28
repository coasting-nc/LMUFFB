# Proposal: Reorganize `src/lmu_sm_interface/` into External vs. Project Subfolders

## Current Situation

The `src/lmu_sm_interface/` directory currently mixes two categories of files:

| File | Origin | Can be Modified? |
|------|--------|-----------------|
| `InternalsPlugin.hpp` | **External** — official LMU SDK header | ❌ No |
| `PluginObjects.hpp` | **External** — official LMU SDK header | ❌ No |
| `SharedMemoryInterface.hpp` | **External** — official LMU SDK header | ❌ No |
| `InternalsPluginWrapper.h` | **Project** — thin wrapper over the SDK | ✅ Yes |
| `LmuSharedMemoryWrapper.h` | **Project** — convenience wrapper | ✅ Yes |
| `PluginObjectsWrapper.h` | **Project** — convenience wrapper | ✅ Yes |
| `SafeSharedMemoryLock.h` | **Project** — RAII lock helper | ✅ Yes |
| `LinuxMock.h` | **Project** — platform mock for CI/Linux | ✅ Yes |
| `linux_mock/windows.h` | **Project** — minimal Windows header mock | ✅ Yes |
| `README.md` | **Project** — documentation | ✅ Yes |

### Problem

Mixing external and project files in the same directory creates friction:

1. **Static analysis (Clang-Tidy)** — external files trigger false positives that
   cannot be fixed without modifying files we don't own. The current workaround
   is a regex exclusion in `.clang-tidy`, which is fragile and couples the
   analysis config to specific filenames.
2. **Code clarity** — contributors cannot immediately tell which files are LMU
   SDK originals vs. project-owned wrappers.
3. **Update ergonomics** — when the LMU SDK is updated, it is hard to know
   exactly which files to replace without accidentally overwriting project code.

---

## Proposed Directory Structure

```
src/lmu_sm_interface/
├── external/                        ← SDK files — never edited directly
│   ├── InternalsPlugin.hpp
│   ├── PluginObjects.hpp
│   └── SharedMemoryInterface.hpp
├── InternalsPluginWrapper.h         ← Project wrappers (stay at current level)
├── LmuSharedMemoryWrapper.h
├── PluginObjectsWrapper.h
├── SafeSharedMemoryLock.h
├── LinuxMock.h
├── linux_mock/
│   └── windows.h
└── README.md
```

> [!NOTE]
> The wrapper files intentionally stay at the `lmu_sm_interface/` level (not
> moved to a sibling folder) so that their include paths remain unchanged across
> the codebase.

---

## Benefits

### 1. Simplified Static Analysis Configuration

Instead of enumerating individual filenames, `.clang-tidy` can use a single,
stable sub-path exclusion:

```yaml
# Before (fragile — tied to specific filenames)
HeaderFilterRegex: 'src/(?!lmu_sm_interface/(InternalsPlugin|PluginObjects|SharedMemoryInterface)\.hpp).*'

# After (stable — excludes the whole external/ subfolder)
HeaderFilterRegex: 'src/(?!lmu_sm_interface/external/).*'
```

### 2. Crystal-Clear Ownership

Any file under `external/` is understood to be **read-only SDK code**.
Any file outside of `external/` within `lmu_sm_interface/` is **project code**.

### 3. Easier SDK Updates

When a new LMU SDK version is released, the update procedure is unambiguous:
replace everything inside `external/` — nothing else needs to change.

---

## Migration Steps

> [!IMPORTANT]
> All include paths that reference the three external headers must be updated.

1. Create `src/lmu_sm_interface/external/`
2. Move the three external headers into it:
   - `InternalsPlugin.hpp`
   - `PluginObjects.hpp`
   - `SharedMemoryInterface.hpp`
3. Update all `#include` directives that reference these files. The only files
   that currently include them directly are the project-owned wrappers:
   - `InternalsPluginWrapper.h` → `#include "external/InternalsPlugin.hpp"`
   - `PluginObjectsWrapper.h` → `#include "external/PluginObjects.hpp"`
   - `LmuSharedMemoryWrapper.h` / `SafeSharedMemoryLock.h` → update accordingly
4. Update `.clang-tidy` to use the simplified regex above.
5. Build and run tests to confirm no regressions.
6. Update `README.md` in `lmu_sm_interface/` to document the folder layout.

---

## Impact Assessment

| Area | Impact |
|------|--------|
| **Build system** | None — CMake uses glob/explicit source lists from `src/`, not `lmu_sm_interface/` |
| **Include paths** | Low — only 4–5 files need updating |
| **Tests** | None — tests include project wrappers, not SDK headers directly |
| **CI workflows** | None |
| **`.clang-tidy`** | 1-line simplification |

---

*Proposed: 2026-02-28*
