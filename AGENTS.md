# LMUFFB - AI Developer Guide

This file provides SOP, build commands, code style, and patterns for AI agents working on LMUFFB (C++ Force Feedback Driver for Le Mans Ultimate).

---

## Build Commands (Windows PowerShell)

**Full build (app + all tests):**
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release --clean-first
```

**Run all compiled tests:**
```powershell
.\build\tests\Release\run_combined_tests.exe
```
Uses unified test runner in `main_test_runner.cpp`

**Rebuild and run tests only:**
```powershell
cmake --build build --config Release --target run_combined_tests; .\build\tests\Release\run_combined_tests.exe
```

**Build single physics test:**
```powershell
cl /EHsc /std:c++17 /I.. tests\test_ffb_engine.cpp src\Config.cpp /Fe:tests\test_ffb_engine.exe; .\tests\test_ffb_engine.exe
```

**Run test with output capture:**
```powershell
tests\test_ffb_engine.exe 2>&1 | Tee-Object -FilePath tmp\test_results.txt
```

**CMake test command:**
```powershell
cmake -S . -B build; cmake --build build --config Release; ctest --test-dir build --output-on-failure
```

---

## Code Style Guidelines

### Formatting
- 2-space indentation (no tabs)
- Braces on new line for functions, same line for control flow
- Maximum line length: 120 characters
- Empty line between function definitions

### Naming Conventions
- **Classes**: PascalCase (`FFBEngine`, `GuiLayer`, `Config`)
- **Structs**: PascalCase (`FFBSnapshot`, `ChannelStats`, `BiquadNotch`)
- **Member variables**: `m_` prefix (`m_gain`, `m_enabled`, `m_phase`)
- **Static constants**: UPPER_SNAKE_CASE (`PI`, `TWO_PI`, `MAX_TORQUE`)
- **Functions**: camelCase (`calculateForce`, `updateFilter`, `getSnapshot`)
- **Local variables**: camelCase (`maxTorque`, `outputForce`, `sampleRate`)

### Types
- Use `float` for FFB output values and user-facing settings
- Use `double` for internal math/physics calculations
- Use `bool` for flags and toggles
- Use `int` for indices and enumerated values
- Use `std::vector` for dynamic arrays
- Prefer fixed-width types when binary compatibility needed (`int32_t`, `uint64_t`)
- Use `constexpr` for compile-time constants like mathematical values

### Imports
- Standard library: `<cmath>`, `<algorithm>`, `<vector>`, `<mutex>`, `<string>`
- System headers: `<windows.h>` only in platform-specific files
- Local headers: Relative paths (`"Config.h"`, `"FFBEngine.h"`)
- Order: Standard lib → External → Project headers

### Error Handling
- Use assertions for invariant checks: `ASSERT_TRUE(condition)`
- Clamp unsafe inputs: `std::min(1.5, loadFactor)` to prevent hardware damage
- Return early on invalid state rather than nested conditionals
- Log errors to console: `std::cerr << "Error: " << msg << std::endl;`
- Single-line comments use `//` style (not block comments)

### Critical Math Rules
- **Vibrations**: Use phase accumulation, NOT `sin(time * freq)`
  - ❌ Wrong: `sin(mElapsedTime * mFreq)`
  - ✅ Right: `mPhase += mFreq * dt; output = sin(mPhase);`
- Always clamp tire load factors before using in force calculations
- Use `std::max`/`std::min` instead of raw ternary for readability

---

## Architecture Patterns

### FFB Engine (400Hz Real-Time)
- Header-only in `src/FFBEngine.h`
- No heap allocation in `calculateForce()` - runs on high-priority thread
- Uses producer-consumer with GUI via `FFBSnapshot` ring buffer
- All physics inputs must be clamped for safety

### GUI Layer (60Hz)
- `src/GuiLayer.cpp` uses ImGui for rendering
- Never read FFBEngine state directly for plots - use snapshot batch
- Thread-safe: guards config access with `std::lock_guard<std::mutex>`

### Configuration
- `src/Config.h` declares defaults (single source of truth)
- `src/Config.cpp` handles persistence (JSON/INI)
- New settings default to `false` or `0.0` (safe defaults)

---

## Testing Requirements

- **ALL logic changes require unit tests** in `tests/test_ffb_engine.cpp`
- Use existing macros: `ASSERT_TRUE()`, `ASSERT_NEAR()`, `ASSERT_GE()`, `ASSERT_LE()`
- Wait `FILTER_SETTLING_FRAMES` (40) before assertions on smoothed values
- Tests must pass before committing changes

---

## Documentation

Update these files when making changes:
| Change Type | Files to Update |
|-------------|-----------------|
| Math/Physics | `docs/dev_docs/FFB_formulas.md` |
| New FFB Effect | `docs/ffb_effects.md`, `docs/the_physics_of__feel_-_driver_guide.md` |
| Telemetry | `docs/dev_docs/telemetry_data_reference.md` |
| GUI Changes | `README.md` |
| Version Bump | `VERSION`, `src/Version.h`, `CHANGELOG.md` |

---

## Common Pitfalls

- Do NOT use `mElapsedTime` for sine wave calculations
- Do NOT allocate memory in the real-time FFB thread
- Do NOT remove vJoyInterface.dll dynamic loading (app must work without vJoy)
- Do NOT change struct packing in shared memory headers

---

## Available Subagents

For specialized tasks, invoke these subagents from `subagents/` directory:

| Subagent | Use When |
|----------|----------|
| `02-language-specialists/cpp-pro.md` | C++ optimization, memory management, template metaprogramming, concurrency patterns |
| `07-specialized-domains/embedded-systems.md` | Real-time constraints, hardware abstraction, interrupt handling, resource-limited code |
| `07-specialized-domains/game-developer.md` | Game telemetry integration, physics simulation, player feedback systems |
| `06-developer-experience/refactoring-specialist.md` | Code smell detection, design pattern application, systematic code improvement |
| `06-developer-experience/build-engineer.md` | Build optimization, compilation strategies, caching mechanisms |
