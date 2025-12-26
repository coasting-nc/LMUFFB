# Investigation: vJoy Bundling and Licensing

**Date:** 2025-05-23
**Status:** Recommendations Ready

## 1. Technical Feasibility of Bundling `vJoyInterface.dll`

### Can we bundle it?
**Yes.**
`vJoyInterface.dll` is a standard Windows DLL. It serves as a bridge between the application and the vJoy Driver (kernel mode).
*   **Location:** If placed in the same directory as `LMUFFB.exe`, `LoadLibrary` (or static linking) will find it automatically.
*   **Architecture:** We must bundle the **x64 (amd64)** version of the DLL since LMUFFB is built as a 64-bit application.
*   **Dependency:** The DLL *requires* the vJoy Driver to be installed on the system to function. It cannot install the driver itself. If the driver is missing, `vJoyInterface.dll` might load, but initialization functions (like `AcquireVJD`) will fail or return "Driver Not Enabled".

### Conclusion
We **can and should** bundle `vJoyInterface.dll` with our release zip/installer. This eliminates the "DLL Not Found" error users face even if they have vJoy installed (sometimes the DLL isn't in PATH).

## 2. Licensing and Copyright

### vJoy License
vJoy (by Shaul Eizikovich) is open source.
*   **Source:** [GitHub - shauleiz/vJoy](https://github.com/shauleiz/vJoy)
*   **License:** **MIT License** (and some parts Public Domain / zlib in older versions, but generally permissive).
*   **SDK License:** The SDK (which includes the DLL) is intended for developers to distribute with their apps.

### Attribution Requirements
The MIT License requires including the copyright notice and permission notice in the software distribution.

### Recommendation
1.  **Bundle the DLL:** Include `vJoyInterface.dll` in the distribution folder.
2.  **Include License:** Add a file `licenses/vJoy_LICENSE.txt` containing the vJoy MIT license text.
3.  **Update Main License:** Mention in our `LICENSE` or `README` that the package contains third-party software (vJoy, Dear ImGui) and point to their licenses.

## 3. Version Checking Strategy

Since the DLL talks to the Driver, version mismatches can cause bugs.
*   **Target:** vJoy 2.1.9.1 (current standard).
*   **Check:** On startup, call `GetvJoyVersion()` (from the DLL) and `GetvJoyManufacturerString()`.
*   **Mismatch:** If the driver version differs significantly (e.g., < 2.1.8), warn the user. 2.1.8 and 2.1.9 are mostly compatible.

## Action Plan
1.  **Distribute:** Add `vJoyInterface.dll` to the repo (in `vendor/vJoy/lib/amd64`) or build script copy step.
2.  **Code:** Implement version check popup.
3.  **Docs:** Add vJoy License text.
