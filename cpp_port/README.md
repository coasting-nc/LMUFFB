# LMUFFB C++ Port

This directory contains the C++ implementation of the LMU Force Feedback App.

## Requirements

1.  **Visual Studio 2022** (with C++ Desktop Development workload).
2.  **CMake** (integrated into VS or standalone).
3.  **vJoy SDK**: Download from [vJoy GitHub](https://github.com/shauleiz/vJoy) or website.
    *   Extract the SDK.
    *   Note the path to `inc` and `lib`.

## Building

1.  Open this folder (`cpp_port`) in Visual Studio.
2.  VS should detect the `CMakeLists.txt`.
3.  Edit `CMakeLists.txt` or configure CMake settings to point `VJOY_SDK_DIR` to your vJoy SDK installation.
    *   Default is `C:/Program Files/vJoy/SDK`.
4.  Build the `LMUFFB.exe` target.

## Running

1.  Ensure `vJoyInterface.dll` is in the same folder as the executable (or in system PATH).
2.  Start Le Mans Ultimate.
3.  Run `LMUFFB.exe`.
