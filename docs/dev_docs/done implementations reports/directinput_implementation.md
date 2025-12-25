# DirectInput FFB Implementation Guide

This document outlines the technical steps required to transition LMUFFB from a vJoy-based "Axis Mapping" architecture to a native **DirectInput Force Feedback** architecture. This change allows the application to send specific "Constant Force" packets directly to the steering wheel driver, bypassing the need for a virtual joystick and allowing the application to coexist seamlessly with the game's input system.

## 1. Overview

**Priority: CRITICAL / REQUIRED**

Currently, LMUFFB acts as a virtual joystick (`vJoy`) and maps the calculated force to the **Axis Position**. This visualizes the force but does **not** drive the physical motors of a user's steering wheel.
To function as a true Force Feedback application (like iRFFB or Marvin's AIRA), LMUFFB **must** implement a DirectInput client that opens the physical wheel and sends `Constant Force` packets.

*Hypothetical Feature Note: Implementing DirectInput correctly is complex due to device enumeration, exclusive locking (cooperative levels), and handling lost devices. It effectively turns the app into a specialized driver client.*

## 2. Technical Requirements

*   **API**: DirectInput8 (via `dinput8.lib` / `dinput8.dll`).
*   **Language**: C++ (Native COM interfaces).
*   **Privileges**: Exclusive access to the FFB device is often required (`DISCL_EXCLUSIVE | DISCL_BACKGROUND`).

## 3. Implementation Steps

### Phase 1: Device Enumeration & Initialization
Instead of connecting to vJoy ID 1, we must scan connected hardware.

```cpp
IDirectInput8* g_pDI = NULL;
IDirectInputDevice8* g_pDevice = NULL;

// 1. Create DirectInput Object
DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_pDI, NULL);

// 2. Enumerate Devices (Filter for Wheels/FFB)
g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);

// 3. Callback Logic
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) {
    // Check if device supports FFB
    // Create Device
    g_pDI->CreateDevice(pdidInstance->guidInstance, &g_pDevice, NULL);
    return DIENUM_STOP; // Stop after finding first FFB wheel
}
```

### Phase 2: Setting Cooperative Level
This is critical. FFB usually requires Exclusive/Background access so forces continue when the app is minimized (running alongside the game).

```cpp
g_pDevice->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
g_pDevice->SetDataFormat(&c_dfDIJoystick);
g_pDevice->Acquire();
```

### Phase 3: Creating the Effect
We need a **Constant Force** effect.

```cpp
DIEFFECT diEffect;
DICONSTANTFORCE diConstantForce;
LPDIRECTINPUTEFFECT g_pEffect = NULL;

// Initialize parameters
diConstantForce.lMagnitude = 0;

diEffect.dwSize = sizeof(DIEFFECT);
diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
diEffect.dwDuration = INFINITE;
diEffect.dwGain = DI_FFNOMINALMAX;
diEffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
diEffect.lpvTypeSpecificParams = &diConstantForce;
// ... (Set Axes/Directions) ...

// Create
g_pDevice->CreateEffect(GUID_ConstantForce, &diEffect, &g_pEffect, NULL);
g_pEffect->Start(1, 0);
```

### Phase 4: Updating the Force (The Loop)
Inside the `FFBThread` (400Hz loop), instead of calling `SetAxis` (vJoy), we update the effect.

```cpp
void UpdateDirectInputForce(double normalizedForce) {
    if (!g_pEffect) return;

    // Map -1.0..1.0 to -10000..10000
    LONG magnitude = (LONG)(normalizedForce * 10000.0);
    
    // Clamp
    if (magnitude > 10000) magnitude = 10000;
    if (magnitude < -10000) magnitude = -10000;

    DICONSTANTFORCE cf;
    cf.lMagnitude = magnitude;

    DIEFFECT eff;
    ZeroMemory(&eff, sizeof(eff));
    eff.dwSize = sizeof(DIEFFECT);
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    // Send to driver (Low latency call)
    g_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_START);
}
```

## 4. Challenges & Solutions

1.  **Device Selection**: Users may have multiple controllers (Handbrake, Shifter, Wheel). The GUI must allow selecting the specific FFB device from a list.
2.  **Spring/Damper Effects**: Some wheels default to a heavy centering spring. The app should explicitly create specific Spring/Damper effects with 0 magnitude to "clear" the driver's default behavior.
3.  **Loss of Focus**: Even with `DISCL_BACKGROUND`, some games (or drivers) steal exclusive access. The app must handle `DIERR_NOTACQUIRED` errors and attempt to `Acquire()` periodically.

## 5. Benefits
*   **Latency**: Bypasses the vJoy -> Driver bridge.
*   **Usability**: User does not need to configure vJoy. They just select their wheel in LMUFFB.
*   **Compatibility**: Works with games that don't support multiple controllers well (though LMU is generally good with this).
