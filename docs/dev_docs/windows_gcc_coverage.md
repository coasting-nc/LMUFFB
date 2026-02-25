# Windows GCC Coverage Guide

This document explains how to build the LMUFFB project on Windows using the GCC compiler (via MinGW-w64 or MSYS2) and generate code coverage reports that match the Linux output format.

## Prerequisites

1.  **MinGW-w64 / MSYS2**: You must have a GCC toolchain for Windows. We recommend the UCRT64 environment (modern MSYS2).
    -   `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python-pip mingw-w64-ucrt-x86_64-python-lxml`
2.  **gcovr**: Install via pip (requires the break-system-packages flag in modern MSYS2):
    -   `pip install gcovr --break-system-packages`
3.  **CMake**: The UCRT64 version is installed in step 1.

## Build and Coverage Workflow

### 1. Configure the Build
Use the `ENABLE_COVERAGE` flag and specify the "Unix Makefiles" or "Ninja" generator to use GCC instead of MSVC.

```powershell
# In an MSYS2 UCRT64 shell
cmake -S . -B build_gcc -G "Ninja" -DBUILD_HEADLESS=ON -DENABLE_COVERAGE=ON
```

### 2. Compile
```powershell
cmake --build build_gcc
```

### 3. Run Tests
Running the tests will generate `.gcda` files in the build directory.

```powershell
./build_gcc/tests/run_combined_tests.exe
```

### 4. Generate the Report
Use `gcovr` to aggregate the results. This command produces an HTML report and a Cobertura XML file, identical to the Linux artifacts.

```bash
# Create report directory
mkdir -p coverage_reports

# Generate HTML report
gcovr -r . --html --html-details -o coverage_reports/coverage_gcc.html --filter src/

# Generate Cobertura XML (for CI/CD)
gcovr -r . --xml -o coverage_reports/cobertura_gcc.xml --filter src/
```

## Troubleshooting GCC on Windows

### Missing Windows Libraries
MinGW-w64 provides its own versions of Windows SDK libraries. If you encounter linker errors for `dinput8` or `dxguid`, ensure you are linking with the library names provided by your toolchain:
-   MSVC: `dinput8.lib`, `dxguid.lib`
-   GCC: `-ldinput8`, `-ldxguid`, `-lwinmm`, `-lversion`, `-limm32`, `-ldxgi`

The project's `CMakeLists.txt` is designed to handle these names automatically across compilers.

### DirectInput 8 Compatibility
MinGW-w64's `dinput.h` is generally compatible with MSVC's. If you encounter missing macro definitions, ensure you have set `#define DIRECTINPUT_VERSION 0x0800` before including the header, which is already handled in `DirectInputFFB.h`.

## Comparison with Linux
The reports generated via this method are bit-for-bit compatible in terms of logic coverage with the Linux reports, as both environments use the same GCC-based instrumentation (`--coverage`). This allows for consistent tracking of the **99% Coverage Policy** regardless of the developer's operating system.
