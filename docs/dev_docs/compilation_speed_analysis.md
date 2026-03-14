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

## Future Optimization Opportunities

Despite current efforts, there is room for improvement, especially for the 10+ minute Coverage builds.

### 1. Unity Builds (Jumbo Builds)
CMake supports a feature called `CMAKE_UNITY_BUILD`. This merges multiple `.cpp` files into a single "super" translation unit before compilation.
-   **Pros**: Dramatically reduces the overhead of re-parsing header files (like `FFBEngine.h` or ImGui headers) across 500+ test files.
-   **Cons**: Can hide missing include errors and may lead to symbol collisions if namespacing isn't strict.

### 2. Precompiled Headers (PCH)
Headers like `<chrono>`, `<vector>`, `<string>`, and especially the vendor-provided `imgui.h` are included in almost every test file.
-   **Proposed Action**: Use `target_precompile_headers` for the test runner. This allows the compiler to parse these headers once and reuse the binary representation, which could shave 1-2 minutes off the Linux build.

### 3. Ccache (Compiler Cache)
In CI environments, using a tool like `ccache` can provide near-instant recompilation for unchanged files across different workflow runs. While GitHub Actions provides cache actions, `ccache` is more granular for C++ objects.

### 4. Splitting the Test Executable
The `run_combined_tests` executable currently links over 100 translation units. Linking is a single-threaded process and can become a bottleneck.
-   **Proposed Action**: Split the suite into several smaller binaries (e.g., `test_physics`, `test_io`, `test_diagnostics`). This allows the linker to work in parallel on multiple smaller tasks.

## Conclusion and Justification

The current project architecture correctly prioritizes **user performance** (optimized `LMUFFB_Core`) while attempting to maintain **developer velocity** (unoptimized `LMUFFB_Core_Fast`).

The disparity in Linux Coverage times is an acceptable trade-off for the high-fidelity diagnostic data it provides. However, as the project grows towards 1,000+ tests, implementing **Precompiled Headers** and **Unity Builds** for the test suite should be the next priority to keep CI times under the 5-minute mark.
