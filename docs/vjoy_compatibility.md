# vJoy Version Compatibility

This document outlines the vJoy version requirements for LMUFFB. Compatibility issues between vJoy versions, Windows versions (10 vs 11), and Force Feedback apps are common.

## Recommendation

**We recommend using vJoy 2.1.9.1 (by jshafer817) for Windows 10 and 11.**

*   **Download**: [vJoy 2.1.9.1 Releases](https://github.com/jshafer817/vJoy/releases)
*   **Reasoning**: This version is signed for Windows 10/11 and includes fixes for Force Feedback (FFB) that were broken in some 2.1.8 releases on newer OS updates.

## Historical Context (iRFFB)

Users coming from iRFFB might be familiar with specific version requirements:
*   **Legacy (Windows 10 older builds)**: vJoy 2.1.8.39 (by shauleiz) was the standard.
*   **Modern (Windows 10 20H2+ / Windows 11)**: The original driver signature expired or was rejected by newer Windows security features. The fork by **jshafer817** (2.1.9.1) resolved this and is now the standard for sim racing tools on modern Windows.

## Compatibility Table

| OS Version | Recommended vJoy Version | Notes |
| :--- | :--- | :--- |
| **Windows 11** | **2.1.9.1** | Must use the jshafer817 fork. Original 2.1.9 may fail to install. |
| **Windows 10 (20H2+)** | **2.1.9.1** | Preferred for FFB stability. |
| **Windows 10 (Old)** | 2.1.8.39 | Legacy standard, acceptable if already installed. |

## Important Note on "Existing" Installations

If you already have vJoy installed for iRFFB:
1.  **Check Version**: Open `vJoyConf` or `Monitor vJoy` to see the version number.
2.  **Keep it**: If it works for iRFFB, it will work for LMUFFB. LMUFFB uses the standard `vJoyInterface.dll` API which is backward compatible.
3.  **Upgrade**: Only upgrade if you are experiencing "Failed to acquire device" errors or missing FFB. **Uninstall the old version completely before installing the new one.**
