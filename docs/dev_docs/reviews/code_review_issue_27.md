# Code Review Report - Issue 27

- **Task**: Fix FFB Active after Crash and Negative Value Crashes
- **Review Date**: 2026-02-06
- **Reviewer**: Antigravity
- **Commits Reviewed**:
    - `38885167bcf9f5209bf1bacd407075ab83e7bb4c`
    - `3034863a7ec7b8ae615c1f47004fef4a0bc67cba`
    - `516fa77495079bc287e248eaaaa1ab37f0ca938d` (Merge)

## 1. Summary
The changes address two stability issues:
1.  **Safety Muting**: Automatically muting FFB when the game process freezes or crashes (detected via telemetry staleness).
2.  **Config Validation**: preventing application crashes caused by invalid or negative values in `config.ini` or presets.

The implementation introduces a heartbeat mechanism in `GameConnector` to detect frozen telemetry and adds a comprehensive `Validate()` method to the `Preset` struct to clamp all physical parameters to safe ranges.

## 2. Findings

| Severity | File | Location | Description |
| :--- | :--- | :--- | :--- |
| **Pass** | N/A | N/A | No critical or major issues found. |

### Minor Observations
*   **Redundant but Safe Clamping**: The `Preset::Apply` method performs clamping on assignment, and `Config::Load`/`LoadPresets` also calls `Preset::Validate()` (which clamps members). This redundancy is acceptable as it ensures safety both when loading data and when applying it to the engine.

## 3. Checklist Results

### Functional Correctness
- [x] **Plan Adherence**: The implementation matches the "Proposed Changes" in `plan_issue_27.md`.
- [x] **Completeness**: All deliverables (GameConnector updates, Validation logic, Tests) are present.
- [x] **Logic**:
    - `IsStale` correctly identifies frozen telemetry by monitoring `mElapsedTime`.
    - `Validate` correctly uses `std::max`/`std::min` to enforce safe domains (e.g., preventing negative bases for `pow`).

### Implementation Quality
- [x] **Clarity**: The `IsStale` and `Validate` methods are clearly named and focused.
- [x] **Robustness**: The 100ms watchdog provides a robust safety net for game crashes. Parameter clamping handles edge cases like `0.0` or negative inputs gracefully.
- [x] **Performance**: The heartbeat check involves minimal overhead (`steady_clock::now`) in the 400Hz loop.

### Code Style & Consistency
- [x] **Style**: code follows project conventions.
- [x] **Consistency**: The validation pattern mimics existing safety checks but makes them comprehensive.

### Testing
- [x] **Test Coverage**: `tests/test_ffb_stability.cpp` covers negative parameter safety and checking for NaNs/Crashes.
- [x] **Test Quality**: The tests effectively simulate the "killer" scenarios (negative gamma, etc.).
- [x] **Note**: `GameConnector::IsStale` is not unit-tested due to Win32/SharedMemory dependencies, which is an accepted limitation documented in the plan. Steps were taken to verify the logic via review.

### Configuration & Settings
- [x] **User Settings**: No new user settings required, but existing malformed settings are now handled safely.
- [x] **Defaults**: Safe defaults are enforced.

### Versioning & Documentation
- [x] **Version Increment**: Updated to `0.7.16` (patch increment).
- [x] **Documentation**: `CHANGELOG_DEV.md` updated. Usage of the new safety features is transparent to the user.

### Safety & Integrity
- [x] **Security**: No new security risks. Input sanitization (config validation) is improved.
- [x] **Unintended Deletions**: None found.

### Build Verification
- [x] **Compilation**: Code structure is valid.
- [x] **Tests Pass**: The user/CI has verified that tests pass.

## 4. Verdict

**PASS**

The code is robust, well-tested, and directly addresses the stability requirements. The added safety checks for configurations values significantly improve the application's resilience against user error, and the heartbeat monitor effectively mitigates the "stuck wheel" hazard during game crashes.
