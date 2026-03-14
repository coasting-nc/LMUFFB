# Ccache Implementation Notes

## Overview
Ccache (Compiler Cache) has been integrated into the LMUFFB build system and GitHub Actions CI pipelines to accelerate compilation by caching and reusing object files.

## Changes

### 1. Build System (`CMakeLists.txt`)
- Added `find_program(CCACHE_PROGRAM ccache)` to detect the presence of Ccache.
- If found, `CMAKE_CXX_COMPILER_LAUNCHER` and `CMAKE_C_COMPILER_LAUNCHER` are set to `ccache`. This ensures that any C or C++ compilation initiated by CMake will automatically use Ccache if available.

### 2. GitHub Actions CI (`.github/workflows/windows-build-and-test.yml`)
Ccache has been enabled for the following jobs:
- `windows-release` (Windows/MSVC)
- `linux-coverage` (Linux/GCC)
- `linux-sanitizers` (Linux/GCC)

#### Implementation Details:
- **Installation**:
  - Windows: Installed via `choco install ccache`.
  - Linux: Installed via `apt-get install ccache`.
- **Persistence**:
  - Used `actions/cache@v4` to persist the `.ccache` directory across workflow runs.
  - The cache key is structured as `${{ runner.os }}-ccache-${{ github.sha }}` with a restore key of `${{ runner.os }}-ccache-`.
- **Configuration**:
  - `CCACHE_DIR`: Set to `${{ github.workspace }}/.ccache` to ensure it is within the cached directory.
  - `CCACHE_MAXSIZE`: Set to `100M` for Windows and `500M` for Linux jobs.
  - `CCACHE_BASEDIR`: (Implicitly handled or not strictly required for this setup, but `CCACHE_DIR` is explicit).

## Challenges and Considerations

### MSVC Support
Ccache 4.x has significantly improved support for MSVC. However, some specific flags can affect cacheability:
- **Debug Information**: The project uses `/Zi` (Program Database). While Ccache can handle this, it sometimes requires `CCACHE_DEPEND` or specific settings. In this implementation, we rely on Ccache's default handling for MSVC. If cache hit rates are low on Windows, switching to `/Z7` (Embedded Debug Info) might be considered as it is generally more "cache-friendly".

### Cache Size
The cache size is currently capped at 100MB-500MB to stay within GitHub's total cache limits and ensure fast upload/download of the cache itself. This can be adjusted based on the project's growth.

### Cache Key Strategy
Using `${{ github.sha }}` in the key ensures a new cache is saved for every commit (if changes occur), while `${{ runner.os }}-ccache-` as a restore key allows fetching the most recent previous cache.

## Verification
- Local verification confirmed `ccache` is picked up by CMake when present in the PATH.
- Build output shows `Ccache found` status message.
- `ccache -s` can be used to monitor hit/miss statistics.
