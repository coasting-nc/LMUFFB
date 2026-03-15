# Deep Dive: Implementing Unity Builds (Jumbo Builds)

Unity builds (also known as Jumbo builds) significantly reduce compilation time by merging multiple translation units (`.cpp` files) into a single, larger translation unit. This document outlines the technical strategies for implementing Unity builds while maintaining code quality and developer productivity.

## 1. Detecting and Preventing Symbol Collisions

In a standard build, `static` variables and functions, as well as symbols in anonymous namespaces, have internal linkage and are local to their translation unit. In a Unity build, multiple files are concatenated, which can lead to "Multiple Definition" errors if two files use the same name for internal symbols in the same chunk.

### Techniques for Detection
*   **Compiler-Led Detection (The "Fail Fast" Method)**: The most straightforward way to find collisions is to attempt a Unity build. The compiler will immediately error out if `static` functions or variables share the same name in the same "chunk."
*   **Static Analysis (Clang-Tidy)**: Use `clang-tidy` with the `readability-identifier-naming` and `bugprone-suspicious-include` checks. To strictly prevent issues, you can configure certain warnings to be treated as errors (see Q&A below).
*   **Symbol Auditing with `nm` or `dumpbin`**: For Linux (GCC/Clang), a script can run `nm -f posix --defined-only <object_file>` and look for `t` or `b` (local text/data) symbols. If the same local symbol appears in multiple object files destined for the same chunk, a warning can be issued before the build starts.
*   **Rigorous Namespace Usage**: The project should adopt a policy where **no code exists in the global namespace** except for `main`. Every file should be wrapped in a specific namespace (e.g., `namespace LMUFFB::Physics::Internal::[FileName]`) or use anonymous namespaces for all private data.

### Automated Collision Checker Script
A pre-build python script can be used to scan the source tree for potential collisions:
```python
# Pseudo-logic for collision detection
# 1. Parse all .cpp files in a batch.
# 2. Extract 'static' declarations and 'namespace { ... }' content.
# 3. Build a map of name -> [list of files].
# 4. Error out if len(list of files) > 1 for any name.
```

## 2. Preventing "Hidden Dependency" Bugs (Include Hygiene)

Unity builds can hide missing includes. If `A.cpp` includes `<vector>` and `B.cpp` (which uses `std::vector`) is appended after `A.cpp` in a Unity chunk, `B.cpp` will compile even if it forgets to include `<vector>`. This breaks the build if `B.cpp` is ever moved or if the chunking logic changes.

### Mitigation Strategies
*   **Dual-Track CI (Mandatory)**: Maintain at least one CI job (typically the Linux Coverage or Sanitizer job) that **never** uses Unity builds. This ensures that every file's include list is self-sufficient.
*   **"Randomized Chunking"**: Periodically change the `UNITY_BUILD_BATCH_SIZE` or the order of files in CMake. This forces hidden dependencies to surface as build errors.
*   **Include-What-You-Use (IWYU)**: Use a tool that analyzes the AST specifically to ensure every symbol used in a file is explicitly included.

## 3. Include-What-You-Use (IWYU) Integration

IWYU is a Clang-based tool that makes suggestions for your `#includes`.

### How to Integrate with CMake
CMake has built-in support for IWYU. You can enable it by setting the `CMAKE_CXX_INCLUDE_WHAT_YOU_USE` property.

```cmake
# In root CMakeLists.txt or a specific target
find_program(IWYU_PATH include-what-you-use)
if(IWYU_PATH)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH};--transitive_includes_only)
endif()
```

### Strategic Implementation
1.  **IWYU Mapping Files**: Create an `iwyu.imp` file to handle project-specific header redirections (e.g., if you want to include `FFBEngine.h` instead of a private internal header).
2.  **Audit Workflow**: Running IWYU on every build is slow. It should be integrated into a dedicated `Static Analysis` GitHub Action that runs:
    *   On every Pull Request.
    *   Weekly on the `main` branch.
3.  **Refactoring Phase**: Use the `fix_includes.py` script provided by the IWYU project to automatically clean up the 100+ existing test files.

## 4. Mitigating Slow Incremental Builds (CI vs. Local)

The main drawback of Unity builds for developers is that modifying one file triggers the recompilation of its entire chunk (e.g., 20 files). We must isolate this feature for CI only.

### CMake Configuration
Define a CMake option in the root `CMakeLists.txt` that defaults to `OFF`.

```cmake
option(LMUFFB_USE_UNITY_BUILD "Enable Unity (Jumbo) builds for faster CI" OFF)

if(LMUFFB_USE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 20)
    message(STATUS "Unity Build Enabled (Batch Size: ${CMAKE_UNITY_BUILD_BATCH_SIZE})")
else()
    message(STATUS "Unity Build Disabled (Standard incremental builds)")
endif()
```

### GitHub CI Workflow (.yml)
In your `.github/workflows/windows-build-and-test.yml` and `linux-build.yml`, explicitly enable the flag during the configuration step.

```yaml
# Inside the YML steps:
- name: Configure CMake
  run: cmake -B build -DLMUFFB_USE_UNITY_BUILD=ON ...
```

### Local Testing Commands
To test a Unity build locally without changing the default behavior permanently, use the following commands:

```powershell
# 1. Configure with Unity Build enabled
cmake -B build_unity -DLMUFFB_USE_UNITY_BUILD=ON

# 2. Build and check for errors
cmake --build build_unity
```

### Summary of Build Logic
| Environment | `LMUFFB_USE_UNITY_BUILD` | Benefit |
| :--- | :--- | :--- |
| **Local Dev** | `OFF` (Default) | Fast incremental builds (compile exactly what changed). |
| **Local Test** | `ON` (One-time) | Manually audit for symbol collisions locally. |
| **GitHub CI** | `ON` (Explicit) | Fast full builds (minimize runner costs and wait times). |
| **CI Audit** | `OFF` (Explicit) | Detect missing includes and hidden dependencies. |

---

## 5. Frequently Asked Questions (Q&A)

### Q: Can we set some `clang-tidy` warnings to raise errors instead of warnings while keeping others as simple warnings?
**A:** Yes. You can use the `-warnings-as-errors` flag in the `clang-tidy` command line or the `WarningsAsErrors` field in a `.clang-tidy` configuration file.

*   **Command Line**: Add `--warnings-as-errors='readability-*,bugprone-*'` to your `clang-tidy` call. This will promote all readability and bugprone warnings to errors, while leaving others (e.g., performance) as warnings.
*   **CMake Integration**:
    ```cmake
    # Promotions done selectively in CMake
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy;--warnings-as-errors=readability-identifier-naming,bugprone-suspicious-include")
    ```

### Q: Do we need to modify the `CMakeLists.txt` to support different builds?
**A:** Yes, once. You add the `option(LMUFFB_USE_UNITY_BUILD ...)` block as shown in Section 4. After that, you **do not** need to edit the file again. The same `CMakeLists.txt` handles both Unity and Standard builds based on the argument you pass to `cmake`.

### Q: How do I choose between Unity and Standard build locally?
**A:** It is controlled entirely by the `-D` flag during the configuration step:
*   **Standard (Default)**: `cmake -B build`
*   **Unity**: `cmake -B build -DLMUFFB_USE_UNITY_BUILD=ON`

The build system will remember this choice in the `build` directory until you delete it or re-run `cmake` with a different flag.

---

## 6. Quick Start: Build Commands

Here is a quick reference for the commands used to configure the build with different optimization and analysis features enabled.

### Standard Build (Default)
Fast incremental builds for daily development.
```powershell
cmake -B build
cmake --build build --config Release
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release
```

### Unity (Jumbo) Build
Fewer translation units, optimized for CI and full rebuilds.
```powershell
cmake -B build -DLMUFFB_USE_UNITY_BUILD=ON
cmake --build build --config Release


```

### Build with IWYU Analysis
Analyze include hygiene during compilation (requires IWYU installed).
```powershell
cmake -B build -DENABLE_IWYU=ON
cmake --build build
```

### Combined (CI Simulation)
The most aggressive build configuration, used to verify the project in a single wide pass.
```powershell
cmake -B build -DLMUFFB_USE_UNITY_BUILD=ON -DENABLE_IWYU=ON
cmake --build build
```



---
