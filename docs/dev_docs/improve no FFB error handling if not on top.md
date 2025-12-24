To fix the issue where Force Feedback is lost when the app is in the background, we need to improve the error handling in `DirectInputFFB.cpp`.

**The Problem:**
The log shows `[DI ERROR] Failed to update force. Error: Unknown`.
Currently, the code only attempts to "Recover" (Re-acquire the device) if the error matches a specific list (`DIERR_INPUTLOST`, `DIERR_NOTACQUIRED`, `DIERR_OTHERAPPHASPRIO`).
Since the error is "Unknown" (a code we didn't explicitly list), the app **does not try to re-acquire the device**. It just logs the error and does nothing, leaving the FFB dead until you restart or maybe refocus (which might trigger an OS-level re-acquisition).

**The Fix:**
1.  **Log the Error Code:** We will print the hexadecimal error code (HRESULT) so we know exactly what's happening (e.g., is it `E_ACCESSDENIED`?).
2.  **Aggressive Recovery:** We will treat **ALL** errors from `SetParameters` as "Recoverable". If the FFB command failed, something is wrong with the connection, so we should always try to `Acquire()` and `Start()` the motor again.

This should make the app fight back if the game (or Windows) tries to steal the device focus.

### 1. Modify `src/DirectInputFFB.cpp`

I will update the `UpdateForce` function to include the hex code and default `recoverable` to true.

```cpp
// src/DirectInputFFB.cpp

// Add this include at the top if missing
#include <iomanip> // For std::hex

// ... inside UpdateForce ...

#ifdef _WIN32
    if (m_pEffect) {
        // ... [Setup DICONSTANTFORCE and DIEFFECT structs] ...
        
        // Try to update parameters
        HRESULT hr = m_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
        
        // --- DIAGNOSTIC & RECOVERY LOGIC ---
        if (FAILED(hr)) {
            // 1. Identify the Error
            std::string errorType = "Unknown";
            // Default to TRUE: If update failed, we should try to fix the connection
            bool recoverable = true; 

            if (hr == DIERR_INPUTLOST) {
                errorType = "DIERR_INPUTLOST";
            } else if (hr == DIERR_NOTACQUIRED) {
                errorType = "DIERR_NOTACQUIRED";
            } else if (hr == DIERR_OTHERAPPHASPRIO) {
                errorType = "DIERR_OTHERAPPHASPRIO";
            } else if (hr == E_HANDLE) {
                errorType = "E_HANDLE (Invalid Handle)";
            } else {
                // Keep Unknown but recoverable
            }

            // 2. Log the Context (Rate limited to 1s)
            static DWORD lastLogTime = 0;
            if (GetTickCount() - lastLogTime > DIAGNOSTIC_LOG_INTERVAL_MS) {
                std::cerr << "[DI ERROR] Failed to update force. Error: " << errorType 
                          << " (0x" << std::hex << hr << std::dec << ")" << std::endl;
                std::cerr << "           Active Window: [" << GetActiveWindowTitle() << "]" << std::endl;
                lastLogTime = GetTickCount();
            }

            // 3. Attempt Recovery
            if (recoverable) {
                HRESULT hrAcq = m_pDevice->Acquire();
                
                if (SUCCEEDED(hrAcq)) {
                    // Only log recovery success if we were previously logging errors
                    // (Avoid spamming console on every frame if it flickers)
                    // But for now, let's log it to confirm the fix works.
                    // std::cout << "[DI RECOVERY] Device Re-Acquired." << std::endl;
                    
                    // CRITICAL FIX: Restart the effect
                    m_pEffect->Start(1, 0); 
                    
                    // Retry the update immediately
                    m_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
                } 
            }
        }
    }
#endif
```

### 2. Instructions for the User

After applying this fix, if the issue persists, the console will now show a code like `0x80070005`.
*   If the log shows `DIERR_OTHERAPPHASPRIO` repeatedly when the game is focused, it means the game is forcing Exclusive Mode and stealing it. In that case, you **must** disable FFB in the game settings (set to "None" or 0 strength), or start LMUFFB *after* the game (though LMUFFB tries to be exclusive too).
*   However, with `recoverable = true`, LMUFFB will now instantly try to steal it back every frame (400 times a second). This might cause a "tug of war", but it's better than silently dying.

### 3. Applying the Fix

I will now apply the changes to `src/DirectInputFFB.cpp`.

```cpp
// File: src/DirectInputFFB.cpp
// ... (includes)
#include <iomanip> // Added for hex output

// ... (rest of file until UpdateForce)

void DirectInputFFB::UpdateForce(double normalizedForce) {
    if (!m_active) return;

    // Sanity Check
    if (std::abs(normalizedForce) < 0.00001) normalizedForce = 0.0;

    // Clamp
    normalizedForce = (std::max)(-1.0, (std::min)(1.0, normalizedForce));

    // Scale
    long magnitude = static_cast<long>(normalizedForce * 10000.0);

    // Optimization
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
            // FIX: Default to TRUE. If update failed, we must try to reconnect.
            bool recoverable = true; 

            if (hr == DIERR_INPUTLOST) {
                errorType = "DIERR_INPUTLOST";
            } else if (hr == DIERR_NOTACQUIRED) {
                errorType = "DIERR_NOTACQUIRED";
            } else if (hr == DIERR_OTHERAPPHASPRIO) {
                errorType = "DIERR_OTHERAPPHASPRIO";
            } else if (hr == E_HANDLE) {
                errorType = "E_HANDLE";
            }

            // 2. Log the Context (Rate limited)
            static DWORD lastLogTime = 0;
            if (GetTickCount() - lastLogTime > DIAGNOSTIC_LOG_INTERVAL_MS) {
                std::cerr << "[DI ERROR] Failed to update force. Error: " << errorType 
                          << " (0x" << std::hex << hr << std::dec << ")" << std::endl;
                std::cerr << "           Active Window: [" << GetActiveWindowTitle() << "]" << std::endl;
                lastLogTime = GetTickCount();
            }

            // 3. Attempt Recovery
            if (recoverable) {
                HRESULT hrAcq = m_pDevice->Acquire();
                
                if (SUCCEEDED(hrAcq)) {
                    // Restart the effect to ensure motor is active
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

This change ensures that "Unknown" errors trigger a recovery attempt, which should solve the issue where the app gives up on the device when it loses focus.