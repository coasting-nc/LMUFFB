#ifndef DYNAMICVJOY_H
#define DYNAMICVJOY_H

#include <windows.h>
#include <iostream>
#include "public.h" // vJoy SDK Header

// Typedefs for vJoy functions
typedef BOOL (WINAPI *vJoyEnabled_t)();
typedef BOOL (WINAPI *AcquireVJD_t)(UINT);
typedef VOID (WINAPI *RelinquishVJD_t)(UINT);
typedef BOOL (WINAPI *SetAxis_t)(LONG, UINT, UINT);
typedef enum VjdStat (WINAPI *GetVJDStatus_t)(UINT);

class DynamicVJoy {
public:
    static DynamicVJoy& Get() {
        static DynamicVJoy instance;
        return instance;
    }

    bool Load() {
        if (m_hModule) return true; // Already loaded

        m_hModule = LoadLibraryA("vJoyInterface.dll");
        if (!m_hModule) {
            std::cout << "[vJoy] Library not found. vJoy support disabled." << std::endl;
            return false;
        }

        m_vJoyEnabled = (vJoyEnabled_t)GetProcAddress(m_hModule, "vJoyEnabled");
        m_AcquireVJD = (AcquireVJD_t)GetProcAddress(m_hModule, "AcquireVJD");
        m_RelinquishVJD = (RelinquishVJD_t)GetProcAddress(m_hModule, "RelinquishVJD");
        m_SetAxis = (SetAxis_t)GetProcAddress(m_hModule, "SetAxis");
        m_GetVJDStatus = (GetVJDStatus_t)GetProcAddress(m_hModule, "GetVJDStatus");

        if (!m_vJoyEnabled || !m_AcquireVJD || !m_RelinquishVJD || !m_SetAxis || !m_GetVJDStatus) {
            std::cerr << "[vJoy] Library loaded but functions missing." << std::endl;
            FreeLibrary(m_hModule);
            m_hModule = NULL;
            return false;
        }

        std::cout << "[vJoy] Library loaded successfully." << std::endl;
        return true;
    }

    bool Enabled() { return (m_hModule && m_vJoyEnabled) ? m_vJoyEnabled() : false; }
    BOOL Acquire(UINT id) { return (m_hModule && m_AcquireVJD) ? m_AcquireVJD(id) : FALSE; }
    VOID Relinquish(UINT id) { if (m_hModule && m_RelinquishVJD) m_RelinquishVJD(id); }
    BOOL SetAxis(LONG value, UINT id, UINT axis) { return (m_hModule && m_SetAxis) ? m_SetAxis(value, id, axis) : FALSE; }
    VjdStat GetStatus(UINT id) { return (m_hModule && m_GetVJDStatus) ? m_GetVJDStatus(id) : VJD_STAT_MISS; }

    bool IsLoaded() const { return m_hModule != NULL; }

private:
    DynamicVJoy() {}
    ~DynamicVJoy() {
        if (m_hModule) FreeLibrary(m_hModule);
    }

    HMODULE m_hModule = NULL;
    vJoyEnabled_t m_vJoyEnabled = NULL;
    AcquireVJD_t m_AcquireVJD = NULL;
    RelinquishVJD_t m_RelinquishVJD = NULL;
    SetAxis_t m_SetAxis = NULL;
    GetVJDStatus_t m_GetVJDStatus = NULL;
};

#endif // DYNAMICVJOY_H
