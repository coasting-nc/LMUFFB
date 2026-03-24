# Incremental Unity Build Plan for Tests

## 1. Can we enable Unity (Jumbo) builds ONLY for tests?
**Yes.** CMake allows setting the `UNITY_BUILD` property on a per-target basis. Rather than using the global `CMAKE_UNITY_BUILD` which affects the entire project (including `LMUFFB_Core` and the main `LMUFFB` executable), we can strictly enable it for the `run_combined_tests` target. 

This is highly beneficial since `LMUFFB_Core` actually builds quite quickly, while the 100+ files in the `tests/` directory are the primary bottleneck for compilation times.

## 2. How to fix clashing definitions incrementally?
When concatenating multiple `.cpp` files into single translation units (chunks), any global symbols, constants, or helper functions that share the same name across different test files will result in compiler errors (e.g., `Multiple Definition` or `Redefinition of 'some_helper'`).

CMake provides a file-level property called `SKIP_UNITY_BUILD_INCLUSION`. We can build an **"opt-in" list (whitelist)** of test files that have been cleaned up and verified to be "Unity-safe". Any file not in the list will automatically skip the Unity build process and compile normally as a standalone `.cpp` file.

## 3. Step-by-Step Implementation Plan

### Step 1: Update `tests/CMakeLists.txt`
Add the following CMake logic just before `enable_testing()` in `tests/CMakeLists.txt`. This creates our whitelist system:

```cmake
# 1. Define the whitelist of files we have verified to be Unity-safe
set(UNITY_READY_TESTS
    test_math_utils.cpp
    test_vehicle_utils.cpp
    test_perf_stats.cpp
    # Add more files here as we fix them...
)

# 2. Enable Unity Builds strictly for the test target
set_target_properties(run_combined_tests PROPERTIES 
    UNITY_BUILD ON
    UNITY_BUILD_BATCH_SIZE 15 # Group 15 files per chunk
)

# 3. Disable Unity inclusion for any test NOT in the whitelist
foreach(src ${TEST_SOURCES})
    # Get the base file name in case paths are relative
    get_filename_component(src_name ${src} NAME)
    
    list(FIND UNITY_READY_TESTS ${src_name} index)
    if(index EQUAL -1)
        # Not in the whitelist -> Compile as standalone file
        set_source_files_properties(${src} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
    endif()
endforeach()
```

### Step 2: The Incremental Workflow
For the developer migrating the tests to a Jumbo build, the workflow is as follows:

1. **Pick the next batch:** Move 5 to 10 files into the `UNITY_READY_TESTS` variable in `tests/CMakeLists.txt`.
2. **Compile:** Run the build. CMake will automatically merge the files in the whitelist into chunk files (e.g., `unity_0_cxx.cxx`) and pass them to the compiler.
3. **Fix Collisions:** If the build fails due to clashing helper definitions (e.g., two tests define `const int FAKE_HZ = 400;` or `void SetupTelemetry()`), apply one of the following fixes to the offending files:
   - **Fix A (Best Practice):** Wrap the test file's private helper functions and global constants in an anonymous namespace:
     ```cpp
     namespace {
         const int FAKE_HZ = 400;
         void SetupTelemetry() { ... }
     }
     ```
   - **Fix B (Standard):** Mark global helper functions explicitly as `static`:
     ```cpp
     static void SetupTelemetry() { ... }
     ```
   - **Fix C (Scoping):** Wrap the entirety of the file (excluding standard includes) in a uniquely named namespace like `namespace Test_VehicleUtils { ... }`.
4. **Verify tests pass:** Run `python scripts/run_all_tests.py` or `ctest` to ensure the compilation didn't inadvertently change the logical behavior of the tests by overlapping state.
5. **Commit and Repeat:** Commit the fixes and the updated whitelist to source control. Move on to the next 5-10 files.

### Step 3: Removing the Whitelist (The End Goal)
## 4. Implementation Progress (v0.7.227)

The unity build system has been fully expanded and now covers the entire test suite (with very specific exceptions).

### Current Status
- **Full Suite Integration**: The `UNITY_READY_TESTS` logic now automatically includes ALL source files in `tests/`, excluding only entry points and a small `UNITY_SKIP_LIST`.
- **Tests Bundled**: **150+ test files** are now processed through the unity build system.
- **Single Batch Optimization**: The `UNITY_BUILD_BATCH_SIZE` has been set to **0** (unlimited). This means the 150+ whitelisted test files are now compiled into a **single massive chunk** (`unity_0_cxx.cxx`), maximizing compilation throughput by parsing shared headers only once for the entire suite.
- **Excluded Files**: Only **8 files** remain in the `UNITY_SKIP_LIST` due to fundamental symbol clashes in legacy GUI and persistence tests that would require refactoring to merge. These files are compiled standalone.
- **CI/CD Integration**: Unity builds are now explicitly enabled in both `windows-build-and-test.yml` and `manual-release.yml` GitHub workflows using the `-DLMUFFB_USE_UNITY_BUILD=ON` flag.

### Results & Performance
- **Zero-Code-Change Expansion**: Successfully moved over 140 files to the whitelist without any code modifications.
- **Symbol Clash Resolution**: Optimized 6 test cases by renaming duplicate `TEST_CASE` names that only became collisions when bundled into a single batch (e.g., `test_legacy_config_migration`).
- **Robustness Fixes**: 
  - Hardened `GameConnectorTestAccessor::Reset()` to properly clear `std::chrono` timers, resolving race conditions in transition logging tests.
  - Fixed uninitialized memory in Linux coverage tests (`test_game_connector_branch_boost`) to ensure stability in high-speed unity environments.

### Build Commands

The unity build is now a core feature of the test target. You can build with unity enabled using your standard command, but it is recommended to explicitly enable it via CMake to ensure consistency:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build -DLMUFFB_USE_UNITY_BUILD=ON; cmake --build build --config Release
```

**Key Optimizations over Standard Builds:**
1. **Header Parsing Elimination**: Instead of 150+ separate translation units parsing `test_ffb_common.h` and `<iostream>`, the compiler only parses them **once** for the entire test suite chunk.
2. **Binary Sizes**: Bundling tests into a single TU allows the compiler to perform more aggressive cross-function optimizations and significantly reduces object file overhead.
3. **CI Throughput**: Compilation time in GitHub Actions has been drastically reduced, allowing for faster PR feedback and shorter build cycles.
