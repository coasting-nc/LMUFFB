# Linux Test Coverage Guide

This document explains how to generate and view code coverage reports for the LMUFFB project on Linux.

## Prerequisites

To collect coverage data, you need the following tools installed:
- **GCC** (standard on most Linux distros)
- **lcov** (to process coverage data)
- **genhtml** (part of the lcov package, to generate HTML reports)

On Ubuntu/Debian, you can install them with:
```bash
sudo apt-get update
sudo apt-get install -y lcov
```

### Automated Script

For convenience, you can use the provided script to run the entire flow:
```bash
chmod +x scripts/run_coverage.sh
./scripts/run_coverage.sh
```

### Manual Step-by-Step Instructions

### 1. Configure the Build with Coverage Enabled

Run CMake with the `ENABLE_COVERAGE` flag. This will add the necessary compiler flags (`--coverage -O0 -g`) to the build.

```bash
cmake -S . -B build -DBUILD_HEADLESS=ON -DENABLE_COVERAGE=ON
```

*Note: `-O0` is important to ensure that the coverage mapping accurately reflects the source code without compiler optimizations.*

### 2. Build the Project and Tests

```bash
cmake --build build --config Debug
```

### 3. Initialize Coverage Counters

Reset the coverage counters before running tests to ensure a clean report.

```bash
lcov --directory . --zerocounters
```

### 4. Run the Tests

Execute the combined test suite to generate coverage data files (`.gcda`).

```bash
./build/tests/run_combined_tests
```

### 5. Collect Coverage Data

Capture the coverage info from the build directory. We exclude system headers and third-party vendor code to focus on our `src` directory.

```bash
lcov --capture --directory . --output-file coverage.info
# Filter to only include src directory
lcov --extract coverage.info "*/src/*" --output-file coverage_filtered.info
```

### 6. Generate HTML Report

Convert the captured data into a human-readable HTML report.

```bash
genhtml coverage_filtered.info --output-directory coverage_report
```

### 7. View the Results

Open `coverage_report/index.html` in your web browser.

---

## 99%+ Coverage Policy

For all new features and bug fixes:
1. **Mandatory Coverage**: All new code in the `src` directory MUST have 99% or higher test coverage.
2. **Regression Testing**: Existing coverage must not decrease.
3. **Iterative Process**: If coverage falls below 99%, you must iterate by adding more unit tests or edge cases until the threshold is met.

See [fixer.md](fixer.md) for more details on the quality enforcement process.
