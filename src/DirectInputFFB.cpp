#include "DirectInputFFB.h"

// Standard Library Headers
#include <iostream>
#include <cmath>
#include <cstdio> // For sscanf, sprintf
#include <algorithm> // For std::max, std::min

// Platform-Specific Headers
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <dinput.h>
#include <iomanip> // For std::hex
#include <string>
#endif

// Constants
namespace {
    constexpr uint32_t DIAGNOSTIC_LOG_INTERVAL_MS = 1000; // Rate limit diagnostic logging to 1 second
    constexpr uint32_t RECOVERY_COOLDOWN_MS = 2000;       // Wait 2 seconds between recovery attempts
}

// Keep existing implementations
DirectInputFFB& DirectInputFFB::Get() {
    static DirectInputFFB instance;
    return instance;
}

DirectInputFFB::DirectInputFFB() {}

// NEW: Helper to get foreground window title for diagnostics - REMOVED for Security/Privacy
std::string DirectInputFFB::GetActiveWindowTitle() {
    return "Window Tracking Disabled";
}

// NEW: Helper Implementations for GUID
std::string DirectInputFFB::GuidToString(const GUID& guid) {
    char buf[64];
#ifdef _WIN32
    sprintf_s(buf, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
#else
    snprintf(buf, sizeof(buf), "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        (unsigned int)guid.Data1, (unsigned int)guid.Data2, (unsigned int)guid.Data3,
        (unsigned int)guid.Data4[0], (unsigned int)guid.Data4[1], (unsigned int)guid.Data4[2], (unsigned int)guid.Data4[3],
        (unsigned int)guid.Data4[4], (unsigned int)guid.Data4[5], (unsigned int)guid.Data4[6], (unsigned int)guid.Data4[7]);
#endif
    return std::string(buf);
}

GUID DirectInputFFB::StringToGuid(const std::string& str) {
    GUID guid = { 0 };
    if (str.empty()) return guid;
    unsigned long p0;
    unsigned short p1, p2;
    unsigned int p3, p4, p5, p6, p7, p8, p9, p10;
#ifdef _WIN32
    int n = sscanf_s(str.c_str(), "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
#else
    int n = sscanf(str.c_str(), "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
#endif
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



#ifdef _WIN32
/**
 * @brief Returns the description for a DirectInput return code.
 * 
 * Parsed from: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416869(v=vs.85)#constants
 * 
 * @param hr The HRESULT returned by a DirectInput method.
 * @return const char* The description of the error or status code.
 */
const char* GetDirectInputErrorString(HRESULT hr) {
    switch (hr) {
        // Success Codes
        case S_OK: // Also DI_OK
            return "The operation completed successfully (S_OK).";
        case S_FALSE: // Also DI_BUFFEROVERFLOW, DI_NOEFFECT, DI_NOTATTACHED, DI_PROPNOEFFECT
            return "Operation technically succeeded but had no effect or hit a warning (S_FALSE). The device buffer overflowed and some input was lost. This value is equal to DI_BUFFEROVERFLOW, DI_NOEFFECT, DI_NOTATTACHED, DI_PROPNOEFFECT.";
        case DI_DOWNLOADSKIPPED:
            return "The parameters of the effect were successfully updated, but the effect could not be downloaded because the associated device was not acquired in exclusive mode.";
        case DI_EFFECTRESTARTED:
            return "The effect was stopped, the parameters were updated, and the effect was restarted.";
        case DI_POLLEDDEVICE:
            return "The device is a polled device.. As a result, device buffering does not collect any data and event notifications is not signaled until the IDirectInputDevice8 Interface method is called.";
        case DI_SETTINGSNOTSAVED:
            return "The action map was applied to the device, but the settings could not be saved.";
        case DI_TRUNCATED:
            return "The parameters of the effect were successfully updated, but some of them were beyond the capabilities of the device and were truncated to the nearest supported value.";
        case DI_TRUNCATEDANDRESTARTED:
            return "Equal to DI_EFFECTRESTARTED | DI_TRUNCATED.";
        case DI_WRITEPROTECT:
            return "A SUCCESS code indicating that settings cannot be modified.";

        // Error Codes
        case DIERR_ACQUIRED:
            return "The operation cannot be performed while the device is acquired.";
        case DIERR_ALREADYINITIALIZED:
            return "This object is already initialized.";
        case DIERR_BADDRIVERVER:
            return "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components.";
        case DIERR_BETADIRECTINPUTVERSION:
            return "The application was written for an unsupported prerelease version of DirectInput.";
        case DIERR_DEVICEFULL:
            return "The device is full.";
        case DIERR_DEVICENOTREG: // Equal to REGDB_E_CLASSNOTREG
            return "The device or device instance is not registered with DirectInput.";
        case DIERR_EFFECTPLAYING:
            return "The parameters were updated in memory but were not downloaded to the device because the device does not support updating an effect while it is still playing.";
        case DIERR_GENERIC: // Equal to E_FAIL
            return "An undetermined error occurred inside the DirectInput subsystem.";
        case DIERR_HANDLEEXISTS: // Equal to E_ACCESSDENIED
            return "Access denied or handle already exists. Another application may have exclusive access.";
        case DIERR_HASEFFECTS:
            return "The device cannot be reinitialized because effects are attached to it.";
        case DIERR_INCOMPLETEEFFECT:
            return "The effect could not be downloaded because essential information is missing. For example, no axes have been associated with the effect, or no type-specific information has been supplied.";
        case DIERR_INPUTLOST:
            return "Access to the input device has been lost. It must be reacquired.";
        case DIERR_INVALIDPARAM: // Equal to E_INVALIDARG
            return "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called.";
        case DIERR_MAPFILEFAIL:
            return "An error has occurred either reading the vendor-supplied action-mapping file for the device or reading or writing the user configuration mapping file for the device.";
        case DIERR_MOREDATA:
            return "Not all the requested information fit into the buffer.";
        case DIERR_NOAGGREGATION:
            return "This object does not support aggregation.";
        case DIERR_NOINTERFACE: // Equal to E_NOINTERFACE
            return "The object does not support the specified interface.";
        case DIERR_NOTACQUIRED:
            return "The operation cannot be performed unless the device is acquired.";
        case DIERR_NOTBUFFERED:
            return "The device is not buffered. Set the DIPROP_BUFFERSIZE property to enable buffering.";
        case DIERR_NOTDOWNLOADED:
            return "The effect is not downloaded.";
        case DIERR_NOTEXCLUSIVEACQUIRED:
            return "The operation cannot be performed unless the device is acquired in DISCL_EXCLUSIVE mode.";
        case DIERR_NOTFOUND:
            return "The requested object does not exist (DIERR_NOTFOUND).";
        // case DIERR_OBJECTNOTFOUND: // Duplicate of DIERR_NOTFOUND
        //    return "The requested object does not exist.";
        case DIERR_OLDDIRECTINPUTVERSION:
            return "The application requires a newer version of DirectInput.";
        // case DIERR_OTHERAPPHASPRIO: // Duplicate of DIERR_HANDLEEXISTS (E_ACCESSDENIED)
        //    return "Another application has a higher priority level, preventing this call from succeeding.";
        case DIERR_OUTOFMEMORY: // Equal to E_OUTOFMEMORY
            return "The DirectInput subsystem could not allocate sufficient memory to complete the call.";
        // case DIERR_READONLY: // Duplicate of DIERR_HANDLEEXISTS (E_ACCESSDENIED)
        //    return "The specified property cannot be changed.";
        case DIERR_REPORTFULL:
            return "More information was requested to be sent than can be sent to the device.";
        case DIERR_UNPLUGGED:
            return "The operation could not be completed because the device is not plugged in.";
        case DIERR_UNSUPPORTED: // Equal to E_NOTIMPL
            return "The function called is not supported at this time.";
        case E_HANDLE:
            return "The HWND parameter is not a valid top-level window that belongs to the process.";
        case E_PENDING:
            return "Data is not yet available.";
        case E_POINTER:
            return "An invalid pointer, usually NULL, was passed as a parameter.";
        
        default:
            return "Unknown DirectInput Error";
    }
}
#endif

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
#ifdef _WIN32
    if (m_pDI) {
        ((IDirectInput8*)m_pDI)->Release();
        m_pDI = nullptr;
    }
#endif
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
    ((IDirectInput8*)m_pDI)->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, &devices, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
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
        ((IDirectInputEffect*)m_pEffect)->Stop();
        ((IDirectInputEffect*)m_pEffect)->Unload();
        ((IDirectInputEffect*)m_pEffect)->Release();
        m_pEffect = nullptr;
    }
    if (m_pDevice) {
        ((IDirectInputDevice8*)m_pDevice)->Unacquire();
        ((IDirectInputDevice8*)m_pDevice)->Release();
        m_pDevice = nullptr;
    }
#endif
    m_active = false;
    m_isExclusive = false;
    m_deviceName = "None";
#ifdef _WIN32
    std::cout << "[DI] Device released by user." << std::endl;
#endif
}

bool DirectInputFFB::SelectDevice(const GUID& guid) {
#ifdef _WIN32
    if (!m_pDI) return false;

    // Cleanup old using new method
    ReleaseDevice();

    std::cout << "[DI] Attempting to create device..." << std::endl;
    if (FAILED(((IDirectInput8*)m_pDI)->CreateDevice(guid, (IDirectInputDevice8**)&m_pDevice, NULL))) {
        std::cerr << "[DI] Failed to create device." << std::endl;
        return false;
    }

    std::cout << "[DI] Setting Data Format..." << std::endl;
    if (FAILED(((IDirectInputDevice8*)m_pDevice)->SetDataFormat(&c_dfDIJoystick))) {
        std::cerr << "[DI] Failed to set data format." << std::endl;
        return false;
    }

    // Reset state
    m_isExclusive = false;

    // Attempt 1: Exclusive/Background (Best for FFB)
    std::cout << "[DI] Attempting to set Cooperative Level (Exclusive | Background)..." << std::endl;
    HRESULT hr = ((IDirectInputDevice8*)m_pDevice)->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    
    if (SUCCEEDED(hr)) {
        m_isExclusive = true;
        std::cout << "[DI] Cooperative Level set to EXCLUSIVE." << std::endl;
    } else {
        // Fallback: Non-Exclusive
        std::cerr << "[DI] Exclusive mode failed (Error: " << std::hex << hr << std::dec << "). Retrying in Non-Exclusive mode..." << std::endl;
        hr = ((IDirectInputDevice8*)m_pDevice)->SetCooperativeLevel(m_hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        
        if (SUCCEEDED(hr)) {
            m_isExclusive = false;
            std::cout << "[DI] Cooperative Level set to NON-EXCLUSIVE." << std::endl;
        }
    }
    
    if (FAILED(hr)) {
        std::cerr << "[DI] Failed to set cooperative level (Non-Exclusive failed too)." << std::endl;
        return false;
    }

    std::cout << "[DI] Acquiring device..." << std::endl;
    if (FAILED(((IDirectInputDevice8*)m_pDevice)->Acquire())) {
        std::cerr << "[DI] Failed to acquire device." << std::endl;
        // Don't return false yet, might just need focus/retry
    } else {
        std::cout << "[DI] Device Acquired in " << (m_isExclusive ? "EXCLUSIVE" : "NON-EXCLUSIVE") << " mode." << std::endl;
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
    m_isExclusive = true; // Default to true in mock to verify UI logic
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

    if (FAILED(((IDirectInputDevice8*)m_pDevice)->CreateEffect(GUID_ConstantForce, &eff, (IDirectInputEffect**)&m_pEffect, NULL))) {
        std::cerr << "[DI] Failed to create Constant Force effect." << std::endl;
        return false;
    }
    
    // Start immediately
    ((IDirectInputEffect*)m_pEffect)->Start(1, 0);
    return true;
#else
    return true;
#endif
}

bool DirectInputFFB::UpdateForce(double normalizedForce) {
    if (!m_active) return false;

    // Sanity Check: If 0.0, stop effect to prevent residual hum
    if (std::abs(normalizedForce) < 0.00001) normalizedForce = 0.0;

    // Clamp
    normalizedForce = (std::max)(-1.0, (std::min)(1.0, normalizedForce));

    // Scale to -10000..10000
    long magnitude = static_cast<long>(normalizedForce * 10000.0);

    // Optimization: Don't call driver if value hasn't changed
    if (magnitude == m_last_force) return false;
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
        HRESULT hr = ((IDirectInputEffect*)m_pEffect)->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
        
        // --- DIAGNOSTIC & RECOVERY LOGIC ---
        if (FAILED(hr)) {
            // 1. Identify the Error
            std::string errorType = GetDirectInputErrorString(hr);

            // Append Custom Advice for Priority/Exclusive Errors
            if (hr == DIERR_OTHERAPPHASPRIO || hr == DIERR_NOTEXCLUSIVEACQUIRED ) {
                errorType += " [CRITICAL: Game has stolen priority! DISABLE IN-GAME FFB]";
                
                // Update exclusivity state to reflect reality
                m_isExclusive = false;
            }

            // FIX: Default to TRUE. If update failed, we must try to reconnect.
            bool recoverable = true; 

            // 2. Log the Context (Rate limited)
            static uint32_t lastLogTime = 0;
            if (GetTickCount() - lastLogTime > DIAGNOSTIC_LOG_INTERVAL_MS) {
                std::cerr << "[DI ERROR] Failed to update force. Error: " << errorType 
                          << " (0x" << std::hex << hr << std::dec << ")" << std::endl;
                std::cerr << "           Active Window: [" << GetActiveWindowTitle() << "]" << std::endl;
                lastLogTime = GetTickCount();
            }

            // 3. Attempt Recovery (with Smart Cool-down)
            if (recoverable) {
                // Throttle recovery attempts to prevent CPU spam when device is locked
                static uint32_t lastRecoveryAttempt = 0;
                uint32_t now = GetTickCount();
                
                // Only attempt recovery if cooldown period has elapsed
                if (now - lastRecoveryAttempt > RECOVERY_COOLDOWN_MS) {
                    lastRecoveryAttempt = now; // Mark this attempt
                    
                    // --- DYNAMIC PROMOTION FIX ---
                    // If we are stuck in "Shared Mode" (0x80040205), standard Acquire() 
                    // just re-confirms Shared Mode. We must force a mode switch.
                    if (hr == DIERR_NOTEXCLUSIVEACQUIRED) {
                        std::cout << "[DI] Attempting to promote to Exclusive Mode..." << std::endl;
                        ((IDirectInputDevice8*)m_pDevice)->Unacquire();
                        ((IDirectInputDevice8*)m_pDevice)->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
                    }
                    // -----------------------------

                    HRESULT hrAcq = ((IDirectInputDevice8*)m_pDevice)->Acquire();
                    
                    if (SUCCEEDED(hrAcq)) {
                        // Log recovery success (rate-limited for diagnostics)
                        static uint32_t lastSuccessLog = 0;
                        if (GetTickCount() - lastSuccessLog > 5000) { // 5 second cooldown
                            std::cout << "[DI RECOVERY] Device re-acquired successfully. FFB motor restarted." << std::endl;
                            lastSuccessLog = GetTickCount();
                        }
                        
                        // Update our internal state if we fixed the exclusivity
                        if (hr == DIERR_NOTEXCLUSIVEACQUIRED) {
                            m_isExclusive = true; 
                            
                            // One-time notification when Dynamic Promotion first succeeds
                            static bool firstPromotionSuccess = false;
                            if (!firstPromotionSuccess) {
                                std::cout << "\n"
                                          << "========================================\n"
                                          << "[SUCCESS] Dynamic Promotion Active!\n"
                                          << "lmuFFB has successfully recovered exclusive\n"
                                          << "control after detecting a conflict.\n"
                                          << "This feature will continue to protect your\n"
                                          << "FFB experience automatically.\n"
                                          << "========================================\n" << std::endl;
                                firstPromotionSuccess = true;
                            }
                        }

                        // Restart the effect to ensure motor is active
                        ((IDirectInputEffect*)m_pEffect)->Start(1, 0);
                        
                        // Retry the update immediately
                        ((IDirectInputEffect*)m_pEffect)->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
                    }
                }
            }
        }
    }
#endif
    return true;
}
