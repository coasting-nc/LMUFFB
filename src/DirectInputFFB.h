#ifndef DIRECTINPUTFFB_H
#define DIRECTINPUTFFB_H

#include <vector>
#include <string>
#include <atomic>

#ifdef _WIN32
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#else
#include "lmu_sm_interface/LinuxMock.h"
// Mock types for non-Windows build/test
typedef void* LPDIRECTINPUT8;
typedef void* LPDIRECTINPUTDEVICE8;
typedef void* LPDIRECTINPUTEFFECT;
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
    // Returns true if the hardware was actually updated (value changed)
    bool UpdateForce(double normalizedForce);

    // NEW: Helpers for Config persistence
    static std::string GuidToString(const GUID& guid);
    static GUID StringToGuid(const std::string& str);
    static std::string GetActiveWindowTitle();

    bool IsActive() const { return m_active; }
    std::string GetCurrentDeviceName() const { return m_deviceName; }
    
    // Check if device was acquired in exclusive mode
    bool IsExclusive() const { return m_isExclusive; }

private:
    DirectInputFFB();
    ~DirectInputFFB();

    LPDIRECTINPUT8 m_pDI = nullptr;
    LPDIRECTINPUTDEVICE8 m_pDevice = nullptr;
    LPDIRECTINPUTEFFECT m_pEffect = nullptr;
    HWND m_hwnd = nullptr;
    
    bool m_active = false;
    bool m_isExclusive = false; // Track acquisition mode
    std::string m_deviceName = "None";
    
    // Internal helper to create the Constant Force effect
    bool CreateEffect();

    long m_last_force = -999999; 
};

#endif // DIRECTINPUTFFB_H
