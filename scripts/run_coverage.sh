#!/bin/bash
set -e

# Configuration
BUILD_DIR="build_coverage"
LCOV_REPORT="coverage.info"
LCOV_REPORT_FILTERED="coverage_filtered.info"
OUTPUT_DIR="coverage_report"

echo "=== Configuring Build with Coverage ==="
cmake -S . -B $BUILD_DIR -DBUILD_HEADLESS=ON -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug

echo "=== Building ==="
cmake --build $BUILD_DIR --config Debug

echo "=== Initializing Counters ==="
lcov --directory . --zerocounters

echo "=== Running Tests ==="
./$BUILD_DIR/tests/run_combined_tests

echo "=== Collecting Coverage Data ==="
lcov --capture --directory . --output-file $LCOV_REPORT
lcov --extract $LCOV_REPORT "*/src/*" --output-file $LCOV_REPORT_FILTERED

echo "=== Generating HTML Report ==="
genhtml $LCOV_REPORT_FILTERED --output-directory $OUTPUT_DIR

echo "=== Coverage Summary ==="
lcov --list $LCOV_REPORT_FILTERED

echo ""
echo "Report generated at: $OUTPUT_DIR/index.html"
