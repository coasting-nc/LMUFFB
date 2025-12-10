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

private:
    DirectInputFFB();
    ~DirectInputFFB();

    LPDIRECTINPUT8 m_pDI = nullptr;
    LPDIRECTINPUTDEVICE8 m_pDevice = nullptr;
    LPDIRECTINPUTEFFECT m_pEffect = nullptr;
    HWND m_hwnd = nullptr;
    
    bool m_active = false;
    std::string m_deviceName = "None";
    
    // Internal helper to create the Constant Force effect
    bool CreateEffect();

    long m_last_force = -999999; 
};

#endif // DIRECTINPUTFFB_H
