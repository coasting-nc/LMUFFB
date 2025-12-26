# Plan: Guided Configurator (In-App)

**Date:** 2025-05-23
**Status:** Proposal

## Goal
Add a "Wizard" button inside the main LMUFFB GUI to help users fix configuration issues (Double FFB, wrong device) without restarting the app or reading external docs.

## Wizard Workflow

### Step 1: Device Check
*   **Prompt:** "Please rotate your physical steering wheel."
*   **Action:** App scans DirectInput devices for axis movement.
*   **Result:** Auto-selects the moving device in the "FFB Device" dropdown.
    *   *Check:* If the moving device is "vJoy", warn the user: "You seem to be rotating a virtual device? Please rotate your REAL wheel."

### Step 2: vJoy / Game Binding Check
*   **Prompt:** "We need to ensure the game is sending FFB to vJoy, not your wheel."
*   **Action:**
    1.  User is asked to wiggle vJoy Axis (via button "Test vJoy" which calls `SetAxis`).
    2.  User confirms if Game detects it.
    *   *Complex Part:* Checking if Game is WRONGLY bound to Real Wheel. We can't read Game Config directly easily.
    *   *Alternative:* Ask user: "Does your wheel resist turning when in the game menu?" (If yes, Game has FFB lock).

### Step 3: Input Feeder Warning
*   **Crucial Step:** Explain the "Steering Bridge".
*   **Text:** "If you bound Game Steering to vJoy, you must use a 'Feeder' app (like vJoy Feeder, Joystick Gremlin, or Mojo) to map your Real Wheel to vJoy. LMUFFB does **not** currently bridge steering input."
*   *Future:* If LMUFFB implements bridging (reading real axis, writing vJoy axis), enable it here.

## Implementation Priorities
1.  **Device Auto-Detect:** High priority. Reduces "I picked the wrong device" support tickets.
2.  **vJoy Test Button:** Easy to implement. Helps user bind game.
