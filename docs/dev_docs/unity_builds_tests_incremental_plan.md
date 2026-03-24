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
## 4. Implementation Progress (v0.7.225)

The incremental unity build system has been successfully integrated into `tests/CMakeLists.txt`. 

### Current Status
- **Whitelist Enabled**: The `UNITY_READY_TESTS` list is active and controls which files are merged into unity chunks.
- **Tests Bundled**: **28 test files** are currently in the whitelist. With a `UNITY_BUILD_BATCH_SIZE` of 15, these are compiled into **2 unity chunks** (`unity_0_cxx.cxx` and `unity_1_cxx.cxx`).
- **Remaining Tests**: There are approximately **140+ test files** remaining in `TEST_SOURCES`. 

### Can we add more tests without code changes?
**Yes.** Most test files in the suite are already encapsulated within the `FFBEngineTests` namespace, which significantly reduces the risk of global symbol clashes. I successfully added several large batches (25+ files) without encountering a single redefinition error. 

It is estimated that **at least 80%** of the remaining tests can be moved to the whitelist without any code modifications. The only files that require caution are those defining global symbols outside of namespaces or using generic names for helper functions (e.g., `Setup()` or `Init()`).

### Build Commands

To build with unity enabled, you can continue using your standard command:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release
```

**Difference from original build:**
1. **Automatic Detection**: Because `UNITY_BUILD ON` is now a target property in `tests/CMakeLists.txt`, CMake will automatically generate the unity chunks and use them during the build process without requiring any extra flags.
2. **Compilation Speed**: Instead of launching 28 separate compiler processes for the whitelisted files, the compiler now only processes 2 larger files, significantly reducing overhead from header parsing (like `test_ffb_common.h` and `<iostream>`).
3. **Chunk Location**: You can find the generated unity source files in your build directory at:
   `build/tests/CMakeFiles/run_combined_tests.dir/Unity/`
