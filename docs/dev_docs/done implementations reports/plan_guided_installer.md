# Plan: Fully Guided Installer

**Date:** 2025-05-23
**Status:** Proposal

## Objective
Replace the raw executable distribution with a "Wizard" that handles prerequisites and configuration, reducing user error ("Double FFB", "vJoy missing").

## Installer Steps (Inno Setup / Custom Launcher)

### Phase 1: Prerequisite Check (Pre-Install)
1.  **Detect vJoy:** Check Registry or `C:\Program Files\vJoy`.
    *   *If missing:* Prompt to download/install (button to launch vJoy installer).
    *   *If present:* Check version (2.1.9.1 recommended).
2.  **Detect Shared Memory Plugin:** Check `[GamePath]/Plugins/rFactor2SharedMemoryMapPlugin64.dll`.
    *   Try to autodetect LMU game path (eg. from registry or common game paths). If it fails, offer to browse for Game Path. In any case, display the detected path in the installer, so that the user can confirm it.
    *   *If missing:* Offer to copy the DLL (bundled with installer) to the game folder. (User must browse for Game Path).
3.  **Visual C++ Redistributable:** Check and install if missing.

### Phase 2: Configuration Wizard (First Run)
When `LMUFFB.exe` is run for the first time (no config.ini):

**Step 1: vJoy Configuration**
*   Launch `vJoyConf.exe` (if possible) or show specific instructions: "Please enable 1 Device with X Axis."
*   Verify vJoy is active (try to acquire it).

**Step 2: Game Binding Helper**
*   *Innovative Idea:* Use a "vJoy Feeder" loop.
    *   "Open LMU Controls Menu. Click 'Detect' on Steering Axis."
    *   User clicks "Wiggle vJoy" button in Wizard.
    *   Wizard oscillates vJoy Axis X. Game detects it.
    *   User binds it.
    *   Instruction: "Now set In-Game FFB to 'None' or 'Smoothing 0'." [TODO: update all references to  In-Game FFB to 'None' or 'Smoothing 0' to reflect what LMU actually allows; replace "Game" with "LMU" in all references]

**Step 3: Wheel Selection**
*   Dropdown list of devices.
*   "Rotate your wheel". Bar moves.
*   "Select this device".

### Phase 3: Launch
*   Tell the user everything should be ready and LMU can be launched.

## Technical Requirements
1.  **Bundled Resources:**
    *   `vJoyInterface.dll` (x64/x86)
    *   `rFactor2SharedMemoryMapPlugin64.dll`
2.  **Logic:**
    *   Registry reading (Game path, vJoy path).
    *   Feeder logic (already in `DynamicVJoy.h`).

## Roadmap
*   **v0.4.0:** Implement Phase 1 (Installer checks).
*   **v0.5.0:** Implement Phase 2 (First Run Wizard).
