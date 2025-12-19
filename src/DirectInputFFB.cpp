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
    m_isExclusive = false;
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

    // Reset state
    m_isExclusive = false;

    // Attempt 1: Exclusive/Background (Best for FFB)
    std::cout << "[DI] Attempting to set Cooperative Level (Exclusive | Background)..." << std::endl;
    HRESULT hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    
    if (SUCCEEDED(hr)) {
        m_isExclusive = true;
        std::cout << "[DI] Cooperative Level set to EXCLUSIVE." << std::endl;
    } else {
        // Fallback: Non-Exclusive
        std::cerr << "[DI] Exclusive mode failed (Error: " << std::hex << hr << std::dec << "). Retrying in Non-Exclusive mode..." << std::endl;
        hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        
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
    if (FAILED(m_pDevice->Acquire())) {
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
