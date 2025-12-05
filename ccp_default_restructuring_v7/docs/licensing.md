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

## 2. Conclusion

All major dependencies (Dear ImGui, vJoy, rF2 Shared Memory Plugin) utilize the **MIT License**.

This means LMUFFB can be:
1.  **Open Sourced** under the MIT License (recommended for consistency).
2.  **Open Sourced** under GPL-3.0 (if you want to enforce that forks remain open).
3.  **Closed Sourced/Proprietary**: The MIT license permits this, provided you include the license notices for the dependencies.

## 3. Recommended Action

We recommend releasing LMUFFB under the **MIT License**.

### Why MIT?
*   It aligns with the ecosystem (all dependencies are MIT).
*   It allows maximum freedom for the community to mod, fork, or integrate the code into other tools.
*   It minimizes legal complexity.

### Licensing Your Distribution
When you release the binary (`LMUFFB.exe`):
1.  Include a file named `LICENSE` containing the MIT text for LMUFFB.
2.  Include a file named `THIRD-PARTY-NOTICES.txt` containing the licenses for:
    *   Dear ImGui (MIT)
    *   vJoy (MIT)
