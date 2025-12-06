#include "DirectInputFFB.h"
#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <dinput.h>
#endif

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
#ifdef _WIN32
    if (m_pEffect) {
        m_pEffect->Unload();
        m_pEffect->Release();
        m_pEffect = nullptr;
    }
    if (m_pDevice) {
        m_pDevice->Unacquire();
        m_pDevice->Release();
        m_pDevice = nullptr;
    }
    if (m_pDI) {
        m_pDI->Release();
        m_pDI = nullptr;
    }
#endif
    m_active = false;
}

// Callback for enumeration
#ifdef _WIN32
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) {
    auto* devices = (std::vector<DeviceInfo>*)pContext;
    DeviceInfo info;
    info.guid = pdidInstance->guidInstance;
    // Convert WCHAR name to string (simplified)
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
    // Mock devices
    DeviceInfo d1; d1.name = "Simucube 2 Pro (Mock)";
    DeviceInfo d2; d2.name = "Logitech G29 (Mock)";
    devices.push_back(d1);
    devices.push_back(d2);
#endif
    return devices;
}

bool DirectInputFFB::SelectDevice(const GUID& guid) {
#ifdef _WIN32
    if (!m_pDI) return false;

    // Cleanup old
    if (m_pDevice) {
        m_pDevice->Unacquire();
        m_pDevice->Release();
        m_pDevice = nullptr;
    }

    if (FAILED(m_pDI->CreateDevice(guid, &m_pDevice, NULL))) {
        std::cerr << "[DI] Failed to create device." << std::endl;
        return false;
    }

    if (FAILED(m_pDevice->SetDataFormat(&c_dfDIJoystick))) {
        std::cerr << "[DI] Failed to set data format." << std::endl;
        return false;
    }

    // Exclusive/Background is typically required for FFB
    if (FAILED(m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND))) {
        std::cerr << "[DI] Failed to set cooperative level." << std::endl;
        return false;
    }

    if (FAILED(m_pDevice->Acquire())) {
        std::cerr << "[DI] Failed to acquire device." << std::endl;
        // Don't return false yet, might just need focus/retry
    }

    // Create Effect
    if (CreateEffect()) {
        m_active = true;
        // Need to store name? We had it in enumeration.
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

    // Clamp
    normalizedForce = (std::max)(-1.0, (std::min)(1.0, normalizedForce));

    // Scale to -10000..10000
    long magnitude = static_cast<long>(normalizedForce * 10000.0);

#ifdef _WIN32
    if (m_pEffect) {
        DICONSTANTFORCE cf;
        cf.lMagnitude = magnitude;
        DIEFFECT eff;
        ZeroMemory(&eff, sizeof(eff));
        eff.dwSize = sizeof(DIEFFECT);
        eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        eff.lpvTypeSpecificParams = &cf;
        
        // Use DIEP_START ensures it keeps running if interrupted? 
        // Or just DIEP_TYPESPECIFICPARAMS for update.
        m_pEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_START);
    }
#else
    // std::cout << "[DI Mock] Force: " << magnitude << std::endl;
#endif
}
