cmake_minimum_required(VERSION 3.10)
project(LMUFFB_Tests)

set(CMAKE_CXX_STANDARD 17)

# Include main source dir for headers
include_directories(..)

# Test Executable
add_executable(run_tests test_ffb_engine.cpp)

# Enable testing
enable_testing()
add_test(NAME CoreLogicTest COMMAND run_tests)
