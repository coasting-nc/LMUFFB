# Report: Preparing Main Code for Unity Builds

This report outlines the strategy for preparing the main `LMUFFB` codebase to support Unity (Jumbo) builds, enforcing strict namespace rules, and mitigating the common drawbacks associated with unified translation units.

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
