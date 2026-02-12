# Security Audit & False Positive Analysis (v0.7.27)

## Overview
This report analyzes the `lmuFFB` codebase to identify features and patterns that may trigger antivirus heuristics or behavioral monitoring warnings.

**Update (v0.7.27)**: A user reported that Windows Defender flagged the v0.7.25/.26 release as `Trojan:Script/Wacatac.C!ml`. This is a machine-learning based heuristic flag often triggered by unsigned binaries performing "Process Access" or memory inspection. Version 0.7.27 addresses this by removing `OpenProcess` calls.

## Findings

### 1. Missing Executable Metadata (High Probability Trigger)
The previous `src/res.rc` file contained only an icon definition. It lacked the standard `VERSIONINFO` resource block.
*   **Impact**: Antivirus heuristics often flag binaries without version information, company name, or product description as "generic" or "suspicious" (e.g., specific trojans often lack this).
*   **Behavior**: The file appears "anonymous" to the OS and security software.
*   **Status**: Fixed in v0.7.27 (Added `VS_VERSION_INFO`).

### 2. System Information Discovery (Medium Probability Trigger)
Review of behavioral logs indicates a detection for "System Information Discovery".
*   **Detection**: "Attempts to query display device information, possibly to determine if the process is running in a virtualized environment."
    *   **Match**: Process `LMUFFB.exe`
*   **Source**: The application initializes a DirectX 11 device for the ImGui overlay (`GuiLayer_Win32.cpp`).
    ```cpp
    // Calls D3D11CreateDeviceAndSwapChain, which queries GPU capabilities
    D3D11CreateDeviceAndSwapChain(..., &sd, &g_pSwapChain, &g_pd3dDevice, ...);
    ```
    Additionally, standard `SystemParametersInfo` calls are made to align the window.
*   **Analysis**: Malware often queries display properties to detect if it's running in a Sandbox (which often has generic/small displays). `lmuFFB` does this legitimate behavior to render its UI.
*   **Risk**: Medium. This is a common behavior for any graphical application, but when combined with other "suspicious" traits (like missing metadata or process handles), it raises the threat score.

### 3. Process Handle Usage (`GameConnector.cpp`)
The application uses `OpenProcess` to verify if the game is running.
```cpp
m_hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
```
*   **Analysis**: The usage of `PROCESS_QUERY_LIMITED_INFORMATION` is good practice (least privilege). However, the act of opening a handle to another process is a core behavior of game hacks and injection tools.
*   **Risk**: Moderate. Some sensitive heuristics might flag this as "Process Access" or "Memory Inspection".
*   **Status**: Fixed in v0.7.27. Replaced `OpenProcess` with `IsWindow` which uses safe window handle validation instead of opening a process handle.

### 4. DirectInput Exclusive Mode (`DirectInputFFB.cpp`)
The application requests `DISCL_EXCLUSIVE` access to the input device.
```cpp
hr = ((IDirectInputDevice8*)m_pDevice)->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
```
*   **Analysis**: This is necessary for high-fidelity FFB. However, exclusive input access can be interpreted by behavioral analysis as potential input interception (keylogging/macro behavior), especially if combined with background processing.
*   **Status**: This is a known false-positive vector for input tools but is functionally required.

## Recommendations

### Short Term (Code Changes)

1.  **Implement `VERSIONINFO` in Resource File**:
    *   **Status**: Implemented. `src/res.rc` now includes a full `VS_VERSION_INFO` block with CompanyName ("lmuFFB"), ProductVersion ("0.7.27.0"), etc.

2.  **Verify Build Security Flags**:
    *   **Status**: Implemented. `CMakeLists.txt` updated to enforce `/GS` (Buffer Security Check), `/DYNAMICBASE` (ASLR), and `/NXCOMPAT` (DEP) for MSVC builds.

### Long Term

1.  **Code Signing**:
    *   Acquire a code signing certificate (e.g., EV or Standard) to sign the executable. This whitelist the app with Microsoft and many AV vendors instantly.

2.  **False Positive Submission**:
    *   Submit the clean file to vendors (Microsoft, Symantec, Kaspersky) via their "False Positive" web forms.

## VirusTotal Report Export Guide

To generate a detailed offline report from VirusTotal for future auditing:

1.  **Install vt-cli**: Download from https://github.com/VirusTotal/vt-cli/releases
2.  **Initialize**: Run `vt init` and paste your API Key.
3.  **Export Report**: Use the following command to fetch the behavioral analysis in JSON format:
    ```powershell
    vt file behaviors 9652deae0ca058c637a5890c198d7bec542ed9dbd94ea621845a6c209896d964 --format json > vt_behavior_report.json
    ```

## VirusTotal Behavioral Report Summary (Template)

*Note: Populate this section with data from the exported JSON report.*

**File Hash**: `9652deae0ca058c637a5890c198d7bec542ed9dbd94ea621845a6c209896d964`
**Analysis Date**: 2026-02-11

### MITRE ATT&CK Matrix Matches

| Tactic | Technique | ID | Description |
| :--- | :--- | :--- | :--- |
| **Discovery** | System Information Discovery | T1082 | Querying display/GPU info (GuiLayer_Win32.cpp) |
| **Execution** | Shared Modules | T1129 | Loading standard DLLs (d3d11.dll, dinput8.dll) |
| **Defense Evasion** | Invalid Code Signature | T1036 | File is unsigned (High Risk Factor) |

### Observable Behavior

*   **Registry Keys**:
    *   *List keys opened/created here*
*   **File System**:
    *   *List files dropped/modified here*
*   **Network**:
    *   *List DNS resolutions or IP traffic here*
*   **Processes**:
    *   `LMUFFB.exe` (Self)
    *   `Le Mans Ultimate.exe` (Target - via AppID lookup or Shared Memory)
