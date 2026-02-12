# Implementation Plan - Remove Screenshot Feature

## Context
The goal is to remove the "Save Screenshot" feature from the LMUFFB application. This feature captures the application window and the console window (if visible) and saves them as a composite image.

**Reason for Removal:**
The user has reported that Windows Defender and VirusTotal scans flag the application as potentially malicious. The screen capture behavior (using `PrintWindow` and `BitBlt` on screen DCs) is a common heuristic for spyware, leading to false positives. Since the feature is no longer needed, removing it is a sound strategy to improve the application's reputation with antivirus software.

## Reference Documents
*   **User Request:** Issue #40 (GitHub) - False positives with Windows Defender.
*   **Related Files:** `src/GuiLayer_Win32.cpp`, `src/GuiLayer_Common.cpp`, `src/GuiLayer.h`, `tests/test_screenshot.cpp`.

## Codebase Analysis Summary

### Current Implementation
The screenshot functionality is implemented across the GUI layer with platform-specific backends:
1.  **UI Trigger (`src/GuiLayer_Common.cpp`)**: A button "Save Screenshot" in the debug/main window triggers the `SaveCompositeScreenshotPlatform` function.
2.  **Architecture (`src/GuiLayer.h`)**: Declares `SaveCompositeScreenshotPlatform` and `CaptureWindowToBuffer` (implicitly via usage in tests, though effectively private/helper).
3.  **Windows Implementation (`src/GuiLayer_Win32.cpp`)**:
    *   `CaptureWindowToBuffer`: Captures a specific HWND using `PrintWindow` or `BitBlt`.
    *   `SaveCompositeScreenshotPlatform`: Orchestrates the capture of the GUI and Console windows, composites them, and uses `stb_image_write` to save as PNG.
    *   Dependencies: Includes `stb_image_write.h`.
4.  **Linux Implementation (`src/GuiLayer_Linux.cpp`)**: Contains a stub for `SaveCompositeScreenshotPlatform`.
5.  **Tests (`tests/test_screenshot.cpp`)**: A dedicated test suite verifying the capture logic, image format, and multi-window handling.

### Impacted Functionality
*   **GUI**: The "Save Screenshot" button will be removed.
*   **Build System**: `stb_image_write.h` will no longer be compiled/linked (via `GuiLayer_Win32.cpp`).
*   **Testing**: The `screenshot` test suite will be removed.

## FFB Effect Impact Analysis
*   **None.** This change is purely related to the GUI and auxiliary tools. No FFB processing logic, physics calculations, or signal paths are touched.

## Proposed Changes

### 1. Remove UI Elements
*   **File:** `src/GuiLayer_Common.cpp`
*   **Action:** Remove the `ImGui::Button("Save Screenshot")` block and its associated logic (lines 211-222 in current version).

### 2. Remove Platform Implementation (Windows)
*   **File:** `src/GuiLayer_Win32.cpp`
*   **Action:**
    *   Remove `#include "stb_image_write.h"` and the implementation define `STB_IMAGE_WRITE_IMPLEMENTATION`.
    *   Remove `CaptureWindowToBuffer` function.
    *   Remove `SaveCompositeScreenshotPlatform` function.

### 3. Remove Platform Implementation (Linux)
*   **File:** `src/GuiLayer_Linux.cpp`
*   **Action:** Remove the `SaveCompositeScreenshotPlatform` stub function.

### 4. Update Header Interface
*   **File:** `src/GuiLayer.h`
*   **Action:** Remove the declaration of `SaveCompositeScreenshotPlatform`.

### 5. Remove Tests
*   **File:** `tests/test_screenshot.cpp`
*   **Action:** Delete the file.
*   **File:** `tests/CMakeLists.txt`
*   **Action:** Remove `test_screenshot.cpp` from the `TEST_SOURCES` list.

### 6. Versioning
*   **Action:** Increment the version number in `VERSION` and `src/Version.h` by +1 (patch level) to reflect the change, as per instructions.

## Test Plan
Since this is a functionality removal, standard TDD (write test, fail, pass) applies differently. The "test" is the successful build and absence of regressions.

### Verification Steps
1.  **Build Verification**:
    *   Run the full build command (`cmake --build build ...`).
    *   **Expectation**: The build must not fail due to missing symbols (e.g., `SaveCompositeScreenshotPlatform`).
2.  **Regression Testing**:
    *   Run `run_combined_tests.exe`.
    *   **Expectation**: All remaining tests (FFB, Persistence, Math) must pass. The `Screenshot` tests should no longer be listed in the output.
3.  **Visual Verification (Manual)**:
    *   Launch the app.
    *   **Expectation**: The "Save Screenshot" button is no longer present in the UI.

## Deliverables
*   [ ] Modified `src/GuiLayer_Common.cpp` (Button removed)
*   [ ] Modified `src/GuiLayer_Win32.cpp` (Implementation removed)
*   [ ] Modified `src/GuiLayer_Linux.cpp` (Stub removed)
*   [ ] Modified `src/GuiLayer.h` (Declaration removed)
*   [ ] Modified `tests/CMakeLists.txt` (Test source removed)
*   [ ] Deleted `tests/test_screenshot.cpp`
*   [ ] Updated `VERSION` and `src/Version.h`
*   [ ] Updated `USER_CHANGELOG.md` and `CHANGELOG_DEV.md`

## Implementation Notes
*   [ ] Check for any unforeseen build errors if `stb_image_write.h` was implicitly relied upon by other includes (unlikely).
*   [ ] Ensure `Enable ImGui` path is clean.

## Implementation Notes
No significant issues encountered. Implementation proceeded as planned.
