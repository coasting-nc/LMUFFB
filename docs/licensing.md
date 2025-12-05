# Licensing Analysis for LMUFFB

This document analyzes the licensing requirements for distributing LMUFFB, specifically concerning its dependencies: Dear ImGui, vJoy, and the rFactor 2 Shared Memory Plugin interface.

## 1. Components

### Dear ImGui
*   **License**: MIT License.
*   **Terms**: The MIT License allows for the use, copying, modification, merger, publication, distribution, sublicense, and/or sale of copies of the Software.
*   **Restriction**: The substantial portion of the Software (the license text) must be included in all copies or substantial portions of the Software.
*   **Implication**: You **can** distribute a compiled version of LMUFFB linked with Dear ImGui. You must include the Dear ImGui license text in your distribution (e.g., in an `About` box or a `LICENSE-IMGUI.txt` file).

### vJoy SDK
*   **License**: MIT License (Source: [shauleiz/vJoy on GitHub](https://github.com/shauleiz/vJoy)).
*   **Terms**: Standard MIT permissions.
*   **Implication**: You can distribute the application linked against `vJoyInterface.lib` / `vJoyInterface.dll` provided you respect the MIT attribution.

### rFactor 2 Shared Memory Map Plugin
*   **Component**: We rely on the header definitions (`struct` layout) and the concept of the plugin.
*   **License**: MIT License (Source: [TheIronWolfModding/rF2SharedMemoryMapPlugin](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin)).
*   **Implication**: Using the struct definitions to interoperate with the plugin is permitted.

## 2. Conclusion & Selection

The project has selected the **GNU General Public License v3.0 (GPL-3.0)** for the LMUFFB source code.

This choice ensures that:
1.  The project remains free software.
2.  Any improvements or modifications (forks) must also be released under the GPL-3.0 (Copyleft).

### Compatibility with Dependencies (MIT)
This combination is **fully compatible**.
*   **The Scenario**: A GPL-3.0 application (LMUFFB) linking against MIT-licensed libraries (Dear ImGui, vJoy).
*   **Legal Mechanic**: The MIT license is permissive and GPL-compatible. It grants the right to sublicense the library code. When compiled together, the resulting binary is distributed under the terms of the GPL-3.0.
*   **Redistribution**: You can legally distribute the `LMUFFB.exe` binary.

## 3. Redistribution Requirements

When you release the binary (`LMUFFB.exe`) or the installer, you must adhere to the following:

1.  **GPL Obligations**:
    *   You must provide the source code of LMUFFB (or a written offer to provide it) to anyone who receives the binary. Hosting this GitHub repository fulfills this.
    *   The binary itself is covered by the GPL-3.0.

2.  **MIT Obligations (Attribution)**:
    *   You must preserve the copyright notices of the MIT components.
    *   **Action**: Include a file named `THIRD-PARTY-NOTICES.txt` in the distribution containing the MIT license texts for **Dear ImGui** and **vJoy**.

### Summary for End Users
*   **LMUFFB**: Free software (GPL-3.0). You have the right to modify and share it.
*   **Included Libraries**: Free software (MIT). They are used by LMUFFB to provide GUI and Joystick functionality.
