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
The simplest files to refactor are pure utilities or isolated leaf nodes (files that don't depend on other internal modules). The core application logic and UI should be done last.

Here is a sorted, prioritized list of files to refactor (from easiest to hardest) based on the current `src/` hierarchy:

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
- [x] Refactor `ffb/FFBSnapshot.h` (Wrapped via `FFBDebugBuffer.h`).
- [x] Refactor `ffb/FFBMetadataManager.h` & `.cpp`.
- [ ] TODO: don't do this. This is another vendor / game file, not to be changed. Update the makefile accordingly. -- Wrap isolated I/O wrappers (`io/rF2/rF2Data.h`).

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
- [ ] **IMPORTANT**: Only begin this phase AFTER Phase 1-5 are 100% complete and the Unity Build is stable.
- [x] Transition `logging/` files from `namespace LMUFFB` to `namespace LMUFFB::Logging`. (Initial batch: all six files in `src/logging/`).
- [ ] Transition `physics/` files to `namespace LMUFFB::Physics`.
- [ ] Transition `gui/` files to `namespace LMUFFB::GUI`.

---

## 7. Questions & Answers

### Q: Why wasn't the Makefile (CMakeLists) updated during the first refactoring? I thought we could enable a Unity build for `src/logging/PerfStats.h` immediately.
**A:** CMake's Unity builds exclusively process **Translation Units** (source files like `.cpp` or `.c`). Header files (`.h` and `.hpp`) are never compiled directly on their own; they are textually inserted by the preprocessor when a `.cpp` file includes them. Because `PerfStats.h` is a header-only structure with no corresponding `.cpp` source, there is no discrete file to attach a `UNITY_BUILD_INCLUSION` property to. The header automatically begins reaping the benefits of the Unity batch build the moment a `.cpp` file that uses it (e.g., `VehicleUtils.cpp` or `FFBEngine.cpp`) is moved into the `UNITY_READY_MAIN` source whitelist.

### Q: Should we have specialized namespaces for each file, "module", or group of files? Why was `PerfStats.h` put directly in the `LMUFFB` namespace rather than a more specific one?
**A:** **Yes**, the architectural end goal (Step 3 of the Refactoring Plan) strictly encourages enforcing subsystem namespaces like `LMUFFB::Logging::ChannelStats` or `LMUFFB::Physics::VehicleUtils`.

For the demonstrative "first refactoring", it was temporarily attached to the global `LMUFFB` root namespace instead of a specialized `LMUFFB::Logging` namespace for the following practical reasons:
1. **Incremental Pragmatism:** The immediate goal was proving "Global Namespace Elimination" while guaranteeing zero build failures. Creating a deep submodule hierarchy out the gate generates a cascade of complex naming updates across non-refactored monolithic classes like `FFBEngine`. 
2. **Current Coupling:** In the current unrefactored state, `FFBEngine` utilizes `ChannelStats` heavily inside its own global definition. When `FFBEngine` itself eventually undergoes the 5-step process (Phase 3), the utility modules like `PerfStats` will be neatly transitioned down into their final `LMUFFB::Logging` domain, leaving a vastly cleaner set of `using namespace` scopes inside the final class implementations.


### Q: Why are we still using the `LMUFFB` namespace? Shouldn't we start using the more specific ones? When should we start using more specific namespaces?
**A:** We are temporarily using the root `LMUFFB` namespace for all files to prioritize **"Global Namespace Elimination"** with minimal architectural friction. If we started using granular namespaces (like `LMUFFB::Physics` or `LMUFFB::Logging`) right now, the monolithic, unrefactored classes (like `FFBEngine`) would require hundreds of complex prefix updates (`LMUFFB::Physics::VehicleUtils::...`) which breaks compilation.

**When to transition:** We will rigidly switch to specific sub-namespaces **only after** all 5 phases of the core Refactoring Plan are complete and the entire application is successfully building via the Unity chunk without global pollution. Once the monoliths are safely inside the root `LMUFFB` namespace, transitioning utilities down into `LMUFFB::Logging` becomes a safe, purely internal refactoring task (Phase 6).

---

## 8. Implementation Notes

### 8.1 Encountered Issues
- **Compilation Order Masking:** During the early configuration of the Unity chunking whitelist, running `cmake --build build` immediately followed by `; python scripts/run_all_tests.py` caused PowerShell to mask genuine C++ compilation errors. If the build step failed, the test script still ran using the *previously compiled* binaries, returning exit code 0 and falsely implying success. Future build-validation commands must evaluate the exit status of the compiler before proceeding to testing.
- **Hidden Dependencies:** Refactoring a seemingly isolated `.h`/`.cpp` leaf module (`VehicleUtils`) logically broke dependent files outside the Unity chunk (like `GripLoadEstimation.cpp`, `FFBMetadataManager.cpp`, and `main.cpp`) because they included the updated header without namespace qualification. Adding `using namespace LMUFFB;` directly beneath the headers in the consumer `.cpp` files resolved these `error C3861: identifier not found` lookup failures.
- **Namespace Resolution (C2888) and True Decoupling:** When refactoring `physics/SteeringUtils.cpp`, initially encapsulating it entirely within `namespace LMUFFB { ... }` resulted in an MSVC `C2888` error because `void FFBEngine::calculate_soft_lock` belongs to a class currently residing in the global namespace. A temporary band-aid of `using namespace LMUFFB;` was previously applied, which defeated the purpose of "Global Namespace Elimination." The correct architectural fix was implemented: `calculate_soft_lock` was fully decoupled from the `FFBEngine` class and converted into a standalone free function (`LMUFFB::SteeringUtils::CalculateSoftLock`) allowing clean namespace encapsulation without compiler errors.
- **Test Runner and Main Entry Points:** Moving `Config` and `FFBEngine` to `namespace LMUFFB` broke the test suite and the main entry point. Adding `using namespace LMUFFB;` to `main_test_runner.cpp`, `test_ffb_common.h`, and `main.cpp` restored compilation hygiene for these "boundary" files without requiring them to be fully refactored into the internal Unity chunk immediately. This allows the core to remain pure while legacy runners continue to function.
- **Unity Build Verification (v0.7.235):** The project now successfully compiles `unity_0_cxx.cxx` containing core modules (`VehicleUtils`, `Config`, `FFBEngine` (partially)). Total tests (629/629) pass under the new namespaced architecture.
- **Incremental Unity Build Securing (v0.7.237):** Secured namespace encapsulation for `FFBEngine`, `FFBSafetyMonitor`, `FFBMetadataManager`, and `GripLoadEstimation`. These modules are now fully integrated into the `LMUFFB` namespace and whitelisted for Unity builds. Standardized forward declarations and fixed ambiguity issues in test suites.
- **Missing Unity Whitelist Entries:** During Phase 2/3, files were successfully wrapped in `namespace LMUFFB` but were not explicitly added to the `UNITY_READY_MAIN` variable in `CMakeLists.txt`. While the code compiled correctly as standalone translation units (since `SKIP_UNITY_BUILD_INCLUSION` was naturally left active for them), this bypassed the core goal of proving their safety *within* the batched Unity chunk. We manually appended `Config.cpp`, `FFBDebugBuffer.cpp`, `FFBSafetyMonitor.cpp`, and `FFBEngine.cpp` to the whitelist and successfully compiled `unity_0_cxx.cxx` with zero ODR (One Definition Rule) violations.


### 8.2 Deviations from the Plan
- **Skipping Class Methods for Initial Refactoring:** We originally considered `physics/SteeringUtils.cpp` as the first `.cpp` file to wrap inside the Unity pipeline. However, since it exclusively contains implementation methods belonging to the globally declared `FFBEngine` class (e.g., `void FFBEngine::calculate_soft_lock`), wrapping it in `namespace LMUFFB` immediately triggers "class not declared" compiler errors. We deviated by selecting `VehicleUtils.cpp` instead, as its purely standalone logic is safely isolated from the monolithic classes.

### 8.3 Suggestions for the Future
- **Piecemeal Testing:** Do not blindly chain test scripts via semicolon `;` to compilation scripts during active refactoring. Explicitly monitor the compiler output directly to immediately catch `identifier not found` errors triggered by missing namespace qualifications.
- **Phase 3 Readiness (The Monoliths):** When approaching Phase 3 (`FFBEngine.h` / `.cpp`), we must anticipate cascading architectural changes across the entire hook surface (`DirectInputFFB.cpp` and `main.cpp`). Because `FFBEngine` fundamentally governs the physics tree, transitioning it into `namespace LMUFFB` will require a meticulously controlled, large-scale commit.

### 8.4 Implementation Notes (v0.7.240)
- **Encountered Issues:** None. The `UpSampler` module was already correctly namespaced within `LMUFFB`, and its consumers in `main.cpp` were already using the `LMUFFB` namespace. The primary task was build system integration.
- **Deviations from the Plan:** None. The task was executed as a single incremental step to conclude Phase 3 as instructed.
- **Suggestions for the Future:** Now that Phase 3 is complete, Phase 4 should proceed with high caution regarding external OS headers. It is recommended to refactor `AsyncLogger` next, as it has the fewest OS-specific dependencies compared to `DirectInputFFB` or `DXGIUtils`.

### 8.5 Implementation Notes (v0.7.244)
- **Encountered Issues:** None.
- **Deviations from the Plan:**
  - Refactored `AsyncLogger.h` only.
  - Investigation confirmed that `AsyncLogger` is a header-only module; no `src/logging/AsyncLogger.cpp` exists in the repository. The plan's mention of a `.cpp` file for this module appears to be a documentation artifact, similar to `PerfStats.h` or `RateMonitor.h` which are also header-only.
  - Verified that all methods in `AsyncLogger` (e.g. `Get()`, `Start()`, `Stop()`, `Log()`) are defined inline within the class body, satisfying ODR requirements for Unity Builds.
  - Confirmed via local compilation (standard and Unity mode) and full test suite execution (631 tests passing) that no linker or compilation errors were introduced.
- **Suggestions for the Future:** Continue Phase 4 by refactoring `DirectInputFFB.h/.cpp` or `RestApiProvider.h/.cpp`, which do possess implementation files.

### 8.6 Implementation Notes (v0.7.246)
- **Encountered Issues:** None.
- **Deviations from the Plan:** None. The `DirectInputFFB` module was refactored and integrated into the Unity Build pipeline as instructed.
- **Suggestions for the Future:** Continue Phase 4 by refactoring `RestApiProvider.h/.cpp` or `DXGIUtils.h/.cpp`.

### 8.7 Implementation Notes (v0.7.247)
- **Encountered Issues:** None.
- **Deviations from the Plan:** None. The `RestApiProvider` module was refactored and integrated into the Unity Build pipeline as instructed.
- **Suggestions for the Future:** Continue Phase 4 by refactoring `gui/DXGIUtils.h/.cpp` or `io/GameConnector.h/.cpp`.

### 8.8 Implementation Notes (v0.7.248)
- **Encountered Issues:** None.
- **Deviations from the Plan:** None. The `DXGIUtils` module was refactored and integrated into the Unity Build pipeline as instructed.
- **Suggestions for the Future:** Continue Phase 4 by refactoring `io/GameConnector.h/.cpp` or other remaining UI-related modules.

### 8.9 Implementation Notes (v0.7.249)
- **Encountered Issues:**
  - Initial refactor introduced build breakages due to missing namespace qualification in call sites across `main.cpp`, `GuiLayer_Common.cpp`, and numerous test files.
  - A typo (`mNumVeholes` instead of `mNumVehicles`) was accidentally introduced during automated string replacement.
- **Deviations from the Plan:**
  - Expanded scope to update all call sites project-wide to use `LMUFFB::GameConnector` to restore build stability.
  - Updated `tests/test_ffb_common.h` and `tests/test_ffb_common.cpp` to ensure `GameConnectorTestAccessor` remains compatible with the namespaced `GameConnector`.
- **Suggestions for the Future:** Phase 4 is technically complete for core OS boundaries. Proceed to Phase 5 (UI & Final Integration).

### 8.10 Implementation Notes (v0.7.250)
- **Encountered Issues:**
  - Encountered linker errors regarding platform-agnostic helper functions (e.g., `ResizeWindowPlatform`). These were originally global and called from `GuiLayer_Common.cpp`. Because `GuiLayer_Common.cpp` was moved into the Unity chunk, it could no longer see the global definitions if the platform-specific files weren't also wrapped and included. Fixed by moving all platform helpers and the `IGuiPlatform` interface into `namespace LMUFFB`.
  - Discovered a namespace visibility issue for `GuiLayerTestAccess`. As a `friend` class declared in the global scope but trying to access a namespaced class, it required a global forward declaration and explicit qualification (`friend class ::GuiLayerTestAccess`) in `GuiLayer.h`.
  - Encountered "static function declared but not defined" errors for platform helpers like `WndProc` and `CreateDeviceD3D` on Windows when bundled in Unity builds. Resolved by moving forward declarations and definitions into an anonymous namespace within `namespace LMUFFB`.
  - A typo `SOP_OUTPUT_SMOOTHING` (intended to be `SLOPE_OUTPUT_SMOOTHING`) caused compilation failures in `GuiLayer_Common.cpp`.
  - Linker error `undefined reference to GuiLayerTestAccess::GetLastLaunchArgs` occurred due to improper namespacing of test-only globals. Fixed by moving these members into the `GuiLayer` class under `LMUFFB_UNIT_TEST`.
  - Discovered missing no-op stubs for `LaunchLogAnalyzer` and `UpdateTelemetry` in the `#else` (headless) block of `GuiLayer_Common.cpp` which broke non-ImGui builds.
  - Linker error `unresolved external symbol ImGui_ImplWin32_WndProcHandler` occurred on Windows because the `extern` declaration was mistakenly placed inside an anonymous namespace, causing incorrect symbol mangling. Fixed by moving the declaration to the global scope.
- **Deviations from the Plan:** None. The GUI layer was successfully namespaced and whitelisted for Unity builds.
- **Suggestions for the Future:** Phase 5 is nearly complete. The final step is to clean up `core/main.cpp` by removing temporary `using namespace` directives and fully qualifying remaining calls, and then proceeding to Phase 6 (Subsystem Namespace Migration).

### 8.11 Implementation Notes (v0.7.251)
- **Encountered Issues:**
  - Moving globals (`g_running`, `g_engine`, etc.) to `namespace LMUFFB` required updating `extern` declarations in multiple files. Failure to wrap these `extern` declarations in `namespace LMUFFB` would result in linker errors due to name mangling mismatches.
  - Test files that utilized these globals as mocks (e.g., `test_main_harness.cpp`) also required wrapping their declarations and ensuring they match the core's namespacing.
  - Discovered and fixed typos in `main.cpp` where extended monitors were updating the wrong variables (`mVelX` instead of `mVelY`/`mVelZ`).
  - Resolved a code review finding regarding naming inconsistencies between namespaced and global entry points.
- **Deviations from the Plan:**
  - Extracted `ChannelMonitor` and `ChannelMonitors` into a dedicated header `src/logging/ChannelMonitor.h` to improve modularity and enable robust regression testing of the 28+ telemetry channel updates.
  - Standardized on `LMUFFB::lmuffb_app_main` as the namespaced entry point to ensure link-time compatibility with existing unit test conventions.
- **Suggestions for the Future:** With Phase 5 complete and all core files whitelisted for Unity builds, Phase 6 can begin. This will involve moving files from the root `LMUFFB` namespace into more granular sub-namespaces (`LMUFFB::Physics`, `LMUFFB::GUI`, etc.). Start with the leaf modules refactored in Phase 1.

### 8.12 Implementation Notes (v0.7.253)
- **Encountered Issues:**
  - Namespacing `Logger.h` required widespread updates across the codebase.
  - **Code Review Finding (Header Pollution):** Initial implementation included `using namespace LMUFFB::Logging;` in `FFBEngine.h`. This was flagged as an anti-pattern that pollutes all dependent files. Resolved by removing the directive and using qualified names where necessary.
  - **Code Review Finding (Bridge Placement):** Temporary `using` bridges were initially placed in the global namespace, which would break qualified lookups like `LMUFFB::Logger`. Resolved by wrapping all bridges in `namespace LMUFFB { ... }` within the headers.
  - **Code Review Finding (Doc Inconsistency):** The implementation refactored all six logging files, but documentation initially claimed only a subset were done and listed the others as "Next Steps". Resolved by updating the Progress Checklist, Implementation Notes, and Next Steps to accurately reflect the full directory migration and its scope.
  - **Code Review Finding (FFBSafetyMonitor.h Hygiene):** Accidentally introduced `using namespace LMUFFB::Logging;` inside `namespace LMUFFB` in `FFBSafetyMonitor.h`. This was flagged as redundant and a violation of the established no-header-pollution rule. Resolved by removing the directive.
  - **Code Review Finding (Doc Typo):** Fixed a formatting regression where a backtick was replaced by a double quote in the progress checklist.
  - Handled namespace ambiguity for `FFBEngine` within `AsyncLogger.h` by using a qualified `using` declaration.
- **Deviations from the Plan:**
  - Namespaced all six logging files instead of just the initial two, as it proved more maintainable for the directory's internal consistency.
- **Suggestions for the Future:** Continue Phase 6 by transitioning `src/utils/` files (e.g., `MathUtils.h`, `TimeUtils.h`, `StringUtils.h`) to `namespace LMUFFB::Utils`.

## 9. Next Steps: Phase 6 - Subsystem Namespace Migration
Phase 5 is now complete. All core project files are encapsulated within the `LMUFFB` namespace and are whitelisted for Unity Builds. The next objective is to improve architectural modularity by migrating modules into granular sub-namespaces.

### Your Objectives for the Next PR:
1. **Continue Phase 6 (Subsystem Namespace Migration):**
   - Transition `src/utils/` files (e.g., `MathUtils.h`, `TimeUtils.h`, `StringUtils.h`) to `namespace LMUFFB::Utils`.
   - Transition `src/physics/` files to `namespace LMUFFB::Physics`.
   - Update call sites in `FFBEngine.cpp`, `main.cpp`, and GUI layer accordingly.

### Critical Reminder for Unity Builds
*   **The Include Rule:** Continue to enforce strict discipline: all `#include` directives **MUST** be outside namespace blocks.
*   **Internal Linkage:** Use anonymous namespaces for any helper functions or constants within `.cpp` files to avoid ODR violations when bundled.

### See also:

See a code review of the changes for one iteration of Phase 6. See in particular the final section on recommendations for remaining open (minor) items. 

* `docs\dev_docs\code_reviews\code_review_unity_build_phase6_v0.7.253.md`