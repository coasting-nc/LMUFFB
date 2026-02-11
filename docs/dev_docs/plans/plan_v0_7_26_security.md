# Implementation Plan - Security Hardening & False Positive Reduction (v0.7.26)

# Context
The objective of this task is to improve the security posture of the `lmuFFB` application and reduce the likelihood of false-positive antivirus detections (specifically `Trojan:Script/Wacatac.C!ml`). This involves adding missing executable metadata, enabling secure build flags, and refactoring code to avoid suspicious API calls.

# Reference Documents
*   `docs/dev_docs/security_audit_v0_7_25.md` (Initial Audit)
*   `docs/dev_docs/security_audit_v0_7_26.md` (Post-Fix Audit)
*   User Report: Windows Defender flagging `Trojan:Script/Wacatac.C!ml`.

# Codebase Analysis Summary
## Current Architecture Overview
*   **GameConnector**: A singleton class responsible for managing the connection to Le Mans Ultimate's shared memory. It previously used `OpenProcess` to verify the game's liveness.
*   **Build System**: CMake-based build system generating MSVC project files.
*   **Resources**: The application lacked a comprehensive resource file (`src/res.rc`), containing only an icon.

## Impacted Functionalities
1.  **Game Connection Logic (`GameConnector`)**: The mechanism for checking if the game is running is being altered from a process-handle based check to a window-handle based check.
2.  **Build Configuration**: The compiler and linker flags are being updated to enforce security standards (`/GS`, `/DYNAMICBASE`, `/NXCOMPAT`).
3.  **Application Metadata**: The binary will now include versioning and company information.

# FFB Effect Impact Analysis
*   **None**: This task is purely structural and security-focused. No FFB algorithms or logic are modified.

# Proposed Changes

## 1. Executable Metadata (`src/res.rc`)
*   **Goal**: Eliminate "Anonymous File" heuristic.
*   **Change**: Add `VS_VERSION_INFO` block.
    *   `FILEVERSION`: 0,7,26,0
    *   `PRODUCTVERSION`: 0,7,26,0
    *   `CompanyName`: "lmuFFB"
    *   `FileDescription`: "Le Mans Ultimate FFB Bridge"
    *   `InternalName`: "lmuFFB"
    *   `OriginalFilename`: "LMUFFB.exe"
    *   `ProductName`: "lmuFFB"

## 2. Build Hardening (`CMakeLists.txt`)
*   **Goal**: Enable exploit mitigations.
*   **Change**: Add MSVC specific flags:
    *   `add_compile_options(/GS)` - Buffer Security Check.
    *   `add_link_options(/DYNAMICBASE)` - ASLR.
    *   `add_link_options(/NXCOMPAT)` - DEP.

## 3. Heuristic Reduction (`src/GameConnector.h`, `src/GameConnector.cpp`)
*   **Goal**: Remove `OpenProcess` usage which triggers "Process Access" heuristics.
*   **Change**:
    *   Remove `m_hProcess` member.
    *   Add `m_hwndGame` (HWND) member.
    *   In `TryConnect()`: Store the window handle found in shared memory (`mAppWindow`).
    *   In `IsConnected()`: Replace `WaitForSingleObject(m_hProcess, 0)` with `IsWindow(m_hwndGame)`.
    *   Update `_DisconnectLocked()` to clear `m_hwndGame` instead of closing a process handle.

# Parameter Synchronization Checklist
*   N/A - No new user settings.

# Version Increment Rule
*   Increment version to `0.7.26`.

# Test Plan (TDD-Ready)

## Test 1: Executable Metadata Verification (`test_security_metadata`)
*   **Goal**: Verify that the built executable contains the expected version and company info.
*   **Input**: Path to current executable (retrieved via `GetModuleFileName`).
*   **Logic**:
    *   Use `GetFileVersionInfoSize` / `GetFileVersionInfo` / `VerQueryValue`.
    *   Query `\StringFileInfo\040904b0\CompanyName`.
*   **Assertion**: Value must equal "lmuFFB".

## Test 2: GameConnector Lifecycle with Window Handle
*   **Goal**: Verify that `GameConnector` correctly identifies disconnection when the window handle becomes invalid.
*   **Note**: Since we can't easily spawn a real game process, we will rely on the existing `test_game_connector_lifecycle` which mocks the shared memory. We will verify it passes with the new `IsWindow` logic (which will return false for NULL or invalid handles, serving our purpose of "safe failure" in tests).

# Deliverables
*   [x] Code changes (`src/res.rc`, `CMakeLists.txt`, `src/GameConnector.*`).
*   [x] New/Updated tests (`tests/test_security_fixes.cpp`, `tests/test_windows_platform.cpp`).
*   [x] Documentation updates (`CHANGELOG_DEV.md`, `docs/dev_docs/security_audit_v0_7_26.md`).
*   [ ] Implementation Notes (update this plan file after completion).
