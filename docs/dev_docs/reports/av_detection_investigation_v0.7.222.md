# Investigation Report: Windows Defender Detection (v0.7.222)

**Report ID:** `investigation_av_detection_07222`  
**Detection Name:** `Program:Win32/Wacapew.A!ml`  
**Status:** Completed  
**Version:** 0.7.222

---

## 1. Executive Summary
Windows Defender has flagged `lmuffb.exe` (v0.7.222) as a "Potentially Unwanted Program" using its **Machine Learning (ML)** heuristic engine. The detection is **behavioral**, meaning the application's runtime sequence and API footprint closely match patterns observed in malicious software, specifically ransomware (due to file-system activity) and info-stealers (due to low-level input/network combinations).

---

## 2. Potential AV Triggers

### 2.1. Behavioral Trigger: Automatic Configuration Migration (v0.7.218/219)
The newly implemented "Phase 2" migration logic is the strongest behavioral trigger. 
- **Action**: App detects `config.ini`, renames it to `config.ini.bak`, then writes multiple new files (`config.toml` and `user_presets/*.toml`).
- **Signature**: Bulk renaming followed by bulk file creation with a different extension in the application's own directory by an **unsigned binary** perfectly mimics the startup behavioral pattern of modern ransomware.

### 2.2. Signature Trigger: "Dirty" Win32 API Footprint
The binary imports a specific set of APIs that, when combined in an unsigned utility, trigger high-alert ML heuristics:
- **`dinput8.dll`**: Taking exclusive control of hardware input (`DirectInput`).
- **`wininet.dll`**: Opening unauthenticated HTTP connections to `localhost`.
- **`<psapi.h>`**: Including the Process Status API (used in `DirectInputFFB.cpp` but currently **unused**). Malware uses this for process injection/discovery.
- **Combined Impact**: To a scanner, this looks like a tool that captures hardware input and sends it to a local service or proxy.

### 2.3. Signature Trigger: Entropic Bloat
- **`toml++` (1.2MB header)**: The massive template-heavy header of `toml++` creates complex machine code patterns and increases the entropy of the binary. This can make the executable appear "packed" or "obfuscated" to simple scanners.
- **Embedded TOML Strings**: Generating large data blocks inside the binary (from `GeneratedBuiltinPresets.h`) further contributes to its non-standard signature.

---

## 3. Remediation Plan

### 3.1. Immediate Code Sanitization
1. **Remove unused `<psapi.h>`**: Clean up the binary's import table by removing the unused inclusion of `psapi.dll`.
2. **De-escalate File Activity**: Remove the automatic `std::filesystem::rename` logic. Instead:
   - Read from `config.ini` if modern TOML is missing.
   - Save the new `config.toml`, but **leave the old file untouched**.
   - Provide a "Cleanup Legacy Files" button in the GUI if the user wants to remove `.ini` files manually.
3. **WinINet Hardening**: Ensure network handles are transient and strictly gated by user permission (`m_rest_api_enabled`).

### 3.2. Long-Term Reputation Alignment
1. **Binary Signing**: The definitive solution for Windows 11. Even a self-signed certificate can help, but a standard Certificate Authority (CA) signature is required for 100% reputation stability.
2. **False Positive Submission**: Every release version (zip and exe) should be submitted to the [Microsoft Security Intelligence Portal](https://www.microsoft.com/en-us/wdsi/filesubmission) to "whitelist" the specific SHA256 of the build.

---

## 4. Technical Audit of Changes (v0.7.187 -> v0.7.222)

| Component | Change Type | AV Impact Note |
| :--- | :--- | :--- |
| **toml++ Library** | Added | Massive template/symbol bloat. |
| **Preset System** | Refactored | Introduced multi-file write activity. |
| **Migration Logic** | Added | Introduced rename/delete/write pattern. |
| **REST API** | Modified | Reduced network activity (good), but still uses WinINet (neutral). |
| **Time-Aware DSP** | Added | Complex math code (neutral). |

---

## 5. Decision Log
- **Conclusion**: The detection is a false positive based on heuristic suspicion, not an actual infection.
- **Priority**: High. Failure to address this will lead to automatic deletion of the software for all Windows 11 users by default.
- **Next Step**: Apply code sanitization to remove `psapi.h` and "soften" the migration logic.
