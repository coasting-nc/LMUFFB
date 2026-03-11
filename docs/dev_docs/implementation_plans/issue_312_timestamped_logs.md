# Implementation Plan - Issue #312: Don't overwrite debug logs

## Context
The current implementation of the `Logger` overwrites `lmuffb_debug.log` every time the application starts because it uses `std::ios::trunc` and a fixed filename. This causes the loss of diagnostic information from previous sessions, which is critical for debugging crashes or intermittent issues.

## Design Rationale
- **Reliability:** By using timestamped filenames, we ensure that every session's debug output is preserved. This is vital for "post-mortem" analysis of crashes.
- **Consistency:** `AsyncLogger` (which handles telemetry logs) already uses timestamped filenames and stores them in a configurable directory. Aligning the debug `Logger` with this behavior provides a unified experience for users and developers.
- **Safety:** Using `std::filesystem` for directory creation ensures that logs are stored in a valid location, even if the user hasn't manually created the `logs/` folder.

## Codebase Analysis Summary
- **Impacted Modules:**
    - `Logger.h`: The core logging class. It needs to handle timestamped filename generation and directory management.
    - `main.cpp`: The entry point where the logger is initialized. It needs to be updated to pass the correct path from `Config`.
- **Impact Zone Rationale:**
    - `Logger` is the single source of truth for synchronous debug logging.
    - `main.cpp` controls the application lifecycle and is responsible for early initialization of system services like logging.

## Proposed Changes

### `src/Logger.h`
- **Add `<filesystem>`, `<ctime>`, `<sstream>`, and `<iomanip>` includes.**
- **Update `Init(const std::string& filename)`:**
    - Change signature to `Init(const std::string& base_filename, const std::string& log_path = "", bool use_timestamp = true)`.
    - Implement a timestamp generator (e.g., `YYYY-MM-DD_HH-MM-SS`).
    - Construct the full path: `log_path / base_filename_TIMESTAMP.log`.
    - Use `std::filesystem::create_directories` to ensure the path exists.
    - Use `std::ios::app` for timestamped logs to prevent data loss if re-initialized in the same second.
    - Add a check to skip re-opening if the filename hasn't changed.
- **Update `_LogNoLock`**: Ensure timestamps inside the file remain consistent.

### `src/main.cpp`
- **Initial Initialization:** Early `Init` call using "logs" as default directory.
- **Post-Config Initialization:** After `Config::Load(g_engine)`, call `Logger::Get().Init` again using `Config::m_log_path`. The new `Logger` logic ensures that if the path is the same, it doesn't duplicate the "Initialized" header unnecessarily, and if it's different, it moves to the correct location.

### Versioning
- Increment `VERSION` from `0.7.166` to `0.7.167`.
- Update `CHANGELOG_DEV.md`.

## Test Plan
- **New Test File:** `tests/test_issue_312_logger.cpp`
- **Test Cases:**
    1. **Unique Filenames:** Call `Init` twice with a small delay and verify two distinct files are created.
    2. **Directory Creation:** Call `Init` with a non-existent nested path (e.g., `test_logs_nested/debug/`) and verify the directories are created.
    3. **Extension Handling:** Verify that `.txt` or no extension is handled correctly.
- **Existing Tests:** Run `./build/tests/run_combined_tests` to ensure no regressions. Updated several existing tests to use `use_timestamp=false` for stable verification.

## Deliverables
- [x] Modified `src/Logger.h`
- [x] Modified `src/main.cpp`
- [x] New `tests/test_issue_312_logger.cpp`
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`
- [x] Implementation Notes (final step)

## Implementation Notes
- **Unforeseen Issues:** Several existing tests relied on a fixed filename and `std::ios::trunc` behavior to verify log content. These tests failed initially with timestamped logs.
- **Plan Deviations:** Added an optional `use_timestamp` parameter to `Logger::Init` (defaulting to `true`) to allow these tests to continue functioning with fixed filenames without requiring a total rewrite of the test logic.
- **Challenges:** Handling the double initialization in `main.cpp` (before and after config load) without losing the early logs or creating two files in the same directory was solved by using `std::ios::app` and an identity check in `Init`.
- **Recommendations:** For future logging improvements, consider a log rotation/cleanup policy to prevent the `logs/` directory from growing indefinitely.
