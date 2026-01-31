# Implementation Plan: Add Application Icon

## 1. Introduction and Motivation
Currently, the LMUFFB application uses the default Windows executable icon, which lacks visual identity and polish. This task aims to add a custom application icon (`lmuffb.ico`) that will be embedded in the executable and displayed in Windows Explorer and the taskbar.

## 2. Technical Approach

### 2.1. Assets
-   Create a new directory `icon/` to store source assets.
-   Include the source image `icon.png`.
-   Provide a script (`icon/magick-ico.sh`) to generate the `.ico` file from the source PNG using ImageMagick, creating multiple sizes (16, 24, 32, 48, 64, 96, 128, 256) for proper scaling in Windows.
    -   **Note:** This script requires [ImageMagick](https://imagemagick.org/) to be installed and available in the system PATH.
-   Store the generated `lmuffb.ico` in the `icon/` directory.

### 2.2. Resource Files
-   Create `src/resource.h` to define the resource identifier (`IDI_ICON1`).
-   Create `src/res.rc` to define the ICON resource, pointing to `lmuffb.ico`.

### 2.3. Build System (CMake)
-   Modify `CMakeLists.txt` to:
    1.  Copy `icon/lmuffb.ico` to the build directory (so the resource compiler can find it easily).
    2.  Add `src/res.rc` to the `add_executable` source list. This triggers CMake to invoke the Resource Compiler (`rc.exe`) on Windows.

## 3. Testing Plan

### 3.1. Manual Verification
-   Build the application in Release mode.
-   Inspect the generated `LMUFFB.exe` in Windows Explorer to verify it displays the new icon.
-   Run the application and verify the icon appears in the taskbar / window title bar (if handled by window creation, though resource usually handles the executable icon).

### 3.2. Automated Tests
-   **Test:** `test_icon_presence`
    -   **Goal:** Verify that the build process correctly stages the icon file.
    -   **Implementation:** Add a test case in a new or existing test file (e.g., `tests/test_resources.cpp`) that checks for the existence of `lmuffb.ico` in the current working directory (or expected build output directory).

## 4. Documentation
-   **CHANGELOG.md:** Add an entry under the "Added" section for the new application icon.
-   **VERSION:** Increment the patch version (optional, depending on release cycle).
