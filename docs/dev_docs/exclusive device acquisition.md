# Question

Regarding the acquisition of the wheel device, currently the app seems to attempt the acquisition with a single request in OR, exclusive or not exclusive. I'd like to know if the device was acquired exclusively or not. This is mainly because sometimes the game FFB is enable, which we don't want if we are trying to test only the FFB signal we get from our app. How do you suggest to implement this? Should we attempt an exclusive acquisition first, and if this fails / raise an exception, try the non exclusive acquisition?
If we acquire the wheel device exclusively, will the game still be able to receive the driver inputs from the steering wheel?


# Answer

To address your requirements, we need to expose the **Acquisition Mode** (Exclusive vs. Shared) to the GUI. This allows you to confirm if LMUFFB has successfully "locked" the device (preventing the game from sending double FFB) or if it is running in shared mode.

### 1. Answers to your questions

**Q: How do you suggest to implement this? Should we attempt an exclusive acquisition first?**
**A: Yes.** The best practice for FFB apps is:
1.  **Try Exclusive Mode first.** If successful, you have priority. This often prevents the game from acquiring the device for FFB (solving the "Double FFB" fight automatically), while still allowing the game to read inputs.
2.  **Fallback to Non-Exclusive.** If Exclusive fails (e.g., the game already grabbed it), catch the error and retry in Non-Exclusive mode so the app still works, even if it has to share the device.

**Q: If we acquire the wheel device exclusively, will the game still be able to receive driver inputs?**
**A: Yes.**
In DirectInput, `DISCL_EXCLUSIVE` means "Only one application can have **Exclusive** access." It does **not** block other applications from getting **Non-Exclusive** access.
*   **LMUFFB (Exclusive):** Can read inputs and send FFB.
*   **Game (Non-Exclusive):** Can read inputs (Steering/Pedals work fine).
*   **Game (FFB):** If the game tries to acquire Exclusive access to send FFB, it will fail. This is actually **desirable** as it mutes the game's native FFB, leaving only LMUFFB's signal.

---

### 2. Implementation Plan

We will modify `DirectInputFFB` to track the acquisition state and update `GuiLayer` to display it.

#### Step 1: Update `src/DirectInputFFB.h`
Add a getter and a member variable to track the mode.

```cpp
// src/DirectInputFFB.h

class DirectInputFFB {
public:
    // ... existing methods ...

    // NEW: Check if we hold Exclusive lock
    bool IsExclusive() const { return m_isExclusive; }

private:
    // ... existing members ...
    
    bool m_isExclusive = false; // Track acquisition mode
};
```

#### Step 2: Update `src/DirectInputFFB.cpp`
Update `SelectDevice` to store the result of the cooperative level attempt.

```cpp
// src/DirectInputFFB.cpp

bool DirectInputFFB::SelectDevice(const GUID& guid) {
    // ... [Creation logic] ...

    // Reset state
    m_isExclusive = false;

    // Attempt 1: Exclusive/Background (Best for FFB)
    std::cout << "[DI] Attempting to set Cooperative Level (Exclusive | Background)..." << std::endl;
    HRESULT hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    
    if (SUCCEEDED(hr)) {
        m_isExclusive = true; // Success!
        std::cout << "[DI] Cooperative Level set to EXCLUSIVE." << std::endl;
    } else {
        // Fallback: Non-Exclusive
        std::cerr << "[DI] Exclusive mode failed (Error: " << std::hex << hr << std::dec << "). Retrying in Non-Exclusive mode..." << std::endl;
        hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        
        if (SUCCEEDED(hr)) {
            m_isExclusive = false; // Shared mode
            std::cout << "[DI] Cooperative Level set to NON-EXCLUSIVE." << std::endl;
        }
    }
    
    if (FAILED(hr)) {
        std::cerr << "[DI] Failed to set cooperative level." << std::endl;
        return false;
    }

    // ... [Acquire and CreateEffect logic] ...
}
```

#### Step 3: Update `src/GuiLayer.cpp`
Display the status in the Tuning Window.

```cpp
// src/GuiLayer.cpp inside DrawTuningWindow

    // ... [After Device Selection Combo] ...

    // NEW: Display Acquisition Mode
    if (DirectInputFFB::Get().IsActive()) {
        if (DirectInputFFB::Get().IsExclusive()) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Mode: EXCLUSIVE (Game FFB Blocked)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("LMUFFB has exclusive control.\nThe game can read steering but cannot send FFB.\nThis prevents 'Double FFB' issues.");
        } else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Mode: SHARED (Potential Conflict)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("LMUFFB is sharing the device.\nEnsure In-Game FFB is set to 'None' or 0% strength\nto avoid two force signals fighting each other.");
        }
    }

    // ... [Rest of UI] ...
```

### 3. Applying the Changes

I will now apply these changes to the codebase.

#### 1. Modify `src/DirectInputFFB.h`

```cpp
#ifndef DIRECTINPUTFFB_H
#define DIRECTINPUTFFB_H

#include <vector>
#include <string>
#include <atomic>

#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#else
// Mock types for non-Windows build/test
typedef void* HWND;
typedef void* LPDIRECTINPUT8;
typedef void* LPDIRECTINPUTDEVICE8;
typedef void* LPDIRECTINPUTEFFECT;
struct GUID { unsigned long Data1; unsigned short Data2; unsigned short Data3; unsigned char Data4[8]; };
#endif

struct DeviceInfo {
    GUID guid;
    std::string name;
};

class DirectInputFFB {
public:
    static DirectInputFFB& Get();

    bool Initialize(HWND hwnd);
    void Shutdown();

    // Returns a list of FFB-capable devices
    std::vector<DeviceInfo> EnumerateDevices();

    // Select and Acquire a device
    bool SelectDevice(const GUID& guid);
    
    // Release the currently acquired device (User unbind)
    void ReleaseDevice();

    // Update the Constant Force effect (-1.0 to 1.0)
    void UpdateForce(double normalizedForce);

    bool IsActive() const { return m_active; }
    std::string GetCurrentDeviceName() const { return m_deviceName; }
    
    // NEW: Check acquisition mode
    bool IsExclusive() const { return m_isExclusive; }

private:
    DirectInputFFB();
    ~DirectInputFFB();

    LPDIRECTINPUT8 m_pDI = nullptr;
    LPDIRECTINPUTDEVICE8 m_pDevice = nullptr;
    LPDIRECTINPUTEFFECT m_pEffect = nullptr;
    HWND m_hwnd = nullptr;
    
    bool m_active = false;
    bool m_isExclusive = false; // Track mode
    std::string m_deviceName = "None";
    
    // Internal helper to create the Constant Force effect
    bool CreateEffect();

    long m_last_force = -999999; 
};

#endif // DIRECTINPUTFFB_H
```

#### 2. Modify `src/DirectInputFFB.cpp`

```cpp
#include "DirectInputFFB.h"
#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <dinput.h>
#endif

// Keep existing implementations
DirectInputFFB& DirectInputFFB::Get() {
    static DirectInputFFB instance;
    return instance;
}

DirectInputFFB::DirectInputFFB() {}

DirectInputFFB::~DirectInputFFB() {
    Shutdown();
}

bool DirectInputFFB::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
#ifdef _WIN32
    if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_pDI, NULL))) {
        std::cerr << "[DI] Failed to create DirectInput8 interface." << std::endl;
        return false;
    }
    std::cout << "[DI] Initialized." << std::endl;
    return true;
#else
    std::cout << "[DI] Mock Initialized (Non-Windows)." << std::endl;
    return true;
#endif
}

void DirectInputFFB::Shutdown() {
    ReleaseDevice(); // Reuse logic
    if (m_pDI) {
        #ifdef _WIN32
        m_pDI->Release();
        m_pDI = nullptr;
        #endif
    }
}

#ifdef _WIN32
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) {
    auto* devices = (std::vector<DeviceInfo>*)pContext;
    DeviceInfo info;
    info.guid = pdidInstance->guidInstance;
    char name[260];
    WideCharToMultiByte(CP_ACP, 0, pdidInstance->tszProductName, -1, name, 260, NULL, NULL);
    info.name = std::string(name);
    devices->push_back(info);
    return DIENUM_CONTINUE;
}
#endif

std::vector<DeviceInfo> DirectInputFFB::EnumerateDevices() {
    std::vector<DeviceInfo> devices;
#ifdef _WIN32
    if (!m_pDI) return devices;
    m_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, &devices, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
#else
    DeviceInfo d1; d1.name = "Simucube 2 Pro (Mock)";
    DeviceInfo d2; d2.name = "Logitech G29 (Mock)";
    devices.push_back(d1);
    devices.push_back(d2);
#endif
    return devices;
}

void DirectInputFFB::ReleaseDevice() {
#ifdef _WIN32
    if (m_pEffect) {
        m_pEffect->Stop();
        m_pEffect->Unload();
        m_pEffect->Release();
        m_pEffect = nullptr;
    }
    if (m_pDevice) {
        m_pDevice->Unacquire();
        m_pDevice->Release();
        m_pDevice = nullptr;
    }
    m_active = false;
    m_isExclusive = false;
    m_deviceName = "None";
    std::cout << "[DI] Device released by user." << std::endl;
#else
    m_active = false;
    m_deviceName = "None";
#endif
}

bool DirectInputFFB::SelectDevice(const GUID& guid) {
#ifdef _WIN32
    if (!m_pDI) return false;

    // Cleanup old using new method
    ReleaseDevice();

    std::cout << "[DI] Attempting to create device..." << std::endl;
    if (FAILED(m_pDI->CreateDevice(guid, &m_pDevice, NULL))) {
        std::cerr << "[DI] Failed to create device." << std::endl;
        return false;
    }

    std::cout << "[DI] Setting Data Format..." << std::endl;
    if (FAILED(m_pDevice->SetDataFormat(&c_dfDIJoystick))) {
        std::cerr << "[DI] Failed to set data format." << std::endl;
        return false;
    }

    // Attempt 1: Exclusive/Background (Best for FFB)
    std::cout << "[DI] Attempting to set Cooperative Level (Exclusive | Background)..." << std::endl;
    HRESULT hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    
    std::string mode_str = "EXCLUSIVE | BACKGROUND"; 

    if (SUCCEEDED(hr)) {
        m_isExclusive = true;
    } else {
        // Fallback: Non-Exclusive
        std::cerr << "[DI] Exclusive mode failed (Error: " << std::hex << hr << std::dec << "). Retrying in Non-Exclusive mode..." << std::endl;
        hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        mode_str = "NON-EXCLUSIVE | BACKGROUND";
        m_isExclusive = false;
    }
    
    if (FAILED(hr)) {
        std::cerr << "[DI] Failed to set cooperative level (Non-Exclusive failed too)." << std::endl;
        return false;
    }

    std::cout << "[DI] Acquiring device..." << std::endl;
    if (FAILED(m_pDevice->Acquire())) {
        std::cerr << "[DI] Failed to acquire device." << std::endl;
        // Don't return false yet, might just need focus/retry
    } else {
        std::cout << "[DI] Device Acquired in " << mode_str << " mode." << std::endl;
    }

    // Create Effect
    if (CreateEffect()) {
       m_active = true;
        std::cout << "[DI] SUCCESS: Physical Device fully initialized and FFB Effect created." << std::endl;
 
        return true;
    }
    return false;
#else
    m_active = true;
    m_deviceName = "Mock Device Selected";
    return true;
#endif
}

bool DirectInputFFB::CreateEffect() {
#ifdef _WIN32
    if (!m_pDevice) return false;

    DWORD rgdwAxes[1] = { DIJOFS_X };
    LONG rglDirection[1] = { 0 };
    DICONSTANTFORCE cf;
    cf.lMagnitude = 0;

    DIEFFECT eff;
    ZeroMemory(&eff, sizeof(eff));
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwSamplePeriod = 0;
    eff.dwGain = DI_FFNOMINALMAX;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval = 0;
    eff.cAxes = 1;
    eff.rgdwAxes = rgdwAxes;
    eff.rglDirection = rglDirection;
    eff.lpEnvelope = NULL;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;
    eff.dwStartDelay = 0;

    if (FAILED(m_pDevice->CreateEffect(GUID_ConstantForce, &eff, &m_pEffect, NULL))) {
        std::cerr << "[DI] Failed to create Constant Force effect." << std::endl;
        return false;
    }
    
    // Start immediately
    m_pEffect->Start(1, 0);
    return true;
#endif
    return true;
}

void DirectInputFFB::UpdateForce(double normalizedForce) {
    if (!m_active) return;

    // Sanity Check: If 0.0, stop effect to prevent residual hum
    // Actually DirectInput 0 means center/off for Constant Force.
    if (std::abs(normalizedForce) < 0.00001) normalizedForce = 0.0;

    // Safety Check: Saturation
    if (std::abs(normalizedForce) > 0.99) {
        static int clip_log = 0;
        if (clip_log++ % 400 == 0) { // Log approx once per second at 400Hz
            std::cout << "[DI] WARNING: FFB Output Saturated (Clipping). Force: " << normalizedForce << ". Reduce Gain or increase Max Torque Ref." << std::endl;
        }
    }

    // Clamp
    normalizedForce = (std::max)(-1.0, (std::min)(1.0, normalizedForce));

    // Scale to -10000..10000
    long magnitude = static_cast<long>(normalizedForce * 10000.0);

    // Optimization: Don't call driver if value hasn't changed
    if (magnitude == m_last_force) return;
    m_last_force = magnitude;

#ifdef _WIN32
    if (m_pEffect) {
        DICONSTANTFORCE cf;
        cf.lMagnitude = magnitude;
        
        DIEFFECT eff;
        ZeroMemory(&eff, sizeof(eff));
        eff.dwSize = sizeof(DIEFFECT);
        // We use DIEP_TYPESPECIFICPARAMS because we are only updating the Magnitude (Specific to ConstantForce).
        // This is more efficient than updating the entire envelope or direction.
        eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        eff.lpvTypeSpecificParams = &cf;
        
        // Update parameters only (magnitude changes).
        // DO NOT pass DIEP_START here as it restarts the envelope and can cause clicks/latency.
        // The effect is started once in CreateEffect() and runs continuously.
        // If device is lost, the re-acquisition logic below will restart it properly.
        HRESULT hr = m_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
        
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
            // Try to re-acquire once
            HRESULT hrAcq = m_pDevice->Acquire();
            if (SUCCEEDED(hrAcq)) {
                // If we re-acquired, we might need to restart effect, or maybe just set params.
                // Safest to SetParams and assume continuous play, but CreateEffect handles Start(1,0).
                // If logic suggests effect stopped, we can explicitly start if needed, but avoid DIEP_START loop.
                m_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
            } else if (hrAcq == DIERR_OTHERAPPHASPRIO) {
                static int log_limit = 0;
                if (log_limit++ % 400 == 0) { // Log once per sec approx
                    std::cerr << "[DI Warning] Device unavailable. LMU (or another app) has Exclusive Priority. " 
                              << "You may have a 'Double FFB' conflict." << std::endl;
                }
            }
        }
    }
#endif
}
```

#### 3. Modify `src/GuiLayer.cpp`

```cpp
// ... [Includes] ...

void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    // LOCK MUTEX to prevent race condition with FFB Thread
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    // ... [Connection Status] ...

    ImGui::Text("Core Settings");
    
    // Device Selection
    static std::vector<DeviceInfo> devices;
    static int selected_device_idx = -1;
    
    // Scan button (or auto scan once)
    if (devices.empty()) {
        devices = DirectInputFFB::Get().EnumerateDevices();
    }

    if (ImGui::BeginCombo("FFB Device", selected_device_idx >= 0 ? devices[selected_device_idx].name.c_str() : "Select Device...")) {
        for (int i = 0; i < devices.size(); i++) {
            bool is_selected = (selected_device_idx == i);
            if (ImGui::Selectable(devices[i].name.c_str(), is_selected)) {
                selected_device_idx = i;
                DirectInputFFB::Get().SelectDevice(devices[i].guid);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Rescan Devices")) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        selected_device_idx = -1;
    }
    
    // NEW: Unbind Device Button
    ImGui::SameLine();
    if (ImGui::Button("Unbind Device")) {
        DirectInputFFB::Get().ReleaseDevice();
        selected_device_idx = -1;
    }

    // NEW: Display Acquisition Mode
    if (DirectInputFFB::Get().IsActive()) {
        if (DirectInputFFB::Get().IsExclusive()) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Mode: EXCLUSIVE (Game FFB Blocked)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("LMUFFB has exclusive control.\nThe game can read steering but cannot send FFB.\nThis prevents 'Double FFB' issues.");
        } else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Mode: SHARED (Potential Conflict)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("LMUFFB is sharing the device.\nEnsure In-Game FFB is set to 'None' or 0% strength\nto avoid two force signals fighting each other.");
        }
    }

    ImGui::Separator();

    // ... [Rest of UI] ...
```