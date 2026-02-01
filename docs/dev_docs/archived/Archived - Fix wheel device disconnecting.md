TODO: move this document to an archive subfolder because it is outdate (already fixed).

# Technical Report: Connection Hardening & Device Persistence Implementation

**Target Version:** v0.4.44
**Priority:** Critical (User Experience / Stability)
**Context:** Addressing feedback from user GamerMuscle regarding connection stability and usability.

## 1. Executive Summary

Users have reported two major usability issues:
1.  **Connection Loss:** The app stops sending FFB when the window loses focus (Alt-Tab) or when the game momentarily steals Exclusive Mode. The current re-acquisition logic is insufficient because it does not explicitly restart the DirectInput Effect motor.
2.  **Lack of Persistence:** The user must manually re-select their steering wheel from the dropdown every time the app is launched.

Additionally, the console output is currently cluttered with "Clipping" warnings, which makes debugging connection issues difficult.

## 2. Technical Strategy

### A. Connection Hardening (The "Smart Reconnect")
We will overhaul the `UpdateForce` loop in `DirectInputFFB.cpp`.
*   **Diagnosis:** Instead of silently failing, we will catch specific DirectInput errors (`DIERR_INPUTLOST`, `DIERR_OTHERAPPHASPRIO`).
*   **Context Logging:** We will log the **Active Foreground Window** when a failure occurs. This definitively proves if the Game (LMU) is stealing the device priority.
*   **Motor Restart:** When we successfully re-acquire the device, we will explicitly call `m_pEffect->Start(1, 0)`. This fixes the "Silent Wheel" bug where the device is connected but the motor is stopped.

### B. Device Persistence
We will add logic to save the unique **GUID** of the selected DirectInput device to `config.ini`. On startup, the app will scan available devices and auto-select the one matching the saved GUID.

### C. Console Decluttering
We will remove the rate-limited "FFB Output Saturated" warning from the main loop to ensure the console remains clean for the new connection diagnostics.

---

## 3. Implementation Guide

### Step 1: Update `src/DirectInputFFB.h`
Add static helper methods to convert between the binary `GUID` struct and `std::string` for config storage.

```cpp
// src/DirectInputFFB.h

// ... existing includes ...

class DirectInputFFB {
public:
    // ... existing methods ...

    // NEW: Helpers for Config persistence
    static std::string GuidToString(const GUID& guid);
    static GUID StringToGuid(const std::string& str);

private:
    // ... existing members ...
};
```

### Step 2: Update `src/DirectInputFFB.cpp`
This is the core logic update. It includes the helpers, the console decluttering, and the robust reconnection logic.

```cpp
// src/DirectInputFFB.cpp

// ... existing includes ...
#include <cstdio> // For sscanf, sprintf
#include <psapi.h> // For GetActiveWindowTitle

// NEW: Helper to get foreground window title for diagnostics
std::string GetActiveWindowTitle() {
    char wnd_title[256];
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        GetWindowTextA(hwnd, wnd_title, sizeof(wnd_title));
        return std::string(wnd_title);
    }
    return "Unknown";
}

// NEW: Helper Implementations for GUID
std::string DirectInputFFB::GuidToString(const GUID& guid) {
    char buf[64];
    sprintf_s(buf, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}

GUID DirectInputFFB::StringToGuid(const std::string& str) {
    GUID guid = { 0 };
    if (str.empty()) return guid;
    unsigned long p0;
    unsigned int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;
    int n = sscanf_s(str.c_str(), "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    if (n == 11) {
        guid.Data1 = p0;
        guid.Data2 = (unsigned short)p1;
        guid.Data3 = (unsigned short)p2;
        guid.Data4[0] = (unsigned char)p3; guid.Data4[1] = (unsigned char)p4;
        guid.Data4[2] = (unsigned char)p5; guid.Data4[3] = (unsigned char)p6;
        guid.Data4[4] = (unsigned char)p7; guid.Data4[5] = (unsigned char)p8;
        guid.Data4[6] = (unsigned char)p9; guid.Data4[7] = (unsigned char)p10;
    }
    return guid;
}

void DirectInputFFB::UpdateForce(double normalizedForce) {
    if (!m_active) return;

    // Sanity Check: If 0.0, stop effect to prevent residual hum
    if (std::abs(normalizedForce) < 0.00001) normalizedForce = 0.0;

    // --- DECLUTTERING: REMOVED CLIPPING WARNING ---
    /*
    if (std::abs(normalizedForce) > 0.99) {
        static int clip_log = 0;
        if (clip_log++ % 400 == 0) { 
            std::cout << "[DI] WARNING: FFB Output Saturated..." << std::endl;
        }
    }
    */
    // ----------------------------------------------

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
        eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        eff.lpvTypeSpecificParams = &cf;
        
        // Try to update parameters
        HRESULT hr = m_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
        
        // --- DIAGNOSTIC & RECOVERY LOGIC ---
        if (FAILED(hr)) {
            // 1. Identify the Error
            std::string errorType = "Unknown";
            bool recoverable = false;

            if (hr == DIERR_INPUTLOST) {
                errorType = "DIERR_INPUTLOST (Physical disconnect or Driver reset)";
                recoverable = true;
            } else if (hr == DIERR_NOTACQUIRED) {
                errorType = "DIERR_NOTACQUIRED (Lost focus/lock)";
                recoverable = true;
            } else if (hr == DIERR_OTHERAPPHASPRIO) {
                errorType = "DIERR_OTHERAPPHASPRIO (Another app stole the device!)";
                recoverable = true;
            }

            // 2. Log the Context (Rate limited to 1s)
            static DWORD lastLogTime = 0;
            if (GetTickCount() - lastLogTime > 1000) {
                std::cerr << "[DI ERROR] Failed to update force. Error: " << errorType << std::endl;
                std::cerr << "           Active Window: [" << GetActiveWindowTitle() << "]" << std::endl;
                lastLogTime = GetTickCount();
            }

            // 3. Attempt Recovery
            if (recoverable) {
                HRESULT hrAcq = m_pDevice->Acquire();
                
                if (SUCCEEDED(hrAcq)) {
                    std::cout << "[DI RECOVERY] Device Re-Acquired successfully." << std::endl;
                    
                    // CRITICAL FIX: Restart the effect
                    // Often, re-acquiring is not enough; the effect must be restarted.
                    m_pEffect->Start(1, 0); 
                    
                    // Retry the update immediately
                    m_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
                } 
            }
        }
    }
#endif
}
```

### Step 3: Update `src/Config.h`
Add the static variable to hold the GUID string.

```cpp
// src/Config.h

class Config {
public:
    // ... existing members ...
    
    // NEW: Persist selected device
    static std::string m_last_device_guid;
};
```

### Step 4: Update `src/Config.cpp`
Implement the Save/Load logic for the GUID.

```cpp
// src/Config.cpp

// Initialize
std::string Config::m_last_device_guid = "";

void Config::Save(const FFBEngine& engine, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        // ... existing saves ...
        file << "last_device_guid=" << m_last_device_guid << "\n"; // NEW
        
        // ... rest of save ...
    }
}

void Config::Load(FFBEngine& engine, const std::string& filename) {
    // ... existing load loop ...
    while (std::getline(file, line)) {
        // ... parsing ...
        if (key == "last_device_guid") m_last_device_guid = value; // NEW
        // ... existing keys ...
    }
}
```

### Step 5: Update `src/GuiLayer.cpp`
Implement the auto-selection logic on startup and save logic on change.

```cpp
// src/GuiLayer.cpp

void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    // ... [Setup] ...
    
    // Device Selection
    static std::vector<DeviceInfo> devices;
    static int selected_device_idx = -1;
    
    // Scan button (or auto scan once)
    if (devices.empty()) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        
        // NEW: Auto-select last used device
        if (selected_device_idx == -1 && !Config::m_last_device_guid.empty()) {
            GUID target = DirectInputFFB::StringToGuid(Config::m_last_device_guid);
            for (int i = 0; i < devices.size(); i++) {
                if (memcmp(&devices[i].guid, &target, sizeof(GUID)) == 0) {
                    selected_device_idx = i;
                    DirectInputFFB::Get().SelectDevice(devices[i].guid);
                    break;
                }
            }
        }
    }

    if (ImGui::BeginCombo("FFB Device", selected_device_idx >= 0 ? devices[selected_device_idx].name.c_str() : "Select Device...")) {
        for (int i = 0; i < devices.size(); i++) {
            bool is_selected = (selected_device_idx == i);
            if (ImGui::Selectable(devices[i].name.c_str(), is_selected)) {
                selected_device_idx = i;
                DirectInputFFB::Get().SelectDevice(devices[i].guid);
                
                // NEW: Save selection to config immediately
                Config::m_last_device_guid = DirectInputFFB::GuidToString(devices[i].guid);
                Config::Save(engine); 
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    // ... [Rest of file] ...
```
