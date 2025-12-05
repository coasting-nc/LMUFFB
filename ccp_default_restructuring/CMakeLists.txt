cmake_minimum_required(VERSION 3.10)
project(LMUFFB_CPP)

set(CMAKE_CXX_STANDARD 17)

# vJoy SDK Location - Users should set this
set(VJOY_SDK_DIR "C:/Program Files/vJoy/SDK" CACHE PATH "Path to vJoy SDK")

include_directories(${VJOY_SDK_DIR}/inc)
link_directories(${VJOY_SDK_DIR}/lib/amd64)

add_executable(LMUFFB main.cpp rF2Data.h)

target_link_libraries(LMUFFB vJoyInterface.lib)

# Copy dll to output if possible, or instruct user
