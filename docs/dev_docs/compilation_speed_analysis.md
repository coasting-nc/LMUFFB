# Compilation Speed Analysis and Future Improvements

## Current State Analysis

The LMUFFB project utilizes GitHub Actions for Continuous Integration across several environments. As of version 0.7.186, the average full build times are:

-   **Windows Release Build**: ~3-4 minutes.
-   **Linux Sanitizers (ASan/UBSan)**: ~4 minutes.
-   **Linux Coverage (Debug)**: ~9-12 minutes.

### The Linux Coverage Bottleneck
The "Linux Coverage" workflow is the significant outlier. This is primarily due to the nature of GCOV/LCOV instrumentation. When `--coverage` is enabled, the compiler must insert counter-increment instructions at every branch point. This significantly increases the complexity of the Abstract Syntax Tree (AST) and the workload for the code generator, leading to much longer compilation times compared to standard Debug or Release builds.

## Existing Optimizations

We are already adopting several strategies to mitigate compilation times:

### 1. Library Splitting (`LMUFFB_Core` vs `LMUFFB_Core_Fast`)
The `CMakeLists.txt` defines two separate static libraries for the core logic:
-   **`LMUFFB_Core`**: Compiled with full optimizations (`/O2` on MSVC, `-O3` on GCC) for the main application. This ensures users get a high-performance, real-time FFB experience.
-   **`LMUFFB_Core_Fast`**: Compiled with minimal optimizations (`/Od` on MSVC, `-O0` on GCC) specifically for the test suite.

This approach follows the best practice that tests do not need to run at peak performance—they just need to run correctly and finish quickly. Reducing optimization levels is the single most effective way to speed up C++ compilation.

### 2. Multi-Processor Compilation
On Windows, we use the `/MP` flag in MSVC to enable parallel compilation of translation units. In Linux CI environments, `make -j$(nproc)` is typically used to achieve the same effect.

---

## Future Optimization Opportunities

The following approaches are ranked from **easiest to implement (low-hanging fruit)** to **most complex**.

### 1. Ccache (Compiler Cache)
**Complexity**: Low | **Impact**: High (for incremental/repeat builds) | **Compatibility**: Orthogonal (Works with all other methods).

Ccache acts as a persistent cache for object files. It hashes the preprocessed source code and compiler flags to determine if a file can be retrieved from the cache rather than recompiled.

-   **Pros**:
    -   Near-instant recompilation for unchanged files.
    -   Dramatically reduces CI time for small PRs, documentation updates, or changes that only touch one of the 100+ translation units.
-   **Cons**:
    -   Requires persistent storage between CI runs (GitHub's `actions/cache`).
    -   Small overhead for cache misses.
-   **Implementation**:
    -   Install `ccache` in the CI runner.
    -   Add `set(CMAKE_CXX_COMPILER_LAUNCHER ccache)` to the top-level `CMakeLists.txt`.
    -   Configure GitHub Actions to cache the `.ccache` directory using a key based on the OS and a hash of the environment.

### 2. Splitting the Test Executable
**Complexity**: Medium | **Impact**: Medium (Link-time optimization) | **Compatibility**: Orthogonal.

Currently, `run_combined_tests` links over 100 translation units into a single large binary. Linking is a serial, single-threaded process in most toolchains (LD, MSVC Linker) and has become a bottleneck as the test count exceeds 500.

-   **Pros**:
    -   Enables parallel linking by creating multiple smaller binaries.
    -   Reduces the "wait-for-one-big-link" phase that occurs at the end of the build.
    -   Improves developer productivity by allowing them to compile and run only the relevant sub-suite (e.g., `test_physics` vs `test_gui`).
-   **Cons**:
    -   Increased complexity in `tests/CMakeLists.txt`.
    -   Requires managing multiple test results in CI.
-   **Implementation**:
    -   Refactor `tests/CMakeLists.txt` to group tests into functional categories.
    -   Create a static library `LMUFFB_Test_Infrastructure` containing shared helpers (like `test_ffb_common.cpp`).
    -   Define multiple executables: `test_physics`, `test_diagnostics`, `test_io_gui`, etc.

### 3. Precompiled Headers (PCH)
**Complexity**: Medium | **Impact**: High | **Compatibility**: Orthogonal to Ccache; can be combined with Unity Builds with care.

Standard headers like `<chrono>`, `<vector>`, `<string>`, and the vendor-provided `imgui.h` are re-parsed by the compiler for every single test file. This is redundant and slow.

-   **Pros**:
    -   Common headers are parsed once and stored in a binary format.
    -   Significant reduction in the "Parsing" phase of compilation.
-   **Cons**:
    -   Changes to the PCH header trigger a full rebuild of the entire project.
    -   Requires careful selection of only "stable" headers that rarely change.
-   **Implementation**:
    -   Create `tests/pch.h` containing:
        ```cpp
        #include <vector>
        #include <string>
        #include <chrono>
        #include <iostream>
        #include "imgui.h"
        ```
    -   Use `target_precompile_headers(run_combined_tests PRIVATE pch.h)`.
-   **Refactoring**: Requires an audit to ensure files aren't accidentally relying on the PCH for project-specific headers (which should remain in the individual `.cpp` files to maintain build granularity).

### 4. Unity Builds (Jumbo Builds)
**Complexity**: High | **Impact**: Very High | **Compatibility**: Orthogonal to Ccache; can conflict with PCH if not managed by CMake.

Unity builds merge multiple `.cpp` files into a few large translation units. This is the "nuclear option" for reducing parsing overhead and improving compiler optimization context.

-   **Pros**:
    -   Drastic reduction in file I/O and redundant header parsing.
    -   Can reduce build times by 50-80% for large suites.
-   **Cons**:
    -   **Symbol Collisions**: Internal linkage symbols (static variables/functions) with the same name in different files will collide.
    -   **Hidden Dependency Bugs**: A missing include in `file_b.cpp` might be satisfied by an include in `file_a.cpp` if they happen to be merged together, leading to "ghost" build failures when files are added or removed.
    -   **Slow Incremental Builds**: Modifying one small test file requires recompiling the entire "unity chunk."
-   **Implementation and Robustness Strategy**:
    -   **Strict Namespacing**: Refactor all test files to use unique namespaces or anonymous namespaces for all internal helper functions and variables.
        -   *Example*: `namespace FFBEngineTests::Braking { ... }` instead of just `namespace FFBEngineTests { ... }`.
    -   **Macro Hygiene**: Ensure all file-local macros are uniquely named or `#undef` at the end of the file.
    -   **Automated IWYU Checks**: To prevent hidden dependency bugs, the CI should run a "Standard Build" (Non-Unity) on a weekly basis or for all merges to `main`. Alternatively, use a tool like `include-what-you-use` (IWYU) to enforce correct include hygiene.
    -   **CMake Refactoring**:
        ```cmake
        set_target_properties(run_combined_tests PROPERTIES
            UNITY_BUILD ON
            UNITY_BUILD_BATCH_SIZE 20
        )
        ```
    -   **Exclusion Policy**: Mark files that rely on complex global state or third-party logic as `SKIP_UNITY_BUILD_INCLUSION` to avoid refactoring impossible-to-fix collisions.

---

## Roadmap Comparison and Combination

All proposed approaches can be combined, although Unity Builds and PCH provide overlapping benefits (Unity builds effectively simulate a PCH by only parsing headers once per chunk).

| Strategy | Complexity | Potential Reduction | Compatibility | Recommendation |
|----------|------------|---------------------|---------------|----------------|
| **Ccache** | Low | 50% (Incremental) | All | Implement immediately. |
| **PCH** | Medium | 20-30% | Ccache, Split | Highly Recommended. |
| **Split Exec** | Medium | 10-15% (Linking) | All | Do as suite grows > 750 tests. |
| **Unity Build** | High | 60-70% | Ccache | Best for CI-only workflows. |

**Verdict**: The most robust future-proof path is implementing **Ccache** and **PCH** first. **Unity Builds** offer the highest reward but require a significant one-time refactoring effort to ensure name safety and include hygiene across the 100+ existing test files.
