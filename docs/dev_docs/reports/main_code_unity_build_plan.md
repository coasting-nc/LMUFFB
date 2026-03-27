# Report: Preparing Main Code for Unity Builds

This report outlines the strategy for preparing the main `LMUFFB` codebase to support Unity (Jumbo) builds, enforcing strict namespace rules, and mitigating the common drawbacks associated with unified translation units.

See also:
* `docs\dev_docs\unity_builds_implementation.md`
* `docs\dev_docs\unity_builds_tests_incremental_plan.md`

## 1. Preparing the Main Code (Rigorous Namespace Usage Plan)

To safely compile the main source code as a Unity build, we must ensure that no internal symbols or implementations collide when multiple `.cpp` files are concatenated. The primary strategy to achieve this is **Rigorous Namespace Usage**. 

### Refactoring Plan
1. **Global Namespace Elimination:** No code should exist in the global namespace except for the final `main()` function entry point.
2. **Project Namespace Wrapping:** All core project files (`.cpp` and `.h`) must encapsulate their implementations within a dedicated root namespace (e.g., `namespace LMUFFB { ... }`).
3. **File/Module Specific Sub-namespaces:** Further segregate code into logical modules, such as `namespace LMUFFB::Physics` or `namespace LMUFFB::GUI`.
4. **Anonymous Namespaces for Internal Linkage:** Any file-local helper functions, constants, or static variables must be placed inside an anonymous namespace specifically within the module's namespace.
   ```cpp
   namespace LMUFFB::Physics {
       namespace {
           const int LOCAL_CONSTANT = 42;
           void PrivateHelper() { /* implementation */ }
       }
   }
   ```
5. **Incremental Migration Strategy:** Similar to the test suite plan, we can implement an incremental "opt-in" list for main codebase files. Files that have been refactored to comply with the namespace rules will be added to a `UNITY_READY_MAIN` CMake list to build as a chunk. Files not yet refactored will skip unity inclusion and compile standalone until they are addressed.

## 2. Preventing Future Regressions 

Once the main code is refactored, we need mechanisms to prevent newly introduced code from breaking the "no global namespace" rule. 

### Static Analysis and Automation
* **`clang-tidy` Custom Checks:** While standard `clang-tidy` doesn't strictly have a "no global namespace" rule configurable out-of-the-box, we can write a custom `clang-query` AST matcher script to run in CI. The matcher would look for `decl()` nodes whose context is the `TranslationUnitDecl` (meaning they reside in the global scope) and flag them as errors (filtering out `main`).
* **Automated Linting Scripts:** Alternatively, a lightweight pre-commit hook or CI Python script using an AST parser (like `libclang`) or regex can efficiently scan new changes to ensure that all class definitions, functions, and variables are contained within `namespace LMUFFB`.
* **Continuous Unity CI Verification:** By running a GitHub Actions job with Unity builds explicitly enabled (`LMUFFB_USE_UNITY_BUILD=ON`), any new PR that introduces a symbol collision via the global namespace will immediately fail the build.

## 3. Drawbacks of Unity Builds: Obscured Errors

**Concern:** *If there is a build error, it seems difficult to understand from which file and line it was raised.*

**Is this concern correct?**
Yes and No. Modern CMake implementations of Unity builds generate a literal source chunk (e.g., `unity_0_cxx.cxx`) consisting of standard `#include "path/to/original.cpp"` directives. 
- If a standard syntax error occurs *inside* `original.cpp`, the compiler correctly tracks the inclusion and points to `original.cpp:line_number`.
- **However**, if the error is caused by a **symbol collision**, a **macro redefinition**, or a **hidden dependency** (e.g., `B.cpp` only compiles because `A.cpp` was included before it in the chunk and provided `<vector>`), the compiler will output confusing cross-file errors or point to the artificial chunk boundary. This makes pinpointing the root cause significantly harder than in standalone compilation.

### Mitigations
1. **Disable Unity Builds Locally:** Developer environments should leave `LMUFFB_USE_UNITY_BUILD=OFF` by default. Developers should rely on standard incremental builds, ensuring that when an error happens locally, it's immediately traceable to exactly one file.
2. **Dual-Track CI:** Maintain CI pipelines that run both standard (non-unity) and Unity builds. If the Unity build fails but the standard build passes, it immediately narrows the issue down to an include hygiene issue or a symbol collision.
3. **Reproduce and Isolate:** When a CI Unity pipeline fails with an obscured error, a developer can temporarily toggle `LMUFFB_USE_UNITY_BUILD=ON` locally to reproduce the exact unified error footprint, applying `#warning` or disabling `#include`s to narrow down the culprit, then toggle it `OFF` to ensure standard compilation hygiene once resolved.

## 4. Incremental Refactoring Strategy: File-by-File Prioritization

The goal is to incrementally process files through the 5-step refactoring plan to eliminate the global namespace. 

### Should we complete all 5 steps per file before moving on?
**No.** Performing all 5 steps simultaneously on a file-by-file basis is usually impractical if the file is a header (`.h`). If you wrap a core utility header (like `VehicleUtils.h`) in `namespace LMUFFB`, then *every single file that includes this header* will instantly break compilation unless they are also updated simultaneously to call `LMUFFB::...`.

**The recommended workflow is:**
1. **Batch the headers:** Wrap a logical, self-contained group of headers in `namespace LMUFFB`.
2. **Implement `using namespace LMUFFB;` temporarily:** In the `.cpp` files that include those newly refactored headers but haven't been fully refactored yet themselves, add `using namespace LMUFFB;` as a bridge to quickly fix the compilation.
3. **Refactor the implementations (all 5 steps):** Once the headers are secure, process the corresponding `.cpp` files one by one. Take each `.cpp` file, apply all 5 steps (wrap in sub-namespace, apply anonymous namespaces, add to Unity "opt-in" whitelist), and remove the temporary `using namespace` statement.

### Sorted Prioritization List (Simplest to Hardest)

> **Note:** As of v0.7.251, all five core phases (1–5) are complete. The list below is preserved as a historical reference for the order in which files were processed.

The simplest files to refactor are pure utilities or isolated leaf nodes (files that don't depend on other internal modules). The core application logic and UI should be done last.

Here is the sorted, prioritized list of files that were refactored (from easiest to hardest), based on the `src/` hierarchy at the time:

#### 1. Leaf Utility Modules (Start Here)
These are mathematically pure or standalone and have minimal dependencies:
* `utils/MathUtils.h`
* `logging/PerfStats.h` / `.cpp`
* `logging/RateMonitor.h`
* `physics/VehicleUtils.h` / `.cpp`
* `physics/SteeringUtils.cpp`

#### 2. Self-Contained I/O and Core Data Structures
Structs, enums, and I/O wrappers that are isolated and mostly stateless:
* `io/rF2/rF2Data.h`
* `io/lmu_sm_interface/` (Shared Memory Wrappers like `SafeSharedMemoryLock.h`)
* `core/Config.h` / `.cpp`
* `ffb/FFBConfig.h`
* `ffb/FFBSnapshot.h`

#### 3. Core Engine Components
These depend on utilities and configs but form the cohesive inner logic:
* `physics/GripLoadEstimation.cpp`
* `ffb/UpSampler.h` / `.cpp`
* `ffb/FFBSafetyMonitor.h` / `.cpp`
* `ffb/FFBDebugBuffer.h` / `.cpp`
* `ffb/FFBEngine.h` / `.cpp`

#### 4. Boundary and External API Layers
These contain OS-specific macros (`windows.h`) or large external API dependencies that require careful namespace hygiene:
* `ffb/DirectInputFFB.h` / `.cpp`
* `gui/DXGIUtils.h` / `.cpp`
* `io/RestApiProvider.h` / `.cpp`
* `logging/AsyncLogger.h`

#### 5. Complex UI and Main Entry (Do Last)
These files tie everything together and represent the most deeply coupled module:
* `gui/Tooltips.h`, `gui/GuiWidgets.h`
* `gui/GuiLayer_*.cpp` (Common, Win32, Linux)
* `io/GameConnector.h` / `.cpp`
* `core/main.cpp` (The only file that *must* keep `main()` exposed in the global namespace).

### Particular Challenges to Watch Out For

1. **`#include` Directives Inside Namespaces:** Always ensure `#include` directives are completely **outside** and above your `namespace LMUFFB { ... }` block. Including external headers (especially Windows C-headers or ImGui) inside a namespace will disastrously wrap those external libraries in your namespace and cause thousands of compilation errors.
2. **`extern "C"` Blocks:** If you have `extern "C"` declarations for C-APIs or plugin structures, be extremely careful not to wrap them inside a C++ namespace. They expect strict C-linkage and normally must remain global.
3. **Forward Declarations:** If a header forward-declares a type from another file (e.g., `class FFBEngine;`), make sure the forward declaration is now correctly placed inside the matching `LMUFFB` namespace block, not floating globally.
4. **Macros:** Macros (`#define`) do not respect namespaces. During this refactoring, convert local `.cpp` macros into `constexpr` variables inside your anonymous namespace to prevent macro leakage during the final Unity chunk concatenation.

## 5. Handling Vendor Files and Proprietary Game APIs

When refactoring the codebase, you will naturally encounter third-party vendor code (e.g., `vendor/toml++/toml.hpp`, `vendor/lz4`, `vendor/imgui`) and proprietary game interface headers (`src/io/lmu_sm_interface/SharedMemoryInterface.hpp`, `PluginObjects.hpp`, `InternalsPlugin.hpp`).

### Do we enforce the same refactoring rules on these files?

**Absolutely NOT.** You must strictly exempt these files from the "Rigorous Namespace Usage" policy. Attempting to modify them carries severe risks:

1. **Proprietary Game Files:** The 3 files located in `src/io/lmu_sm_interface/` define the exact memory layout (ABI), external linkages, and API contract expected by the actual game engine. Wrapping them in a C++ namespace or changing their internal linkage will break the plugin structure. The game will fail to load or communicate with your plugin.
2. **Vendor Libraries:** Modifying complex libraries like `imgui` or `toml++` makes it extremely painful to update them to newer versions. Fortunately, modern libraries (like `toml++` and `imgui`) already responsibly encapsulate their code within their own namespaces (`toml::`, `ImGui::`). C-libraries (like `lz4`) are inherently designed to operate in the global scope.

### How to manage them in a Unity Build

Since we cannot (and should not) refactor these external files to conform to our internal namespace rules, we manage them via build configuration and rigid `#include` discipline:

1. **Standalone Compilation for Vendor `.cpp` Files:**
   If a vendor library requires compiling source files (e.g., `imgui.cpp`, `lz4.c`), we simply exclude them from our Unity build chunk. We flag them in CMake so they compile normally as standalone translation units:
   ```cmake
   set_source_files_properties(vendor/imgui/imgui.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
   ```
   This ensures that any chaotic internal macros or static variables inside vendor implementations cannot clash with our `LMUFFB` internals during the unity chunking phase.

2. **Strict `#include` Isolation for Headers:**
   When including `SharedMemoryInterface.hpp`, `toml.hpp`, or `imgui.h` in our newly refactored files, **always** place the `#include` directive *outside* and *above* the `namespace LMUFFB { ... }` block. 
   
   *Correct:*
   ```cpp
   #include <toml++/toml.hpp>
   #include "io/lmu_sm_interface/SharedMemoryInterface.hpp"
   
   namespace LMUFFB {
       // Our code safely uses them without mutating their intended scope.
   }
   ```
   
   *Incorrect (Catastrophic):*
   ```cpp
   namespace LMUFFB {
       #include "io/lmu_sm_interface/SharedMemoryInterface.hpp" // Disastrous!
   }
   ```

By treating vendor code and proprietary APIs as immutable global resources, we preserve direct update paths and maintain game ABI continuity, while applying our rigorous standards exclusively to the code we own.

## 6. Progress Checklist

This section tracks the progress made towards fully refactoring the main code and enabling it to compile under Unity builds.

### 6.1 Unity Build Infrastructure Support
- [x] Add `UNITY_READY_MAIN` whitelist logic to `src/CMakeLists.txt` for incremental `.cpp` chunking.
- [x] Apply `SKIP_UNITY_BUILD_INCLUSION` automatically to unrefactored `.cpp` files in `src/CMakeLists.txt`.
- [x] Ensure vendor modules (`imgui.cpp`, etc.) are explicitly excluded from Unity inclusion.

### 6.2 Phase 1: Leaf Utility Modules (Headers & Source)
- [x] Refactored `logging/PerfStats.h` (Wrapped `ChannelStats` in `namespace LMUFFB`).
- [x] Refactor `utils/MathUtils.h` (Consolidated `ffb_math` into `namespace LMUFFB`).
- [x] Refactor `utils/TimeUtils.h` & `logging/RateMonitor.h` (Wrapped in `namespace LMUFFB`).
- [x] Refactor `physics/VehicleUtils.h` & `.cpp` (First viable `.cpp` file for `UNITY_READY_MAIN`).
- [x] Refactor `physics/SteeringUtils.cpp` (Decoupled from `FFBEngine` into a true utility module inside `namespace LMUFFB`).

### 6.3 Phase 2: Core Data Structures
- [x] Refactor `core/Config.h` & `.cpp`.
- [x] Refactor `ffb/FFBConfig.h` (Already wrapped).
- [x] Refactor `ffb/FFBSnapshot.h` (Wrapped via `ffb/FFBDebugBuffer.h`).
- [x] Refactor `ffb/FFBMetadataManager.h` & `.cpp`.
- [ ] ~~Wrap isolated I/O wrappers (`io/rF2/rF2Data.h`).~~ **Do NOT do this.** This is a proprietary vendor/game file with a strict ABI contract. It must not be wrapped in any namespace. Update `CMakeLists.txt` to permanently exclude it from any Unity inclusion consideration.

### 6.4 Phase 3: Core Logic (FFB & Physics)
- [x] Refactor `ffb/UpSampler.h` & `.cpp`.
- [x] Refactor `ffb/FFBSafetyMonitor.h` & `.cpp`.
- [x] Refactor `ffb/FFBDebugBuffer.h` & `.cpp`.
- [x] Refactor `physics/GripLoadEstimation.cpp`.
- [x] Refactor `ffb/FFBEngine.h` & `.cpp` (Central consumer wrapped).

### 6.5 Phase 4: OS Boundaries & Subsystems
- [x] Refactor `ffb/DirectInputFFB.h` & `.cpp`. (Fully namespaced and whitelisted).
- [x] Refactor `gui/DXGIUtils.h` & `.cpp`. (Fully namespaced and whitelisted).
- [x] Refactor `io/RestApiProvider.h` & `.cpp`. (Fully namespaced and whitelisted).
- [x] Refactor `logging/AsyncLogger.h`. (Header-only module officially wrapped).
- [x] Refactor `io/GameConnector.h` & `.cpp`. (Fully namespaced and whitelisted).

### 6.6 Phase 5: UI & Final Integration
- [x] Refactor `gui/Tooltips.h` & `gui/GuiWidgets.h`.
- [x] Refactor `gui/GuiLayer_*.cpp`. (Fully namespaced and whitelisted).
- [x] Finalize `core/main.cpp` (Retaining global `main()` declaration).

### 6.7 Phase 6: Subsystem Namespace Migration (Post-Unity Stability)
- [x] ~~**IMPORTANT**: Only begin this phase AFTER Phase 1-5 are 100% complete and the Unity Build is stable.~~ ✅ Gate condition met as of v0.7.251.
- [x] Transition `logging/` files from `namespace LMUFFB` to `namespace LMUFFB::Logging`. (Initial batch: all six files in `src/logging/`).
- [x] Transition `utils/` files to `namespace LMUFFB::Utils`. (v0.7.256)
- [x] Transition `physics/` files to `namespace LMUFFB::Physics`. (v0.7.257)
- [x] Transition `gui/` files to `namespace LMUFFB::GUI`. (v0.7.258)
- [x] Conduct Internal Linkage Audit and harden `.cpp` files with anonymous namespaces (Batch 1: core/gui). (v0.7.259)
- [x] Remove temporary bridge aliases in root `namespace LMUFFB` for the `Logging` subsystem. (v0.7.259)
- [ ] Conduct Internal Linkage Audit and harden `.cpp` files (Batch 2: ffb/io).
- [ ] Transition `ffb/` files to `namespace LMUFFB::FFB`.
- [ ] Transition `io/` files to `namespace LMUFFB::IO`.

---

## 7. Implementation Notes

### 7.1 Implementation Notes (v0.7.259)
- **Encountered Issues:**
  - Encountered linker errors when moving GUI platform helpers (e.g., `GetGuiPlatform`, `ResizeWindowPlatform`) into anonymous namespaces. These functions require external linkage because they are accessed by `GuiLayer_Common.cpp` (in Unity builds) and the unit test suite across different translation units. Resolved by moving them out of anonymous namespaces while keeping them protected within `namespace LMUFFB::GUI`.
  - Discovered that several unit tests relied on `LMUFFB::FFBThread` and `LMUFFB::PopulateSessionInfo` being visible globally. Moving these to anonymous namespaces in `main.cpp` broke the test runner. Fixed by restoring external linkage for these specific test-required symbols.
  - Encountered several "Invalid merge diff" errors during refactoring of `Config.cpp` and `FFBEngine.cpp` due to minor snippet mismatches. Resolved by re-reading the files and ensuring search blocks exactly matched the disk state.
- **Deviations from the Plan:**
  - Decided to focus the removal of bridge aliases specifically on the `Logging` subsystem for this iteration to maintain a strictly incremental approach. Other subsystems (`Utils`, `Physics`, `GUI`) will be handled in subsequent steps.
- **Suggestions for the Future:**
  - Continue the Namespace Hygiene rollout by incrementally removing bridge aliases for the `Utils` and `Physics` subsystems.
  - Conduct a targeted Internal Linkage sweep for the `src/ffb/` and `src/io/` directories, ensuring file-local helpers are correctly encapsulated in anonymous namespaces.

### 7.2 Implementation Notes (v0.7.258)
- **Encountered Issues:**
  - Encountered linker errors for `g_engine_mutex` and `g_running` within the `LMUFFB::GUI` namespace in `GuiLayer_Common.cpp`, `GuiLayer_Win32.cpp`, and `GuiLayer_Linux.cpp`. Resolved by ensuring these `extern` declarations are positioned within the root `namespace LMUFFB` while the implementation remains in `namespace LMUFFB::GUI`.
- **Deviations from the Plan:** None.
- **Suggestions for the Future:** Phase 6 (Subsystem Namespace Migration) is now complete for all major functional directories. Future efforts should focus on refining internal linkage within these namespaces and potentially migrating remaining root-level files if architectural needs arise.

### 7.3 Implementation Notes (v0.7.257)
- **Encountered Issues:**
  - Discovered that `FFBEngine.h` had several hardcoded types and constants (`LoadTransform`, `GripResult`, `FFBCalculationContext`, `DEFAULT_CALC_DT`) that belonged to the physics domain. Moving them to a new header `src/physics/GripLoadEstimation.h` caused circular dependency issues and ambiguous symbol errors because `DEFAULT_CALC_DT` was also defined in `FFBEngine.h`. Resolved by centralizing these in the new header under `LMUFFB::Physics`, renaming the constant to `PHYSICS_CALC_DT`, and adding bridge aliases.
  - Encountered linker errors for `g_engine_mutex` when used within the `LMUFFB::Physics` namespace in `GripLoadEstimation.cpp`. Resolved by ensuring `extern std::recursive_mutex g_engine_mutex;` is declared in the root `LMUFFB` namespace and correctly referenced.
- **Deviations from the Plan:**
  - Extracted shared physics types and constants from `FFBEngine.h` into a new header `src/physics/GripLoadEstimation.h` to complete the encapsulation of the physics subsystem.
  - Decoupled several physics-only helper functions (`CalculateRawSlipAnglePair`, `CalculateSlipAngle`, etc.) from the `FFBEngine` class and converted them into standalone functions within `namespace LMUFFB::Physics` to improve modularity and satisfy namespace rules.
  - **Named Constants Preservation:** While some named constants (`MIN_SLIP_ANGLE_VELOCITY`, `SLOPE_HOLD_TIME`) were initially replaced by literals during the refactor, they have been fully restored as `static constexpr` members within `namespace LMUFFB::Physics` in `GripLoadEstimation.h` to maintain codebase consistency and avoid magic numbers.
- **Suggestions for the Future:** Continue Phase 6 by transitioning `src/gui/` files (e.g., `GuiLayer.h`, `GuiWidgets.h`, `Tooltips.h`) to `namespace LMUFFB::GUI`.

### 7.4 Implementation Notes (v0.7.256)
- **Encountered Issues:**
  - Encountered conflicts between `extern` declarations of mock time globals (`g_mock_time`, `g_use_mock_time`) in `tests/test_main_harness.cpp` and `main.cpp` vs their new definition in `LMUFFB::Utils`. Resolved by using `using Utils::g_mock_time;` and `using Utils::g_use_mock_time;` in the consumer files and defining them inside `namespace Utils` in `tests/main_test_runner.cpp`.
  - Discovered that the initial bridge implementation `namespace LMUFFB { using LMUFFB::Utils::PI; ... }` inside `MathUtils.h` was causing "invalid use of qualified-name" errors and member access issues in `Config.h` and other headers because of how the compiler handles nested namespaces and using declarations within the same translation unit. Fixed by using non-qualified `using Utils::PI;` etc. within `namespace LMUFFB` in the headers.
- **Deviations from the Plan:** None.
- **Suggestions for the Future:** Continue Phase 6 by transitioning `src/physics/` files (e.g., `VehicleUtils.h`, `SteeringUtils.h`, `GripLoadEstimation.cpp`) to `namespace LMUFFB::Physics`.

### 7.5 Phase 1-3 Implementation Notes (v0.7.233 - v0.7.253)
- **Encountered Issues:**
  - **Compilation Order Masking:** During early whitelist configuration, failing builds were sometimes masked by successful previous binaries in PowerShell.
  - **Hidden Dependencies:** Refactoring leaf modules broke dependent files requiring explicit `using namespace LMUFFB;`.
  - **Namespace Resolution (C2888):** Bypassed by decoupling `FFBEngine` methods into standalone utility functions (e.g., `CalculateSoftLock`).
  - **Header Pollution:** Initial Phase 6 attempts included `using namespace` in headers; corrected to use qualified names or file-scope directives.
  - **Linker Errors (ImGui/Win32):** `ImGui_ImplWin32_WndProcHandler` required global-scope `extern` declaration to maintain correct symbol mangling.
- **Deviations from the Plan:**
  - Expanded scope of Phase 4 to update all call sites project-wide to use `LMUFFB::GameConnector`.
  - Namespaced all six logging files in v0.7.253 instead of just the initial two for consistency.

---

## 8. Questions & Answers

### Q: Why wasn't the Makefile (CMakeLists) updated during the first refactoring? I thought we could enable a Unity build for `src/logging/PerfStats.h` immediately.
**A:** CMake's Unity builds exclusively process **Translation Units** (source files like `.cpp` or `.c`). Header files (`.h` and `.hpp`) are never compiled directly on their own; they are textually inserted by the preprocessor when a `.cpp` file includes them. Because `PerfStats.h` is a header-only structure with no corresponding `.cpp` source, there is no discrete file to attach a `UNITY_BUILD_INCLUSION` property to. The header automatically begins reaping the benefits of the Unity batch build the moment a `.cpp` file that uses it (e.g., `VehicleUtils.cpp` or `FFBEngine.cpp`) is moved into the `UNITY_READY_MAIN` source whitelist.

### Q: Should we have specialized namespaces for each file, "module", or group of files? Why was `PerfStats.h` put directly in the `LMUFFB` namespace rather than a more specific one?
**A:** **Yes**, the architectural end goal (Step 3 of the Refactoring Plan) strictly encourages enforcing subsystem namespaces like `LMUFFB::Logging::ChannelStats` or `LMUFFB::Physics::VehicleUtils`.

For the demonstrative "first refactoring", it was temporarily attached to the global `LMUFFB` root namespace instead of a specialized `LMUFFB::Logging` namespace for the following practical reasons:
1. **Incremental Pragmatism:** The immediate goal was proving "Global Namespace Elimination" while guaranteeing zero build failures. Creating a deep submodule hierarchy out the gate generates a cascade of complex naming updates across non-refactored monolithic classes like `FFBEngine`.
2. **Deferred Sub-namespace Migration:** The plan always called for a dedicated Phase 6 to handle sub-namespace migration once the monoliths were safely inside `namespace LMUFFB`. Phase 6 has now begun — `src/logging/` files have been transitioned to `LMUFFB::Logging` (v0.7.253). `src/utils/` and `src/physics/` are next.


### Q: Why are we still using the `LMUFFB` namespace? Shouldn't we start using the more specific ones? When should we start using more specific namespaces?
**A:** We are temporarily using the root `LMUFFB` namespace for all files to prioritize **"Global Namespace Elimination"** with minimal architectural friction. If we started using granular namespaces (like `LMUFFB::Physics` or `LMUFFB::Logging`) right now, the monolithic, unrefactored classes (like `FFBEngine`) would require hundreds of complex prefix updates (`LMUFFB::Physics::VehicleUtils::...`) which breaks compilation.

**When to transition:** The sub-namespace migration was always gated on completing Phases 1–5 first. That gate has been passed (v0.7.251). Phase 6 is now active — `src/logging/` has been transitioned to `LMUFFB::Logging` (v0.7.253), `src/utils/` to `LMUFFB::Utils` (v0.7.256), and `src/physics/` to `LMUFFB::Physics` (v0.7.257). Sub-namespace migration for `src/gui/` (`LMUFFB::GUI`) is the next objective.

## 9. Next Steps: Post-Migration Cleanup and Hardening
Phase 6 and internal hardening are now well underway. All major subsystems are namespaced, and internal linkage hardening has progressed significantly.

### Your Objectives for the Next PR:
1. **Namespace Hygiene (Utils Subsystem):**
   - Systematically remove temporary bridge aliases in `src/utils/MathUtils.h`, `src/utils/StringUtils.h`, and `src/utils/TimeUtils.h`.
   - Update all call sites project-wide (including tests) to use fully qualified names (`Utils::...`) or file-scope `using namespace LMUFFB::Utils;`.
2. **Continued Subsystem Migration (FFB & I/O):**
   - Transition `src/ffb/` and `src/io/` modules to `namespace LMUFFB::FFB` and `namespace LMUFFB::IO` respectively.
   - Maintain the "Include Rule" and "Using Placement Rule" during migration.
3. **Extended Internal Linkage Audit (FFB & I/O Subsystems):**
   - Conduct a systematic review of `.cpp` files in `src/ffb/` and `src/io/` to move internal helper functions and constants into anonymous namespaces.

### Critical Reminders for Phase 6
*   **The Include Rule:** All `#include` directives **MUST** remain outside namespace blocks. This is non-negotiable for Unity Build compatibility.
*   **The `using namespace` Placement Rule:** In `.cpp` files, `using namespace LMUFFB::SomeSubNs;` directives **MUST** be placed at **file scope** — after all `#include` directives and before the `namespace LMUFFB { ... }` block. Do **not** place them inside a namespace block. See §8.12 for the real-world example of this issue occurring in `FFBSafetyMonitor.cpp` and three GUI files during v0.7.253.
*   **Internal Linkage:** Use anonymous namespaces for any helper functions or constants within `.cpp` files to avoid ODR violations when bundled.
*   **Bridge Aliases:** When migrating a header to a sub-namespace, add temporary `using` alias bridges inside `namespace LMUFFB { using Foo = LMUFFB::SubNs::Foo; }` at the bottom of the header to keep existing call sites compiling. Remove them only once all call sites have been updated.

### See also:

See the code review of the v0.7.253 iteration of Phase 6, which includes a recommendations section for minor open items carried forward:

* `docs\dev_docs\code_reviews\code_review_unity_build_phase6_v0.7.253.md`