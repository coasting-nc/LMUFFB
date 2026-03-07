# Implementation Plan - Issue #257: Fix exported log file names

Fix exported log file names to use car class and car brand instead of the livery name (vehicle name).

## Issue Reference
Issue #257: Fix exported log file names: use car class and car brand instead of livery name

## Design Rationale
The current log naming convention uses the vehicle name, which often corresponds to a specific livery. This makes it difficult to group logs by car model or class. Switching to `<Brand>_<Class>` provides a more structured and searchable naming scheme while keeping the track and timestamp for uniqueness.

## Reference Documents
- `src/AsyncLogger.h`
- `src/VehicleUtils.cpp`
- `src/main.cpp`
- `src/GuiLayer_Common.cpp`

## Codebase Analysis Summary
- `AsyncLogger::Start` (in `src/AsyncLogger.h`) constructs the filename.
- `SessionInfo` struct already contains `vehicle_brand` and `vehicle_class` (added in v0.7.132).
- `main.cpp` populates `vehicle_brand` and `vehicle_class` for auto-logging.
- `GuiLayer_Common.cpp` only populates `vehicle_name` for manual logging.

## Proposed Changes

### `src/AsyncLogger.h`
- Modify `AsyncLogger::Start` to use `info.vehicle_brand` and `info.vehicle_class` in the filename construction.
- The new format: `lmuffb_log_<timestamp>_<brand>_<class>_<track>.bin`.
- Ensure `SanitizeFilename` handles the new components correctly.

### `src/GuiLayer_Common.cpp`
- Update the "START LOGGING" button handler to populate `info.vehicle_brand` and `info.vehicle_class`.
- Use `ParseVehicleClass` and `ParseVehicleBrand` from `VehicleUtils.h`.

### `src/main.cpp`
- Review the auto-logging block to ensure it's consistent with the new naming scheme (it already seems to populate the fields).

### Version and Changelog
- Increment version to `0.7.133` in `VERSION`.
- Update `CHANGELOG_DEV.md`.

## Test Plan
1. Create `tests/test_issue_257_filenames.cpp`.
2. Define a `SessionInfo` with known brand, class, and track.
3. Call `AsyncLogger::Get().Start(info)`.
4. Verify `AsyncLogger::Get().GetFilename()` matches the expected pattern: `lmuffb_log_*_<Brand>_<Class>_<Track>.bin`.
5. Call `AsyncLogger::Get().Stop()`.

## Deliverables
- [x] Modified `src/AsyncLogger.h`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] New `tests/test_issue_257_filenames.cpp`
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`
- [x] Implementation Notes updated in this plan.

## Implementation Notes

### Iterative Loop - Round 1
- **Actions taken:**
    - Modified `src/AsyncLogger.h` to change the filename construction logic.
    - Modified `src/GuiLayer_Common.cpp` to populate `vehicle_brand` and `vehicle_class` when manual logging is started.
    - Created `tests/test_issue_257_filenames.cpp` to verify the new filename format.
    - Updated `tests/CMakeLists.txt` to include the new test.
- **Issues encountered:**
    - Initial build failed because `m_current_class_name` was private in `FFBEngine`. I moved it to the public section to allow GUI access.
    - Code review (Iteration 1) identified a missing `#include "VehicleUtils.h"` in `src/GuiLayer_Common.cpp`, which would cause a build failure.
    - Code review also noted missing mandatory deliverables (`VERSION`, `CHANGELOG_DEV.md`).
- **Resolutions:**
    - Added `#include "VehicleUtils.h"` to `src/GuiLayer_Common.cpp`.
    - Updated `VERSION` to `0.7.133`.
    - Updated `CHANGELOG_DEV.md` with details of the fix and the new version.
    - Verified the build succeeds in headless mode on Linux.
    - Verified all tests pass, including the new regression test.

### Final Verification
- All 414 test cases pass, including the new `test_issue_257_log_filename_format`.
- The generated log filename correctly follows the pattern: `lmuffb_log_<timestamp>_<brand>_<class>_<track>.bin`.
